#include "Evaluator.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <vector>

#include "PathUtil.h"
#include "chart/ChartRenderer.h"
#include "chart/EmfCanvas.h"
#include "chart/SvgCanvas.h"
#include "db/FingerprintDB.h"
#include "db/TiffReader.h"
#include "exec/EvalScheduler.h"
#include "plan/EvalPlan.h"
#include "report/ReportModel.h"
#include "result/Csv.h"
#include "result/ResultStore.h"

namespace fpe {

namespace {

std::string fmtPct(double v, int prec = 2) {
  char b[48];
  snprintf(b, sizeof b, "%.*f%%", prec, v * 100);
  return b;
}
std::string fmtF(double v, int prec = 3) {
  char b[48];
  snprintf(b, sizeof b, "%.*f", prec, v);
  return b;
}
// 계산할 수 없는 지표(kNA)는 N/A로 표시한다
std::string pctOrNA(double v) { return isNA(v) ? std::string("N/A") : fmtPct(v); }

std::string imageKey(const FingerprintDB& db, int idx, bool anonymize) {
  if (!anonymize) return db.images()[idx].name();
  char b[16];
  snprintf(b, sizeof b, "%05d", idx);
  return b;
}

// 리포트·로그 폴더: 비어 있으면 결과 폴더, 지정하면 <폴더>/<실행 이름>
std::wstring subDirFor(const std::wstring& base, const std::wstring& runDir) {
  return base.empty() ? runDir : joinPath(base, fileNameOf(runDir));
}

// 진행 통지를 그대로 전달하면서 단계·로그 줄을 eval.log 파일에도 남긴다.
// 파일을 열기 전의 줄은 모아 두었다가 열 때 먼저 기록한다. 작업 스레드에서도 호출되므로 잠근다.
class LogTee : public IProgressListener {
 public:
  explicit LogTee(IProgressListener& inner) : inner_(inner) {}
  ~LogTee() override {
    if (f_) fclose(f_);
  }

  bool open(const std::wstring& path) {
    std::lock_guard<std::mutex> lk(mu_);
    if (f_) fclose(f_);
    f_ = _wfopen(path.c_str(), L"ab");
    if (!f_) return false;
    for (const auto& s : pending_) fprintf(f_, "%s\n", s.c_str());
    pending_.clear();
    fflush(f_);
    return true;
  }

  void onPhase(const std::string& phase, size_t total) override {
    write("== " + phase);
    inner_.onPhase(phase, total);
  }
  void onProgress(size_t done, size_t total, size_t failures) override { inner_.onProgress(done, total, failures); }
  void onLog(const std::string& line) override {
    write(line);
    inner_.onLog(line);
  }

 private:
  void write(const std::string& s) {
    SYSTEMTIME t;
    GetLocalTime(&t);
    char stamp[32];
    snprintf(stamp, sizeof stamp, "%04d-%02d-%02d %02d:%02d:%02d ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
             t.wSecond);
    std::lock_guard<std::mutex> lk(mu_);
    if (!f_) {
      pending_.push_back(stamp + s);
      return;
    }
    fprintf(f_, "%s%s\n", stamp, s.c_str());
    fflush(f_);
  }

