// AlgoBench.exe <dbDir> [scoresCsv]
// FPAlgo 정확도 벤치 (알고리즘 튜닝용, 제출물 아님)
// - 폴더의 모든 tif에서 특징 추출 → 모든 무순서 쌍 비교 (같은 손가락 = genuine)
// - 추출 실패 영상이 포함된 쌍은 점수 0(거부)으로 계산해 실패 누락으로 EER이 좋아 보이지 않게 한다
// - 매처별 EER / FMR100 / FMR1000 / ZeroFMR 출력, scoresCsv에 쌍별 점수 덤프
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "PathUtil.h"
#include "Pipeline.h"
#include "db/TiffReader.h"
#include "match/Matcher.h"

using namespace fpa;

namespace {

struct Scorer {
  const char* name;
  std::function<double(const std::vector<Minutia>&, const std::vector<Minutia>&)> fn;
};

struct Metrics {
  double eer, fmr100, fmr1000, zeroFmr;
};

// 점수가 높을수록 genuine. FMR(t)=P(imp ≥ t), FNMR(t)=P(gen < t)
Metrics computeMetrics(std::vector<double> gen, std::vector<double> imp) {
  std::sort(gen.begin(), gen.end());
  std::sort(imp.begin(), imp.end());
  std::vector<double> th(gen);
  th.insert(th.end(), imp.begin(), imp.end());
  th.push_back(2.0);
  std::sort(th.begin(), th.end());
  th.erase(std::unique(th.begin(), th.end()), th.end());
  auto fmrAt = [&](double t) {
    return static_cast<double>(imp.end() - std::lower_bound(imp.begin(), imp.end(), t)) / imp.size();
  };
  auto fnmrAt = [&](double t) {
    return static_cast<double>(std::lower_bound(gen.begin(), gen.end(), t) - gen.begin()) / gen.size();
  };
  Metrics m{1, 1, 1, 1};
  double prevFmr = 1, prevFnmr = 0;
  bool eerSet = false;
  for (double t : th) {
    double fmr = fmrAt(t), fnmr = fnmrAt(t);
    if (!eerSet && fnmr >= fmr) {
      // 직전 임계값과 현재 임계값 사이에서 교차: 두 점의 평균 중 차이가 작은 쪽
      double a = (prevFmr + prevFnmr) / 2, b = (fmr + fnmr) / 2;
      m.eer = std::fabs(prevFmr - prevFnmr) < std::fabs(fmr - fnmr) ? a : b;
      eerSet = true;
    }
    if (fmr <= 0.01) m.fmr100 = std::min(m.fmr100, fnmr);
    if (fmr <= 0.001) m.fmr1000 = std::min(m.fmr1000, fnmr);
    if (fmr <= 0.0) m.zeroFmr = std::min(m.zeroFmr, fnmr);
    prevFmr = fmr;
    prevFnmr = fnmr;
  }
  return m;
}

std::string fingerOf(const std::wstring& name) {
  std::string s = fpe::toUtf8(name);
  return s.substr(0, s.find('_'));
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
  if (argc < 2) {
    printf("AlgoBench <dbDir> [scoresCsv]\n");
    return 2;
  }
  std::wstring dir = argv[1];
  std::vector<std::wstring> files;
  WIN32_FIND_DATAW fd;
  HANDLE hf = FindFirstFileW((dir + L"\\*.tif").c_str(), &fd);
  if (hf == INVALID_HANDLE_VALUE) {
    printf("no tif in dir\n");
    return 1;
  }
  do {
    files.push_back(fd.cFileName);
  } while (FindNextFileW(hf, &fd));
  FindClose(hf);
  std::sort(files.begin(), files.end());
  const size_t n = files.size();

  std::vector<Scorer> scorers = {
      {"final", [](const auto& e, const auto& a) { return compareFeatures(e, a); }},
  };
  for (const auto& s : matcherVariants()) scorers.push_back({s.name, s.fn});
  // FPA_BENCH_ONLY=mcc,hough : 지정한 매처만 (튜닝 속도용)
  if (const char* only = std::getenv("FPA_BENCH_ONLY")) {
    std::string list = std::string(",") + only + ",";
    scorers.erase(std::remove_if(scorers.begin(), scorers.end(),
                                 [&](const Scorer& s) { return list.find(std::string(",") + s.name + ",") == std::string::npos; }),
                  scorers.end());
  }

  // 1) 특징 추출 (병렬)
  std::vector<std::vector<Minutia>> feats(n);
  std::vector<int> ok(n, 0);
  std::atomic<size_t> next{0};
  unsigned nt = std::max(1u, std::thread::hardware_concurrency());
  auto t0 = std::chrono::steady_clock::now();
  {
    std::vector<std::thread> th;
    for (unsigned t = 0; t < nt; ++t)
      th.emplace_back([&] {
        for (size_t i; (i = next++) < n;) {
          fpe::GrayImage g;
          std::string err;
          if (!fpe::readTiff(dir + L"\\" + files[i], g, err)) continue;
          const char* msg = "";
          ok[i] = extractFeatures(g.pixels.data(), g.width, g.height, feats[i], &msg) == 0;
        }
      });
    for (auto& t : th) t.join();
  }
  auto t1 = std::chrono::steady_clock::now();
  size_t fte = 0, totalMin = 0;
  for (size_t i = 0; i < n; ++i) {
    fte += !ok[i];
    totalMin += feats[i].size();
  }

  // 2) 모든 무순서 쌍 비교 (병렬)
  struct Pair {
    int i, j;
    bool gen;
  };
  std::vector<Pair> pairs;
  for (size_t i = 0; i < n; ++i)
    for (size_t j = i + 1; j < n; ++j)
      pairs.push_back({static_cast<int>(i), static_cast<int>(j), fingerOf(files[i]) == fingerOf(files[j])});
  const size_t ns = scorers.size();
  std::vector<double> score(pairs.size() * ns, 0.0);
  std::vector<double> scorerUs(ns * nt, 0.0);
  next = 0;
  {
    std::vector<std::thread> th;
    for (unsigned t = 0; t < nt; ++t)
      th.emplace_back([&, t] {
        for (size_t p; (p = next++) < pairs.size();) {
          const Pair& pr = pairs[p];
          if (!ok[pr.i] || !ok[pr.j]) continue;
          for (size_t s = 0; s < ns; ++s) {
            auto a = std::chrono::steady_clock::now();
            score[p * ns + s] = scorers[s].fn(feats[pr.i], feats[pr.j]);
            scorerUs[t * ns + s] +=
                std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - a).count();
          }
        }
      });
    for (auto& t : th) t.join();
  }

