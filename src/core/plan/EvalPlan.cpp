#include "plan/EvalPlan.h"

#include <algorithm>
#include <set>

#include "PathUtil.h"
#include "result/Csv.h"

namespace fpe {

bool EvalPlan::build(const EvalConfig& cfg, const FingerprintDB& db, EvalPlan& out,
                     std::vector<std::string>& warnings, std::string& err) {
  out = EvalPlan{};
  out.seed = cfg.seed;

  std::vector<int> enrollSet;
  std::vector<Pair> pairs;
  if (!cfg.pairsFile.empty()) {
    // 부분 평가: 파일에 나열된 쌍만
    if (!loadPairsFile(cfg.pairsFile, db, pairs, warnings, err)) return false;
    std::set<int> es;
    for (auto& p : pairs) es.insert(p.enroll);
    enrollSet.assign(es.begin(), es.end());
    out.enrollDesc = "쌍 파일의 등록 이미지";
    out.genuineDesc = out.impostorDesc = "쌍 파일 " + toUtf8(fileNameOf(cfg.pairsFile));
  } else {
    auto er = RuleFactory::enroll(cfg);
    auto gr = RuleFactory::genuine(cfg);
    auto ir = RuleFactory::impostor(cfg);
    Rng rEnroll(Rng::derive(cfg.seed, "enroll"));
    Rng rGen(Rng::derive(cfg.seed, "genuine"));
    Rng rImp(Rng::derive(cfg.seed, "impostor"));
    enrollSet = er->select(db, rEnroll);
    if (enrollSet.empty()) {
      err = "enroll rule selected no images";
      return false;
    }
    auto g = gr->generate(db, enrollSet, rGen);
    auto im = ir->generate(db, enrollSet, rImp);
    pairs.insert(pairs.end(), g.begin(), g.end());
    pairs.insert(pairs.end(), im.begin(), im.end());
    out.enrollDesc = er->describe();
    out.genuineDesc = gr->describe();
    out.impostorDesc = ir->describe();
  }

  // 동일 이미지 쌍·중복 쌍 제거
  std::sort(pairs.begin(), pairs.end());
  pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
  pairs.erase(std::remove_if(pairs.begin(), pairs.end(), [](const Pair& p) { return p.enroll == p.attempt; }),
              pairs.end());
  if (pairs.empty()) {
    err = "no comparison pairs generated";
    return false;
  }

  std::set<int> attemptSet;
  for (auto& p : pairs) {
    attemptSet.insert(p.attempt);
    (p.type == 'G' ? out.nGenuine : out.nImpostor)++;
  }
  out.nEnroll = enrollSet.size();
  out.nAttempt = attemptSet.size();

  // 실행 순서 무작위화: 알고리즘이 파일명 순서 등으로 규칙을 유추하지 못하게
  for (int i : enrollSet) out.extractJobs.push_back({0, JobKind::Enroll, i, -1, 0});
  for (int i : attemptSet) out.extractJobs.push_back({0, JobKind::Attempt, i, -1, 0});
  for (auto& p : pairs) out.compareJobs.push_back({0, JobKind::Compare, p.enroll, p.attempt, p.type});
  Rng rOrder1(Rng::derive(cfg.seed, "order-extract"));
  Rng rOrder2(Rng::derive(cfg.seed, "order-compare"));
  rOrder1.shuffle(out.extractJobs);
  rOrder2.shuffle(out.compareJobs);
  uint32_t id = 1;
  for (auto& j : out.extractJobs) j.id = id++;
  for (auto& j : out.compareJobs) j.id = id++;
  return true;
}

bool EvalPlan::save(const std::wstring& path, const FingerprintDB& db) const {
  std::string s = "# seed=" + std::to_string(seed) + "\n";
  s += "# enroll=" + enrollDesc + " | genuine=" + genuineDesc + " | impostor=" + impostorDesc + "\n";
  s += "jobId,kind,a,b,type\n";
  auto name = [&](int i) { return i >= 0 ? db.images()[i].name() : std::string(); };
  for (const auto* list : {&extractJobs, &compareJobs})
    for (const Job& j : *list)
      s += csvRow({std::to_string(j.id), std::string(1, (char)j.kind), name(j.a), name(j.b),
                   j.type ? std::string(1, j.type) : std::string()});
  return writeText(path, s);
}

bool EvalPlan::load(const std::wstring& path, const FingerprintDB& db, EvalPlan& out, std::string& err) {
  std::vector<uint8_t> buf;
  if (!readFile(path, buf)) {
    err = "cannot read plan " + toUtf8(path);
    return false;
  }
  out = EvalPlan{};
  std::string text(buf.begin(), buf.end());
  size_t sp = text.find("# seed=");
  if (sp != std::string::npos) out.seed = std::stoull(text.substr(sp + 7, text.find('\n', sp) - sp - 7));
  size_t dp = text.find("# enroll=");
  if (dp != std::string::npos) {
    std::string line = text.substr(dp + 9, text.find('\n', dp) - dp - 9);
    size_t g = line.find(" | genuine="), im = line.find(" | impostor=");
    if (g != std::string::npos && im != std::string::npos) {
      out.enrollDesc = line.substr(0, g);
      out.genuineDesc = line.substr(g + 11, im - g - 11);
      out.impostorDesc = line.substr(im + 12);
    }
  }
  std::set<int> es, as;
  for (const std::string& line : splitLines(text)) {
    if (line.rfind("jobId", 0) == 0) continue;
    auto f = parseCsvLine(line);
    if (f.size() < 5) continue;
    Job j;
    j.id = (uint32_t)std::stoul(f[0]);
    j.kind = (JobKind)f[1][0];
    j.a = db.find(f[2]);
    j.b = f[3].empty() ? -1 : db.find(f[3]);
    j.type = f[4].empty() ? 0 : f[4][0];
    if (j.a < 0 || (j.kind == JobKind::Compare && j.b < 0)) {
      err = "plan refers to missing image: " + line;
      return false;
    }
    if (j.kind == JobKind::Compare) {
      out.compareJobs.push_back(j);
      (j.type == 'G' ? out.nGenuine : out.nImpostor)++;
    } else {
      out.extractJobs.push_back(j);
      (j.kind == JobKind::Enroll ? es : as).insert(j.a);
    }
  }
  out.nEnroll = es.size();
  out.nAttempt = as.size();
  return !out.compareJobs.empty();
}

bool EvalPlan::savePairs(const std::wstring& dir, const FingerprintDB& db) const {
  std::string g = "# seed=" + std::to_string(seed) + " rule=" + genuineDesc + "\nenroll,attempt,type\n";
  std::string im = "# seed=" + std::to_string(seed) + " rule=" + impostorDesc + "\nenroll,attempt,type\n";
  for (const Job& j : compareJobs)
    (j.type == 'G' ? g : im) += csvRow({db.images()[j.a].name(), db.images()[j.b].name(), std::string(1, j.type)});
  return writeText(joinPath(dir, L"pairs_genuine.csv"), g) && writeText(joinPath(dir, L"pairs_impostor.csv"), im);
}

}  // namespace fpe