  IProgressListener& inner_;
  std::mutex mu_;
  FILE* f_ = nullptr;
  std::vector<std::string> pending_;
};

bool parseJobId(const std::string& s, uint32_t& id) {
  if (s.empty() || s.size() > 10) return false;
  uint64_t v = 0;
  for (char c : s) {
    if (c < '0' || c > '9') return false;
    v = v * 10 + static_cast<uint64_t>(c - '0');
  }
  if (v == 0 || v > 0xFFFFFFFFull) return false;
  id = static_cast<uint32_t>(v);
  return true;
}

// plan.csv에서 계획 요약만 읽는다 (DB 없이 리포트 재생성용)
struct PlanSummary {
  bool found = false;
  size_t nG = 0, nI = 0, sameImageRemoved = 0;
  std::set<uint32_t> jobIds;
  std::string desc;
};

PlanSummary readPlanSummary(const std::wstring& path) {
  PlanSummary s;
  std::vector<uint8_t> buf;
  if (!readFile(path, buf)) return s;
  s.found = true;
  std::string text(buf.begin(), buf.end());
  size_t dp = text.find("# enroll=");
  if (dp != std::string::npos) s.desc = text.substr(dp + 2, text.find('\n', dp) - dp - 2);
  size_t rp = text.find("# same_image_removed=");
  if (rp != std::string::npos) s.sameImageRemoved = strtoull(text.c_str() + rp + 21, nullptr, 10);
  for (const auto& line : splitLines(text)) {
    if (line.empty() || line[0] == '#') continue;
    auto f = parseCsvLine(line);
    uint32_t id = 0;
    if (f.size() < 5 || !parseJobId(f[0], id)) continue;
    s.jobIds.insert(id);
    if (f[1] == "C") (f[4] == "G" ? s.nG : s.nI)++;
  }
  return s;
}

// 이어하기 무결성 확인용 지문: DLL 파일 해시, DB(이미지 이름 + 픽셀 해시) 해시
uint64_t dbFingerprint(const FingerprintDB& db, const std::vector<uint64_t>& pixelHash) {
  uint64_t h = kFnvOffset;
  for (size_t i = 0; i < pixelHash.size(); ++i) {
    std::string n = db.images()[i].name();
    h = hashBytes(n.data(), n.size(), h);
    h = hashBytes(&pixelHash[i], sizeof pixelHash[i], h);
  }
  return h;
}

// ---------------- 리포트 생성 ----------------
RunOutcome buildReport(const std::wstring& runDir, const EvalConfig& cfg, IProgressListener& l) {
  RunOutcome out;
  out.runDir = runDir;
  l.onPhase("STEP 5 지표·리포트", 1);

  if (!cfg.html && !cfg.docx) {
    out.message = "리포트 형식(HTML/Word)이 하나도 선택되지 않음";
    l.onLog("[오류] " + out.message);
    return out;
  }

  std::vector<ExtractRecord> ex;
  std::vector<MatchRecord> mt;
  std::string err;
  if (!ResultStore::load(runDir, ex, mt, err)) {
    out.message = err;
    return out;
  }
  PlanSummary plan = readPlanSummary(joinPath(runDir, L"plan.csv"));
  if (!plan.found) {
    out.message = "plan.csv를 읽을 수 없음";
    l.onLog("[오류] " + out.message);
    return out;
  }

  // 계획 대비 결과가 없는 작업: 중단된 평가에서 리포트만 만들면 부분 결과임을 표시한다
  std::set<uint32_t> doneIds;
  for (const auto& r : ex) {
    if (isFinal(r.status)) doneIds.insert(r.jobId);
  }
  for (const auto& r : mt) {
    if (isFinal(r.status)) doneIds.insert(r.jobId);
  }
  for (uint32_t id : plan.jobIds) {
    if (!doneIds.count(id)) ++out.incompleteJobs;
  }

  MetricOptions mo;
  mo.fvcEer = cfg.eerMethod != "Nearest";
  mo.predefinedDenominator = cfg.denominator != "attempted";
  MetricsResult m = MetricsCalculator::compute(ex, mt, plan.nG, plan.nI, mo);
  out.metrics = m;

  Json info;
  {
    std::vector<uint8_t> buf;
    if (readFile(joinPath(runDir, L"run_info.json"), buf)) Json::parse(std::string(buf.begin(), buf.end()), info, err);
  }
  std::string dbName = info.get("dbName").asString("DB");
  std::wstring repDir = subDirFor(cfg.reportDir, runDir);
  std::wstring logPath = joinPath(subDirFor(cfg.logDir, runDir), L"eval.log");

  ReportModel rm;
  rm.title = "지문인식 알고리즘 성능 평가 리포트 — " + dbName;
  rm.info = {
      {"알고리즘 DLL", info.get("dllName").asString(toUtf8(fileNameOf(cfg.dllPath))) + " (" +
                           std::to_string(info.get("dllBits").asInt()) + "bit)"},
      {"테스트 DB", toUtf8(cfg.dbPath)},
      {"이미지", std::to_string(info.get("images").asInt()) + "장 / 손가락 " +
                     std::to_string(info.get("fingers").asInt()) + "개 / " +
                     std::to_string(info.get("width").asInt()) + "x" + std::to_string(info.get("height").asInt()) +
                     " / iType " + std::to_string(cfg.iType)},
      {"평가 규칙", plan.desc},
      {"동일 이미지 비교 제외", std::to_string(plan.sameImageRemoved) + "건 (파일명은 다르지만 픽셀이 같은 이미지)"},
      {"seed", std::to_string(cfg.seed)},
      {"동시 수행 수 / timeout", std::to_string(cfg.workers) + " / " + std::to_string(cfg.timeoutSec) + "초"},
      {"EER 방식 / 분모", cfg.eerMethod + " / " + cfg.denominator},
      {"평가 시작", info.get("startedAt").asString()},
      {"진행 상태", out.incompleteJobs ? "미완료 " + std::to_string(out.incompleteJobs) + "건 / 계획 " +
                                             std::to_string(plan.jobIds.size()) + "건 — 중단된 평가의 부분 결과"
                                       : "완료 (계획 " + std::to_string(plan.jobIds.size()) + "건)"},
      {"결과 폴더", toUtf8(runDir)},
      {"로그 파일", toUtf8(logPath)},
  };
  if (out.incompleteJobs)
    rm.notes.push_back("평가가 끝나지 않은 상태의 리포트입니다. 결과가 없는 " + std::to_string(out.incompleteJobs) +
                       "건은 지표 계산에서 빠져 있으며, 결과 폴더에서 이어하기로 마칠 수 있습니다.");

  auto failBreakdown = [&]() {
    std::string s;
    for (auto& kv : m.enrollFailBy) s += std::string(s.empty() ? "" : ", ") + toString(kv.first) + " " + std::to_string(kv.second);
    return s.empty() ? std::string() : " — " + s;
  };
  auto timeStr = [](const TimeStat& t) {
    if (!t.n) return std::string("N/A (성공 건 없음)");
    return fmtF(t.meanMs, 2) + " ms (최대 " + fmtF(t.maxMs, 1) + " ms, n=" + std::to_string(t.n) + ")";
  };
  const std::string ratesWhy = "N/A — FMR·FNMR 분모가 0 (N_G=" + std::to_string(m.nG) + ", N_I=" + std::to_string(m.nI) + ")";
  auto rateItem = [&](double v) { return m.ratesDefined ? pctOrNA(v) : ratesWhy; };
  auto ratio = [](size_t a, size_t b) { return " (" + std::to_string(a) + "/" + std::to_string(b) + ")"; };

  if (cfg.hasItem(item::FTE)) {
    rm.summary.push_back({"FTE", (isNA(m.fte) ? std::string("N/A (등록 시도 없음)") : fmtPct(m.fte) + ratio(m.nEF, m.nEnroll)) +
                                     failBreakdown()});
    rm.summary.push_back({"비교용 추출 실패", m.nAttemptExtract ? fmtPct((double)m.nAF / m.nAttemptExtract) + ratio(m.nAF, m.nAttemptExtract)
                                                               : std::string("N/A (추출 시도 없음)")});
    if (m.nInternal)
      rm.summary.push_back({"평가 프로그램 오류", std::to_string(m.nInternal) + "건 (INTERNAL) — 알고리즘 실패가 아니므로 FTE·REJ에서 제외"});
  }
  if (cfg.hasItem(item::REJ)) {
    rm.summary.push_back({"REJ genuine", isNA(m.rejG) ? std::string("N/A (genuine 비교 호출 없음)") : fmtPct(m.rejG) + ratio(m.nGCallFail, m.nGCalled)});
    rm.summary.push_back({"REJ impostor", isNA(m.rejI) ? std::string("N/A (impostor 비교 호출 없음)") : fmtPct(m.rejI) + ratio(m.nICallFail, m.nICalled)});
  }
  if (cfg.hasItem(item::EER)) {
    rm.summary.push_back({"EER", m.ratesDefined ? fmtPct(m.eer) + (m.eerExact ? " (정확, t=" : " (추정, t=") + fmtF(m.eerT) +
                                                      ", 구간 " + fmtPct(m.eerLow) + " ~ " + fmtPct(m.eerHigh) + ")"
                                                : ratesWhy});
  }
  if (cfg.hasItem(item::EERstar)) {
    rm.summary.push_back({"EER*", m.starDefined ? fmtPct(m.eerStar) + " (t=" + fmtF(m.eerStarT) + ")"
                                                : "N/A — 성공 점수 genuine " + std::to_string(m.gOk) + "건 / impostor " +
                                                      std::to_string(m.iOk) + "건"});
  }
  if (cfg.hasItem(item::ZeroFMR)) rm.summary.push_back({"ZeroFMR", rateItem(m.zeroFMR)});
  if (cfg.hasItem(item::ZeroFNMR)) {
    rm.summary.push_back({"ZeroFNMR", !m.ratesDefined ? ratesWhy
                                      : isNA(m.zeroFNMR) ? std::string("정의 불가 (FNMR=0이 되는 임계값 없음)")
                                                         : fmtPct(m.zeroFNMR)});
  }
  if (cfg.hasItem(item::FMR100)) rm.summary.push_back({"FMR100", rateItem(m.fmr100)});
  if (cfg.hasItem(item::FMR1000)) rm.summary.push_back({"FMR1000", rateItem(m.fmr1000)});
  if (cfg.hasItem(item::FMR10000)) rm.summary.push_back({"FMR10000", rateItem(m.fmr10000)});
  if (cfg.hasItem(item::Time)) {
    rm.summary.push_back({"Enrollment time", timeStr(m.enrollTime)});
    rm.summary.push_back({"Matching time (전체)", timeStr(m.matchAll)});
    rm.summary.push_back({"Matching time (genuine)", timeStr(m.matchG)});
    rm.summary.push_back({"Matching time (impostor)", timeStr(m.matchI)});
  }
  if (cfg.hasItem(item::Size)) {
    rm.summary.push_back({"Template size", m.enrollTime.n ? "평균 " + fmtF(m.tplMean, 1) + " B / 최대 " + fmtF(m.tplMax, 0) + " B"
                                                         : std::string("N/A (등록 성공 없음)")});
  }

  if (cfg.hasItem(item::REJ) || cfg.hasItem(item::FTE)) {
    rm.counts = {{"구분", "계획(N)", "등록 실패(EF)", "비교 실패(MF)", "DLL 호출", "호출 실패", "성공(OK)"},
                 {"Genuine", std::to_string(m.nG), std::to_string(m.nGEF), std::to_string(m.nGMF),
                  std::to_string(m.nGCalled), std::to_string(m.nGCallFail), std::to_string(m.gOk)},
                 {"Impostor", std::to_string(m.nI), std::to_string(m.nIEF), std::to_string(m.nIMF),
                  std::to_string(m.nICalled), std::to_string(m.nICallFail), std::to_string(m.iOk)}};
  }

  std::vector<std::string> writeErrors;
  std::wstring figDir = joinPath(repDir, L"figures");
  if (!makeDirs(figDir)) {
    out.message = "리포트 폴더를 만들 수 없음: " + toUtf8(figDir);
    l.onLog("[오류] " + out.message);
    return out;
  }

  // 그래프: SVG(HTML용) + EMF(Word용) 동시 출력. FMR·FNMR이 정의되지 않으면 곡선 그래프는 생략한다.
  ChartRenderer cr(fromUtf8(dbName));
  struct FigDef {
    const char* item;
    const char* id;
    const char* caption;
    bool needsRates;
    void (ChartRenderer::*draw)(ICanvas&, const MetricsResult&) const;
  };
  const FigDef defs[] = {{item::DET, "det", "DET Curve", true, &ChartRenderer::drawDET},
                         {item::Dist, "score_dist", "Score Distribution", false, &ChartRenderer::drawDistribution},
                         {item::Rates, "fmr_fnmr", "FMR(t) and FNMR(t)", true, &ChartRenderer::drawRates}};
  for (const auto& d : defs) {
    if (!cfg.hasItem(d.item)) continue;
    if (d.needsRates && !m.ratesDefined) {
      rm.notes.push_back(std::string(d.caption) + " 그래프는 genuine 또는 impostor 계획 시도가 없어 그릴 수 없으므로 생략했습니다.");
      continue;
    }
    ReportFigure f;
    f.id = d.id;
    f.caption = d.caption;
    f.svgPath = joinPath(figDir, fromUtf8(std::string(d.id) + ".svg"));
    f.emfPath = joinPath(figDir, fromUtf8(std::string(d.id) + ".emf"));
    f.svgRel = std::string("figures/") + d.id + ".svg";
    SvgCanvas svg(f.svgPath, ChartRenderer::W, ChartRenderer::H);
    (cr.*d.draw)(svg, m);
    bool ok = svg.finish();
    EmfCanvas emf(f.emfPath, ChartRenderer::W, ChartRenderer::H);
    (cr.*d.draw)(emf, m);
    ok = emf.finish() && ok;
    if (!ok) {
      writeErrors.push_back("그래프 저장 실패: " + std::string(d.id));
      continue;
    }
    rm.figures.push_back(f);
  }

  rm.showErrors = cfg.hasItem(item::Errors);
  for (const auto& r : ex) {
    if (r.status != Status::OK && r.status != Status::Aborted) {
      ++rm.totalErrors;
      if (rm.errors.size() < 50)
        rm.errors.push_back({std::string(r.kind == 'E' ? "등록 " : "추출 ") + r.image, toString(r.status), r.errMsg, r.errCode});
    }
  }
  for (const auto& r : mt) {
    if (isCallFailure(r.status) || r.status == Status::InternalError) {
      ++rm.totalErrors;
      if (rm.errors.size() < 50)
        rm.errors.push_back({std::string("비교 ") + r.enroll + " vs " + r.attempt, toString(r.status), r.errMsg, r.errCode});
    }
  }
  if (rm.summary.empty() && rm.figures.empty() && !rm.showErrors)
    l.onLog("[경고] 선택된 리포트 항목이 없어 평가 환경만 기록합니다");

  if (cfg.html) {
    std::wstring p = joinPath(repDir, L"report.html");
    HtmlReportWriter w;
    if (w.write(rm, p, err)) {
      out.htmlPath = p;
    } else {
      writeErrors.push_back(err);
    }
  }
  if (cfg.docx) {
    std::wstring p = joinPath(repDir, L"report.docx");
    DocxReportWriter w;
    if (w.write(rm, p, err)) {
      out.docxPath = p;
    } else {
      writeErrors.push_back(err);
    }
  }
  l.onProgress(1, 1, 0);
  l.onLog("EER " + pctOrNA(m.eer) + ", EER* " + pctOrNA(m.eerStar) + ", FTE " + pctOrNA(m.fte) + ", FMR100 " +
          pctOrNA(m.fmr100) + ", FMR1000 " + pctOrNA(m.fmr1000));
  if (out.incompleteJobs) l.onLog("[경고] 미완료 작업 " + std::to_string(out.incompleteJobs) + "건 — 부분 결과 리포트");

  if (!writeErrors.empty()) {
    out.message = "리포트 저장 실패: " + writeErrors.front();
    for (auto& e : writeErrors) l.onLog("[오류] " + e);
    return out;
  }
  out.ok = true;
  out.message = out.incompleteJobs ? "리포트 생성 (미완료 " + std::to_string(out.incompleteJobs) + "건)" : "완료";
  return out;
}

}  // namespace

Evaluator::Evaluator(EvalConfig cfg, IProgressListener& l) : cfg_(std::move(cfg)), l_(l) {}
Evaluator::~Evaluator() = default;

void Evaluator::requestStop() {
  stop_ = true;
  std::lock_guard<std::mutex> lk(mu_);
  if (sched_) sched_->requestStop();
}

void Evaluator::abort() {
  stop_ = abort_ = true;
  std::lock_guard<std::mutex> lk(mu_);
  if (sched_) sched_->abort();
}

RunOutcome Evaluator::reportOnly(const std::wstring& runDir, const EvalConfig* reportCfg, IProgressListener& l) {
  EvalConfig cfg;
  std::string err;
  if (!EvalConfig::load(joinPath(runDir, L"config.json"), cfg, err)) {
    RunOutcome o;
    o.message = "config.json: " + err;
    return o;
  }
  if (reportCfg) {
    cfg.items = reportCfg->items;
    cfg.html = reportCfg->html;
    cfg.docx = reportCfg->docx;
    cfg.eerMethod = reportCfg->eerMethod;
    cfg.denominator = reportCfg->denominator;
  }
  LogTee tee(l);
  std::wstring logDir = subDirFor(cfg.logDir, runDir);
  if (makeDirs(logDir)) tee.open(joinPath(logDir, L"eval.log"));
  tee.onLog("리포트만 생성: " + toUtf8(runDir));
  RunOutcome o = buildReport(runDir, cfg, tee);
  o.logPath = joinPath(logDir, L"eval.log");
  return o;
}

RunOutcome Evaluator::run(const std::wstring& resumeDir) {
  RunOutcome out;
  LogTee tee(l_);
  auto fail = [&](const std::string& msg) {
    out.message = msg;
    tee.onLog("[오류] " + msg);
    return out;
  };
  const bool resume = !resumeDir.empty();
  std::string err;

  // ---------------- STEP 0 사전 검증 ----------------
  tee.onPhase("STEP 0 사전 검증", 1);
  std::wstring runDir;
  if (resume) {
    EvalConfig saved;
    if (!EvalConfig::load(joinPath(resumeDir, L"config.json"), saved, err)) return fail("이어하기: config.json을 읽을 수 없음 — " + err);
    saved.workers = cfg_.workers;  // 실행 옵션은 현재 값 사용
    saved.timeoutSec = cfg_.timeoutSec;
    cfg_ = saved;
    runDir = resumeDir;
    tee.onLog("이어하기: " + toUtf8(runDir));
  }
  if (!cfg_.html && !cfg_.docx) return fail("리포트 형식(HTML/Word)을 하나 이상 선택하세요");
  if (!fileExists(cfg_.dllPath)) return fail("DLL 파일이 없음: " + toUtf8(cfg_.dllPath));
  int bits = peBitness(cfg_.dllPath);
  if (!bits) return fail("Windows DLL이 아니거나 지원하지 않는 플랫폼");
  std::wstring workerExe = joinPath(exeDir(), bits == 32 ? L"FPWorker32.exe" : L"FPWorker64.exe");
  if (!fileExists(workerExe)) return fail("Worker 실행 파일이 없음: " + toUtf8(workerExe));
  uint64_t dllHash = 0;
  if (!hashFile(cfg_.dllPath, dllHash)) return fail("DLL 파일을 읽을 수 없음: " + toUtf8(cfg_.dllPath));
  tee.onLog("DLL " + toUtf8(fileNameOf(cfg_.dllPath)) + " — " + std::to_string(bits) + "bit, Worker " +
            toUtf8(fileNameOf(workerExe)) + ", hash " + hashHex(dllHash));

  FingerprintDB db;
  if (!db.scan(cfg_.dbPath, err)) return fail(err);
  for (auto& w : db.warnings()) tee.onLog("[경고] " + w);
  tee.onLog("DB: 이미지 " + std::to_string(db.images().size()) + "장, 손가락 " + std::to_string(db.fingers().size()) + "개");

  if (!resume) {
    if (cfg_.outputDir.empty()) return fail("결과 폴더를 지정하세요");
    // 같은 초에 시작한 평가끼리 폴더가 겹치지 않도록 이미 있으면 _2, _3 … 을 붙인다
    std::wstring base = joinPath(cfg_.outputDir, fileNameOf(cfg_.dbPath) + L"_" + nowStamp());
    runDir = base;
    for (int k = 2; dirExists(runDir); ++k) runDir = base + L"_" + std::to_wstring(k);
  }
  if (!makeDirs(runDir)) return fail("결과 폴더를 만들 수 없음: " + toUtf8(runDir));
  out.runDir = runDir;
  std::wstring logDir = subDirFor(cfg_.logDir, runDir);
  out.logPath = joinPath(logDir, L"eval.log");
  if (!makeDirs(logDir) || !tee.open(out.logPath)) return fail("로그 파일을 만들 수 없음: " + toUtf8(out.logPath));

  EvalScheduler::Options so;
  so.workerExe = workerExe;
  so.dllPath = cfg_.dllPath;
  so.workers = std::max(1, cfg_.workers);
  so.timeoutMs = (DWORD)std::max(1, cfg_.timeoutSec) * 1000;
  EvalScheduler sched(so);
  {
    std::lock_guard<std::mutex> lk(mu_);
    sched_ = &sched;
  }
  struct Clear {
    Evaluator* e;
    ~Clear() {
      std::lock_guard<std::mutex> lk(e->mu_);
      e->sched_ = nullptr;
    }
  } clear{this};
  if (stop_) sched.requestStop();
  if (abort_) sched.abort();

  if (!sched.probe(err)) return fail("DLL 로드 실패: " + err);
  tee.onLog("KISA_Enroll / KISA_Attempt / KISA_Compare 확인");
  tee.onProgress(1, 1, 0);

  // ---------------- STEP 1 이미지 캐시 ----------------
  std::wstring cacheDir = joinPath(runDir, L"cache");
  if (!makeDirs(cacheDir)) return fail("캐시 폴더를 만들 수 없음: " + toUtf8(cacheDir));
  std::wstring tplDir = cfg_.templateDir.empty() ? joinPath(runDir, L"templates")
                                                 : joinPath(cfg_.templateDir, fileNameOf(runDir));
  if (!makeDirs(tplDir)) return fail("템플릿 폴더를 만들 수 없음: " + toUtf8(tplDir));
  const auto& imgs = db.images();
  tee.onPhase("STEP 1 이미지 캐시", imgs.size());
  int W = 0, H = 0;
  std::vector<std::wstring> rawPath(imgs.size());
  std::vector<uint64_t> pixelHash(imgs.size());
  for (size_t i = 0; i < imgs.size(); ++i) {
    if (stop_) return fail("중지됨");
    rawPath[i] = joinPath(cacheDir, fromUtf8(imageKey(db, (int)i, cfg_.anonymize)) + L".raw");
    GrayImage g;
    if (!readTiff(imgs[i].path, g, err)) return fail(imgs[i].name() + ": " + err);
    if (i == 0) {
      W = g.width;
      H = g.height;
    } else if (g.width != W || g.height != H) {
      return fail(imgs[i].name() + ": 이미지 크기가 다름 (" + std::to_string(g.width) + "x" + std::to_string(g.height) + ")");
    }
    int dims[2] = {g.width, g.height};
    pixelHash[i] = hashBytes(g.pixels.data(), g.pixels.size(), hashBytes(dims, sizeof dims));
    if (!fileExists(rawPath[i]) && !writeFile(rawPath[i], g.pixels.data(), g.pixels.size()))
      return fail("캐시 쓰기 실패: " + toUtf8(rawPath[i]));
    if (i % 10 == 0 || i + 1 == imgs.size()) tee.onProgress(i + 1, imgs.size(), 0);
  }
  tee.onLog("이미지 크기 " + std::to_string(W) + "x" + std::to_string(H) + ", iType " + std::to_string(cfg_.iType));
  {
    std::map<uint64_t, std::vector<int>> groups;
    for (size_t i = 0; i < pixelHash.size(); ++i) groups[pixelHash[i]].push_back((int)i);
    size_t dupImages = 0;
    for (auto& kv : groups) {
      if (kv.second.size() < 2) continue;
      dupImages += kv.second.size();
      std::string names;
      for (int k : kv.second) names += (names.empty() ? "" : " = ") + imgs[k].name();
      tee.onLog("[경고] 픽셀이 같은 이미지: " + names + " — 서로 비교하지 않음");
    }
    if (dupImages) tee.onLog("픽셀이 같은 이미지 " + std::to_string(dupImages) + "장 발견");
  }
  const uint64_t dbHash = dbFingerprint(db, pixelHash);

  // ---------------- STEP 2 평가 계획 ----------------
  tee.onPhase("STEP 2 평가 계획", 1);
  EvalPlan plan;
  std::wstring planPath = joinPath(runDir, L"plan.csv");
  std::wstring infoPath = joinPath(runDir, L"run_info.json");
  if (resume) {
    // 이어하기 전에 DLL·DB가 처음 실행 때와 같은지 확인 (다르면 기존 결과와 섞이므로 중단)
    std::vector<uint8_t> buf;
    Json info;
    if (!readFile(infoPath, buf) || !Json::parse(std::string(buf.begin(), buf.end()), info, err))
      return fail("이어하기: run_info.json을 읽을 수 없음");
    std::string savedDll = info.get("dllHash").asString(), savedDb = info.get("dbHash").asString();
    if (savedDll.empty() || savedDb.empty()) {
      tee.onLog("[경고] 이전 버전 결과 폴더라 DLL·DB 동일성을 확인할 수 없음");
    } else {
      if (savedDll != hashHex(dllHash))
        return fail("이어하기 불가: DLL이 처음 실행 때와 다름 (저장 " + savedDll + ", 현재 " + hashHex(dllHash) + ")");
      if (savedDb != hashHex(dbHash))
        return fail("이어하기 불가: DB 이미지가 처음 실행 때와 다름 (저장 " + savedDb + ", 현재 " + hashHex(dbHash) + ")");
      tee.onLog("DLL·DB가 처음 실행 때와 같음을 확인");
    }
    if (!EvalPlan::load(planPath, db, plan, err, &pixelHash)) return fail("이어하기: " + err);
  } else {
    std::vector<std::string> warns;
    if (!EvalPlan::build(cfg_, db, plan, warns, err, &pixelHash)) return fail(err);
    for (size_t k = 0; k < warns.size() && k < 30; ++k) tee.onLog("[경고] " + warns[k]);
    if (warns.size() > 30) tee.onLog("[경고] … 외 " + std::to_string(warns.size() - 30) + "건");
    if (!plan.save(planPath, db)) return fail("plan.csv 저장 실패: " + toUtf8(planPath));
    if (!plan.savePairs(runDir, db)) return fail("pairs_*.csv 저장 실패: " + toUtf8(runDir));
    if (!cfg_.save(joinPath(runDir, L"config.json"))) return fail("config.json 저장 실패");
    Json info = Json::object();
    info["dllName"] = toUtf8(fileNameOf(cfg_.dllPath));
    info["dllBits"] = bits;
    info["dllHash"] = hashHex(dllHash);
    info["dbName"] = toUtf8(fileNameOf(cfg_.dbPath));
    info["dbHash"] = hashHex(dbHash);
    info["images"] = (int)imgs.size();
    info["fingers"] = (int)db.fingers().size();
    info["width"] = W;
    info["height"] = H;
    info["startedAt"] = toUtf8(nowStamp());
    if (!writeText(infoPath, info.dump())) return fail("run_info.json 저장 실패");
  }
  tee.onLog("계획: 등록 " + std::to_string(plan.nEnroll) + ", 비교용 추출 " + std::to_string(plan.nAttempt) + ", 본인비교 " +
            std::to_string(plan.nGenuine) + ", 타인비교 " + std::to_string(plan.nImpostor) + " (seed " +
            std::to_string(plan.seed) + ", 동일 이미지 비교 제외 " + std::to_string(plan.nSameImageRemoved) + "건)");
  tee.onProgress(1, 1, 0);

  ResultStore store;
  if (!store.open(runDir, err)) return fail(err);
  std::set<uint32_t> doneIds;
  std::map<int, Status> enrollSt, attemptSt;  // 이미지별 최종 추출 상태 (없으면 아직 끝나지 않음)
  if (resume) {
    std::vector<ExtractRecord> ex;
    std::vector<MatchRecord> mt;
    if (!ResultStore::load(runDir, ex, mt, err)) return fail("이어하기: 기존 결과를 읽을 수 없음 — " + err);
    for (auto& r : ex) {
      if (!isFinal(r.status)) continue;
      doneIds.insert(r.jobId);
      int idx = db.find(r.image);
      if (idx >= 0) (r.kind == 'E' ? enrollSt : attemptSt)[idx] = r.status;
    }
    for (auto& r : mt) {
      if (isFinal(r.status)) doneIds.insert(r.jobId);
    }
    tee.onLog("기존 결과 " + std::to_string(doneIds.size()) + "건은 건너뜀");
  }
  std::mutex stMu;

  auto tplPath = [&](char kind, int img) {
    return joinPath(tplDir, fromUtf8(std::string(1, kind) + "_" + imageKey(db, img, cfg_.anonymize) + ".tpl"));
  };

  // ---------------- STEP 3 특징 추출 ----------------
  std::vector<Job> pending;
  for (auto& j : plan.extractJobs) {
    if (!doneIds.count(j.id)) pending.push_back(j);
  }
  tee.onPhase("STEP 3 특징 추출 (Enroll/Attempt)", pending.size());
  std::atomic<size_t> done{0}, fails{0};
  auto makeExtract = [&](size_t i) {
    const Job& j = pending[i];
    WorkItem w;
    w.jobId = j.id;
    w.type = j.kind == JobKind::Enroll ? MsgType::Enroll : MsgType::Attempt;
    ExtractReq req{};
    req.iType = cfg_.iType;
    req.width = W;
    req.height = H;
    wcsncpy_s(req.rawPath, rawPath[j.a].c_str(), _TRUNCATE);
    wcsncpy_s(req.outPath, tplPath((char)j.kind, j.a).c_str(), _TRUNCATE);
    w.body.resize(sizeof req);
    memcpy(w.body.data(), &req, sizeof req);
    return w;
  };
  auto doneExtract = [&](size_t i, const WorkResult& r) {
    if (r.status == Status::Aborted) return;
    const Job& j = pending[i];
    ExtractRecord rec;
    rec.jobId = j.id;
    rec.kind = (char)j.kind;
    rec.image = imgs[j.a].name();
    rec.status = r.status;
    rec.retVal = r.reply.retVal;
    rec.errCode = r.reply.errCode;
    rec.errMsg = acpToUtf8(r.reply.errMsg);
    rec.elapsedUs = r.reply.elapsedUs;
    rec.size = r.reply.outSize;
    rec.sehCode = r.reply.sehCode;
    store.append(rec);
    {
      std::lock_guard<std::mutex> lk(stMu);
      (j.kind == JobKind::Enroll ? enrollSt : attemptSt)[j.a] = r.status;
    }
    if (r.status != Status::OK) {
      ++fails;
      tee.onLog(std::string("[") + toString(r.status) + "] " + (j.kind == JobKind::Enroll ? "등록 " : "추출 ") + rec.image +
                (rec.errMsg.empty() ? "" : " — " + rec.errMsg));
    }
    size_t d = ++done;
    if (d % 10 == 0 || d == pending.size()) tee.onProgress(d, pending.size(), fails);
  };
  if (!sched.run(pending.size(), makeExtract, doneExtract, err)) return fail("평가 중단: " + err);
  if (sched.stopRequested()) {
    out.stopped = true;
    out.message = "중지됨 — 결과 폴더에서 이어하기 가능";
    tee.onLog(out.message);
    return out;
  }

  // ---------------- STEP 4 비교 ----------------
  std::vector<Job> cmp;
  size_t skipped = 0, held = 0;
  for (auto& j : plan.compareJobs) {
    if (doneIds.count(j.id)) continue;
    auto e = enrollSt.find(j.a);
    auto a = attemptSt.find(j.b);
    // 선행 추출이 평가 프로그램 오류(INTERNAL) 등으로 끝나지 않았으면 기록하지 않고 이어하기로 넘긴다
    if (e == enrollSt.end() || !isFinal(e->second) || a == attemptSt.end() || !isFinal(a->second)) {
      ++held;
      continue;
    }
    Status skip = Status::OK;
    if (e->second != Status::OK) {
      skip = Status::SkipEF;
    } else if (a->second != Status::OK) {
      skip = Status::SkipAF;
    }
    if (skip != Status::OK) {  // 템플릿이 없으면 DLL을 호출하지 않고 기록
      MatchRecord rec;
      rec.jobId = j.id;
      rec.enroll = imgs[j.a].name();
      rec.attempt = imgs[j.b].name();
      rec.type = j.type;
      rec.status = skip;
      store.append(rec);
      ++skipped;
    } else {
      cmp.push_back(j);
    }
  }
  if (skipped) tee.onLog("템플릿이 없어 건너뛴 비교 " + std::to_string(skipped) + "건 (SKIP_EF/SKIP_AF)");
  if (held) tee.onLog("[경고] 선행 추출이 끝나지 않아 보류한 비교 " + std::to_string(held) + "건 — 이어하기로 다시 실행");
  tee.onPhase("STEP 4 비교 (Compare)", cmp.size());
  done = 0;
  fails = 0;
  auto makeCompare = [&](size_t i) {
    const Job& j = cmp[i];
    WorkItem w;
    w.jobId = j.id;
    w.type = MsgType::Compare;
    CompareReq req{};
    wcsncpy_s(req.enrollPath, tplPath('E', j.a).c_str(), _TRUNCATE);
    wcsncpy_s(req.attemptPath, tplPath('A', j.b).c_str(), _TRUNCATE);
    w.body.resize(sizeof req);
    memcpy(w.body.data(), &req, sizeof req);
    return w;
  };
  auto doneCompare = [&](size_t i, const WorkResult& r) {
    if (r.status == Status::Aborted) return;
    const Job& j = cmp[i];
    MatchRecord rec;
    rec.jobId = j.id;
    rec.enroll = imgs[j.a].name();
    rec.attempt = imgs[j.b].name();
    rec.type = j.type;
    rec.status = r.status;
    rec.score = r.reply.score;
    rec.errCode = r.reply.errCode;
    rec.errMsg = acpToUtf8(r.reply.errMsg);
    rec.elapsedUs = r.reply.elapsedUs;
    rec.sehCode = r.reply.sehCode;
    store.append(rec);
    if (r.status != Status::OK) {
      ++fails;
      tee.onLog(std::string("[") + toString(r.status) + "] 비교 " + rec.enroll + " vs " + rec.attempt +
                (rec.errMsg.empty() ? "" : " — " + rec.errMsg));
    }
    size_t d = ++done;
    if (d % 20 == 0 || d == cmp.size()) tee.onProgress(d, cmp.size(), fails);
  };
  if (!sched.run(cmp.size(), makeCompare, doneCompare, err)) return fail("평가 중단: " + err);
  if (sched.stopRequested()) {
    out.stopped = true;
    out.message = "중지됨 — 결과 폴더에서 이어하기 가능";
    tee.onLog(out.message);
    return out;
  }
  store.close();

  // ---------------- STEP 5 지표·리포트 ----------------
  RunOutcome rep = buildReport(runDir, cfg_, tee);
  rep.runDir = runDir;
  rep.logPath = out.logPath;
  return rep;
}

}  // namespace fpe