  size_t ng = 0;
  for (const auto& p : pairs) ng += p.gen;
  printf("db %s images %zu FTE %zu avgMinutiae %.1f extract %.1fms/img genuine %zu impostor %zu\n",
         fpe::toUtf8(dir).c_str(), n, fte, n ? static_cast<double>(totalMin) / n : 0.0,
         std::chrono::duration<double, std::milli>(t1 - t0).count() * nt / n, ng, pairs.size() - ng);
  printf("%-14s %7s %7s %8s %8s %9s\n", "matcher", "EER%", "FMR100", "FMR1000", "ZeroFMR", "us/pair");
  for (size_t s = 0; s < ns; ++s) {
    std::vector<double> gen, imp;
    for (size_t p = 0; p < pairs.size(); ++p) (pairs[p].gen ? gen : imp).push_back(score[p * ns + s]);
    Metrics m = computeMetrics(gen, imp);
    double us = 0;
    for (unsigned t = 0; t < nt; ++t) us += scorerUs[t * ns + s];
    printf("%-14s %7.2f %7.2f %8.2f %8.2f %9.1f\n", scorers[s].name, m.eer * 100, m.fmr100 * 100, m.fmr1000 * 100,
           m.zeroFmr * 100, us / pairs.size());
  }

  if (argc >= 3) {
    FILE* f = _wfopen(argv[2], L"w");
    if (f) {
      fprintf(f, "a,b,type");
      for (const auto& s : scorers) fprintf(f, ",%s", s.name);
      fprintf(f, "\n");
      for (size_t p = 0; p < pairs.size(); ++p) {
        fprintf(f, "%s,%s,%c", fpe::toUtf8(files[pairs[p].i]).c_str(), fpe::toUtf8(files[pairs[p].j]).c_str(),
                pairs[p].gen ? 'G' : 'I');
        for (size_t s = 0; s < ns; ++s) fprintf(f, ",%.4f", score[p * ns + s]);
        fprintf(f, "\n");
      }
      fclose(f);
    }
  }
  return 0;
}
