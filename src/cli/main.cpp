// FPEvalCli.exe — GUI 없이 같은 평가 엔진(FPEvalCore)을 실행하는 콘솔 도구 (자동 테스트·일괄 평가용)
//
// 사용법:
//   FPEvalCli --config cfg.json [--dll path] [--db dir] [--itype n] [--out dir] [--workers n]
//             [--timeout sec] [--seed n] [--pairs file]
//   FPEvalCli --resume <결과폴더> [--workers n]
//   FPEvalCli --report-only <결과폴더>
#include <windows.h>

#include <cstdio>
#include <string>

#include "Evaluator.h"
#include "PathUtil.h"

using namespace fpe;

namespace {

class ConsoleListener : public IProgressListener {
 public:
  void onPhase(const std::string& phase, size_t total) override {
    printf("\n== %s (%zu)\n", phase.c_str(), total);
    last_ = (size_t)-1;
  }
  void onProgress(size_t done, size_t total, size_t failures) override {
    size_t pct = total ? done * 100 / total : 100;
    if (pct / 10 != last_ || done == total) {
      printf("   %3zu%%  %zu/%zu  실패 %zu\n", pct, done, total, failures);
      last_ = pct / 10;
    }
  }
  void onLog(const std::string& line) override { printf("   %s\n", line.c_str()); }

 private:
  size_t last_ = (size_t)-1;
};

Evaluator* g_eval = nullptr;
BOOL WINAPI onCtrl(DWORD) {
  if (g_eval) {
    printf("\n강제 종료 요청...\n");
    g_eval->abort();
  }
  return TRUE;
}

void usage() {
  printf(
      "FPEvalCli --config cfg.json [--dll p] [--db d] [--itype n] [--out d] [--workers n] [--timeout s] [--seed n] "
      "[--pairs f]\n"
      "FPEvalCli --resume <run dir> [--workers n]\n"
      "FPEvalCli --report-only <run dir>\n");
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
  SetConsoleOutputCP(CP_UTF8);
  EvalConfig cfg;
  std::wstring resumeDir, reportDir;
  bool haveCfg = false;
  for (int i = 1; i < argc; ++i) {
    std::wstring a = argv[i];
    auto next = [&]() -> std::wstring { return i + 1 < argc ? argv[++i] : L""; };
    if (a == L"--config") {
      std::string err;
      if (!EvalConfig::load(next(), cfg, err)) {
        printf("config 오류: %s\n", err.c_str());
        return 2;
      }
      haveCfg = true;
    } else if (a == L"--dll") cfg.dllPath = next();
    else if (a == L"--db") cfg.dbPath = next();
    else if (a == L"--itype") cfg.iType = _wtoi(next().c_str());
    else if (a == L"--out") cfg.outputDir = next();
    else if (a == L"--workers") cfg.workers = _wtoi(next().c_str());
    else if (a == L"--timeout") cfg.timeoutSec = _wtoi(next().c_str());
    else if (a == L"--seed") cfg.seed = _wcstoui64(next().c_str(), nullptr, 10);
    else if (a == L"--pairs") cfg.pairsFile = next();
    else if (a == L"--resume") resumeDir = next();
    else if (a == L"--report-only") reportDir = next();
    else {
      usage();
      return 2;
    }
  }
  if (!haveCfg && resumeDir.empty() && reportDir.empty() && cfg.dllPath.empty()) {
    usage();
    return 2;
  }
  if (cfg.items.empty())
    for (auto& s : allReportItems()) cfg.items.insert(s);

  ConsoleListener l;
  RunOutcome out;
  if (!reportDir.empty()) {
    out = Evaluator::reportOnly(reportDir, nullptr, l);
  } else {
    Evaluator ev(cfg, l);
    g_eval = &ev;
    SetConsoleCtrlHandler(onCtrl, TRUE);
    out = ev.run(resumeDir);
    g_eval = nullptr;
  }
  printf("\n결과: %s\n", out.message.c_str());
  if (!out.runDir.empty()) printf("폴더: %s\n", toUtf8(out.runDir).c_str());
  if (!out.htmlPath.empty()) printf("HTML: %s\n", toUtf8(out.htmlPath).c_str());
  if (!out.docxPath.empty()) printf("DOCX: %s\n", toUtf8(out.docxPath).c_str());
  return out.ok ? 0 : (out.stopped ? 3 : 1);
}
