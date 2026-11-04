#include "Evaluator.h"

#include <cstring>
#include <map>
#include <set>

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

std::string imageKey(const FingerprintDB& db, int idx, bool anonymize) {
  if (!anonymize) return db.images()[idx].name();
  char b[16];
  snprintf(b, sizeof b, "%05d", idx);
  return b;
}

// plan.csv에서 계획된 genuine/impostor 수와 규칙 설명만 읽는다 (DB 없이 리포트 재생성용)
void readPlanSummary(const std::wstring& path, size_t& nG, size_t& nI, std::string& desc) {
  nG = nI = 0;
  std::vector<uint8_t> buf;
  if (!readFile(path, buf)) return;
  std::string text(buf.begin(), buf.end());
  size_t dp = text.find("# enroll=");
  if (dp != std::string::npos) desc = text.substr(dp + 2, text.find('\n', dp) - dp - 2);
  for (const auto& line : splitLines(text)) {
    auto f = parseCsvLine(line);
    if (f.size() >= 5 && f[1] == "C") (f[4] == "G" ? nG : nI)++;
  }
}

// ---------------- 리포트 생성 ----------------
RunOutcome buildReport(const std::wstring& runDir, const EvalConfig& cfg, IProgressListener& l) {
  RunOutcome out;
  out.runDir = runDir;
  l.onPhase("STEP 5 지표·리포트", 1);

  std::vector<ExtractRecord> ex;
  std::vector<MatchRecord> mt;
  std::string err;
  if (!ResultStore::load(runDir, ex, mt, err)) {
    out.message = err;
    return out;
  }
  size_t nG = 0, nI = 0;
  std::string planDesc;
  readPlanSummary(joinPath(runDir, L"plan.csv"), nG, nI, planDesc);

  MetricOptions mo;
  mo.fvcEer = cfg.eerMethod != "Nearest";
  mo.predefinedDenominator = cfg.denominator != "attempted";
  MetricsResult m = MetricsCalculator::compute(ex, mt, nG, nI, mo);
  out.metrics = m;

  Json info;
  {
    std::vector<uint8_t> buf;
    if (readFile(joinPath(runDir, L"run_info.json"), buf)) Json::parse(std::string(buf.begin(), buf.end()), info, err);
  }
  std::string dbName = info.get("dbName").asString("DB");

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
      {"평가 규칙", planDesc},
      {"seed", std::to_string(cfg.seed)},
      {"동시 수행 수 / timeout", std::to_string(cfg.workers) + " / " + std::to_string(cfg.timeoutSec) + "초"},
      {"EER 방식 / 분모", cfg.eerMethod + " / " + cfg.denominator},
      {"평가 시작", info.get("startedAt").asString()},
  };

  auto failBreakdown = [&]() {
    std::string s;
    for (auto& kv : m.enrollFailBy) s += std::string(s.empty() ? "" : ", ") + toString(kv.first) + " " + std::to_string(kv.second);
    return s.empty() ? std::string() : " — " + s;
  };
  auto timeStr = [](const TimeStat& t) { return fmtF(t.meanMs, 2) + " ms (최대 " + fmtF(t.maxMs, 1) + " ms, n=" + std::to_string(t.n) + ")"; };
  auto na = [&](double v) { return v < 0 ? std::string("정의 불가") : fmtPct(v); };

  if (cfg.hasItem(item::FTE)) {
    rm.summary.push_back({"FTE", fmtPct(m.fte) + " (" + std::to_string(m.nEF) + "/" + std::to_string(m.nEnroll) + ")" + failBreakdown()});
    rm.summary.push_back({"비교용 추출 실패", fmtPct(m.nAttemptExtract ? (double)m.nAF / m.nAttemptExtract : 0) + " (" +
                                         std::to_string(m.nAF) + "/" + std::to_string(m.nAttemptExtract) + ")"});
  }
  if (cfg.hasItem(item::REJ)) {
    rm.summary.push_back({"REJ genuine", fmtPct(m.rejG) + " (" + std::to_string(m.nGCallFail) + "/" + std::to_string(m.nGCalled) + ")"});
    rm.summary.push_back({"REJ impostor", fmtPct(m.rejI) + " (" + std::to_string(m.nICallFail) + "/" + std::to_string(m.nICalled) + ")"});
  }
  if (cfg.hasItem(item::EER))
    rm.summary.push_back({"EER", fmtPct(m.eer) + (m.eerExact ? " (정확, t=" : " (추정, t=") + fmtF(m.eerT) + ", 구간 " +
                                     fmtPct(m.eerLow) + " ~ " + fmtPct(m.eerHigh) + ")"});
  if (cfg.hasItem(item::EERstar)) rm.summary.push_back({"EER*", fmtPct(m.eerStar) + " (t=" + fmtF(m.eerStarT) + ")"});
  if (cfg.hasItem(item::ZeroFMR)) rm.summary.push_back({"ZeroFMR", fmtPct(m.zeroFMR)});
  if (cfg.hasItem(item::ZeroFNMR)) rm.summary.push_back({"ZeroFNMR", na(m.zeroFNMR)});
  if (cfg.hasItem(item::FMR100)) rm.summary.push_back({"FMR100", fmtPct(m.fmr100)});
  if (cfg.hasItem(item::FMR1000)) rm.summary.push_back({"FMR1000", fmtPct(m.fmr1000)});
  if (cfg.hasItem(item::Time)) {
    rm.summary.push_back({"Enrollment time", timeStr(m.enrollTime)});
    rm.summary.push_back({"Matching time (전체)", timeStr(m.matchAll)});
    rm.summary.push_back({"Matching time (genuine)", timeStr(m.matchG)});
    rm.summary.push_back({"Matching time (impostor)", timeStr(m.matchI)});
  }
  if (cfg.hasItem(item::Size))
    rm.summary.push_back({"Template size", "평균 " + fmtF(m.tplMean, 1) + " B / 최대 " + fmtF(m.tplMax, 0) + " B"});

  if (cfg.hasItem(item::REJ) || cfg.hasItem(item::FTE)) {
    rm.counts = {{"구분", "계획(N)", "등록 실패(EF)", "비교 실패(MF)", "DLL 호출", "호출 실패", "성공(OK)"},
                 {"Genuine", std::to_string(m.nG), std::to_string(m.nGEF), std::to_string(m.nGMF),
                  std::to_string(m.nGCalled), std::to_string(m.nGCallFail), std::to_string(m.gOk)},
                 {"Impostor", std::to_string(m.nI), std::to_string(m.nIEF), std::to_string(m.nIMF),
                  std::to_string(m.nICalled), std::to_string(m.nICallFail), std::to_string(m.iOk)}};
  }

  // 그래프: SVG(HTML용) + EMF(Word용) 동시 출력
  std::wstring figDir = joinPath(runDir, L"figures");
  makeDirs(figDir);
  ChartRenderer cr(fromUtf8(dbName));
  struct FigDef {
    const char* item;
    const char* id;
    const char* caption;
    void (ChartRenderer::*draw)(ICanvas&, const MetricsResult&) const;
  };
  const FigDef defs[] = {{item::DET, "det", "DET Curve", &ChartRenderer::drawDET},
                         {item::Dist, "score_dist", "Score Distribution", &ChartRenderer::drawDistribution},
                         {item::Rates, "fmr_fnmr", "FMR(t) and FNMR(t)", &ChartRenderer::drawRates}};
  for (const auto& d : defs) {
    if (!cfg.hasItem(d.item)) continue;
    ReportFigure f;
    f.id = d.id;
    f.caption = d.caption;
    f.svgPath = joinPath(figDir, fromUtf8(std::string(d.id) + ".svg"));
    f.emfPath = joinPath(figDir, fromUtf8(std::string(d.id) + ".emf"));
    f.svgRel = std::string("figures/") + d.id + ".svg";
    SvgCanvas svg(f.svgPath, ChartRenderer::W, ChartRenderer::H);
    (cr.*d.draw)(svg, m);
    svg.finish();
    EmfCanvas emf(f.emfPath, ChartRenderer::W, ChartRenderer::H);
    (cr.*d.draw)(emf, m);
    emf.finish();
    rm.figures.push_back(f);
  }

  rm.showErrors = cfg.hasItem(item::Errors);
  for (const auto& r : ex)
    if (r.status != Status::OK && r.status != Status::Aborted) {
      ++rm.totalErrors;
      if (rm.errors.size() < 50)
        rm.errors.push_back({std::string(r.kind == 'E' ? "등록 " : "추출 ") + r.image, toString(r.status), r.errMsg, r.errCode});
    }
  for (const auto& r : mt)
    if (isCallFailure(r.status) || r.status == Status::InternalError) {
      ++rm.totalErrors;
      if (rm.errors.size() < 50)
        rm.errors.push_back({std::string("비교 ") + r.enroll + " vs " + r.attempt, toString(r.status), r.errMsg, r.errCode});
    }

  if (cfg.html) {
    out.htmlPath = joinPath(runDir, L"report.html");
    HtmlReportWriter w;
    if (!w.write(rm, out.htmlPath, err)) l.onLog("[오류] " + err);
  }
  if (cfg.docx) {
    out.docxPath = joinPath(runDir, L"report.docx");
    DocxReportWriter w;
    if (!w.write(rm, out.docxPath, err)) l.onLog("[오류] " + err);
  }
  l.onProgress(1, 1, 0);
  l.onLog("EER " + fmtPct(m.eer) + ", EER* " + fmtPct(m.eerStar) + ", FTE " + fmtPct(m.fte) + ", FMR100 " + fmtPct(m.fmr100) +
          ", FMR1000 " + fmtPct(m.fmr1000));
  out.ok = true;
  out.message = "완료";
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
  return buildReport(runDir, cfg, l);
}

RunOutcome Evaluator::run(const std::wstring& resumeDir) {
  RunOutcome out;
  auto fail = [&](const std::string& msg) {
    out.message = msg;
    l_.onLog("[오류] " + msg);
    return out;
  };
  const bool resume = !resumeDir.empty();
  std::string err;

  // ---------------- STEP 0 사전 검증 ----------------
  l_.onPhase("STEP 0 사전 검증", 1);
  std::wstring runDir;
  if (resume) {
    EvalConfig saved;
    if (!EvalConfig::load(joinPath(resumeDir, L"config.json"), saved, err)) return fail("이어하기: config.json을 읽을 수 없음 — " + err);
    saved.workers = cfg_.workers;     // 실행 옵션은 현재 값 사용
    saved.timeoutSec = cfg_.timeoutSec;
    cfg_ = saved;
    runDir = resumeDir;
    l_.onLog("이어하기: " + toUtf8(runDir));
  }
  if (cfg_.items.empty())
    for (auto& s : allReportItems()) cfg_.items.insert(s);
  if (!fileExists(cfg_.dllPath)) return fail("DLL 파일이 없음: " + toUtf8(cfg_.dllPath));
  int bits = peBitness(cfg_.dllPath);
  if (!bits) return fail("Windows DLL이 아니거나 지원하지 않는 플랫폼");
  std::wstring workerExe = joinPath(exeDir(), bits == 32 ? L"FPWorker32.exe" : L"FPWorker64.exe");
  if (!fileExists(workerExe)) return fail("Worker 실행 파일이 없음: " + toUtf8(workerExe));
  l_.onLog("DLL " + toUtf8(fileNameOf(cfg_.dllPath)) + " — " + std::to_string(bits) + "bit, Worker " + toUtf8(fileNameOf(workerExe)));

  FingerprintDB db;
  if (!db.scan(cfg_.dbPath, err)) return fail(err);
  for (auto& w : db.warnings()) l_.onLog("[경고] " + w);
  l_.onLog("DB: 이미지 " + std::to_string(db.images().size()) + "장, 손가락 " + std::to_string(db.fingers().size()) + "개");

  if (!resume) {
    if (cfg_.outputDir.empty()) return fail("결과 폴더를 지정하세요");
    // 같은 초에 시작한 평가끼리 폴더가 겹치지 않도록 이미 있으면 _2, _3 … 을 붙인다
    std::wstring base = joinPath(cfg_.outputDir, fileNameOf(cfg_.dbPath) + L"_" + nowStamp());
    runDir = base;
    for (int k = 2; dirExists(runDir); ++k) runDir = base + L"_" + std::to_wstring(k);
  }
  if (!makeDirs(runDir)) return fail("결과 폴더를 만들 수 없음: " + toUtf8(runDir));
  out.runDir = runDir;

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
  l_.onLog("KISA_Enroll / KISA_Attempt / KISA_Compare 확인");
  l_.onProgress(1, 1, 0);

  // ---------------- STEP 1 이미지 캐시 ----------------
  std::wstring cacheDir = joinPath(runDir, L"cache");
  makeDirs(cacheDir);
  std::wstring tplDir = cfg_.templateDir.empty() ? joinPath(runDir, L"templates")
                                                 : joinPath(cfg_.templateDir, fileNameOf(runDir));
  makeDirs(tplDir);
  const auto& imgs = db.images();
  l_.onPhase("STEP 1 이미지 캐시", imgs.size());
  int W = 0, H = 0;
  std::vector<std::wstring> rawPath(imgs.size());
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
    if (!fileExists(rawPath[i]) && !writeFile(rawPath[i], g.pixels.data(), g.pixels.size()))
      return fail("캐시 쓰기 실패");
    if (i % 10 == 0 || i + 1 == imgs.size()) l_.onProgress(i + 1, imgs.size(), 0);
  }
  l_.onLog("이미지 크기 " + std::to_string(W) + "x" + std::to_string(H) + ", iType " + std::to_string(cfg_.iType));

  // ---------------- STEP 2 평가 계획 ----------------
  l_.onPhase("STEP 2 평가 계획", 1);
  EvalPlan plan;
  std::wstring planPath = joinPath(runDir, L"plan.csv");
  if (resume) {
    if (!EvalPlan::load(planPath, db, plan, err)) return fail("plan.csv: " + err);
  } else {
    std::vector<std::string> warns;
    if (!EvalPlan::build(cfg_, db, plan, warns, err)) return fail(err);
    for (auto& w : warns) l_.onLog("[경고] " + w);
    plan.save(planPath, db);
    plan.savePairs(runDir, db);
    cfg_.save(joinPath(runDir, L"config.json"));
    Json info = Json::object();
    info["dllName"] = toUtf8(fileNameOf(cfg_.dllPath));
    info["dllBits"] = bits;
    info["dbName"] = toUtf8(fileNameOf(cfg_.dbPath));
    info["images"] = (int)imgs.size();
    info["fingers"] = (int)db.fingers().size();
    info["width"] = W;
    info["height"] = H;
    info["startedAt"] = toUtf8(nowStamp());
    writeText(joinPath(runDir, L"run_info.json"), info.dump());
  }
  l_.onLog("계획: 등록 " + std::to_string(plan.nEnroll) + ", 비교용 추출 " + std::to_string(plan.nAttempt) + ", 본인비교 " +
           std::to_string(plan.nGenuine) + ", 타인비교 " + std::to_string(plan.nImpostor) + " (seed " + std::to_string(plan.seed) + ")");
  l_.onProgress(1, 1, 0);

  ResultStore store;
  if (!store.open(runDir, err)) return fail(err);
  std::set<uint32_t> doneIds;
  std::map<int, bool> enrollOk, attemptOk;  // 이미지별 추출 성공 여부
  if (resume) {
    std::vector<ExtractRecord> ex;
    std::vector<MatchRecord> mt;
    ResultStore::load(runDir, ex, mt, err);
    for (auto& r : ex) {
      if (!isFinal(r.status)) continue;
      doneIds.insert(r.jobId);
      int idx = db.find(r.image);
      if (idx >= 0) (r.kind == 'E' ? enrollOk : attemptOk)[idx] = r.status == Status::OK;
    }
    for (auto& r : mt)
      if (isFinal(r.status)) doneIds.insert(r.jobId);
    l_.onLog("기존 결과 " + std::to_string(doneIds.size()) + "건은 건너뜀");
  }
  std::mutex okMu;

  auto tplPath = [&](char kind, int img) {
    return joinPath(tplDir, fromUtf8(std::string(1, kind) + "_" + imageKey(db, img, cfg_.anonymize) + ".tpl"));
  };

  // ---------------- STEP 3 특징 추출 ----------------
  std::vector<Job> pending;
  for (auto& j : plan.extractJobs)
    if (!doneIds.count(j.id)) pending.push_back(j);
  l_.onPhase("STEP 3 특징 추출 (Enroll/Attempt)", pending.size());
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
      std::lock_guard<std::mutex> lk(okMu);
      (j.kind == JobKind::Enroll ? enrollOk : attemptOk)[j.a] = r.status == Status::OK;
    }
    if (r.status != Status::OK) {
      ++fails;
      l_.onLog(std::string("[") + toString(r.status) + "] " + (j.kind == JobKind::Enroll ? "등록 " : "추출 ") + rec.image +
               (rec.errMsg.empty() ? "" : " — " + rec.errMsg));
    }
    size_t d = ++done;
    if (d % 10 == 0 || d == pending.size()) l_.onProgress(d, pending.size(), fails);
  };
  if (!sched.run(pending.size(), makeExtract, doneExtract, err)) return fail("평가 중단: " + err);
  if (sched.stopRequested()) {
    out.stopped = true;
    out.message = "중지됨 — 결과 폴더에서 이어하기 가능";
    l_.onLog(out.message);
    return out;
  }

  // ---------------- STEP 4 비교 ----------------
  std::vector<Job> cmp;
  size_t skipped = 0;
  for (auto& j : plan.compareJobs) {
    if (doneIds.count(j.id)) continue;
    Status skip = Status::OK;
    if (!enrollOk[j.a]) skip = Status::SkipEF;
    else if (!attemptOk[j.b]) skip = Status::SkipAF;
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
  if (skipped) l_.onLog("템플릿이 없어 건너뛴 비교 " + std::to_string(skipped) + "건 (SKIP_EF/SKIP_AF)");
  l_.onPhase("STEP 4 비교 (Compare)", cmp.size());
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
      l_.onLog(std::string("[") + toString(r.status) + "] 비교 " + rec.enroll + " vs " + rec.attempt +
               (rec.errMsg.empty() ? "" : " — " + rec.errMsg));
    }
    size_t d = ++done;
    if (d % 20 == 0 || d == cmp.size()) l_.onProgress(d, cmp.size(), fails);
  };
  if (!sched.run(cmp.size(), makeCompare, doneCompare, err)) return fail("평가 중단: " + err);
  if (sched.stopRequested()) {
    out.stopped = true;
    out.message = "중지됨 — 결과 폴더에서 이어하기 가능";
    l_.onLog(out.message);
    return out;
  }
  store.close();

  // ---------------- STEP 5 지표·리포트 ----------------
  RunOutcome rep = buildReport(runDir, cfg_, l_);
  rep.runDir = runDir;
  return rep;
}

}  // namespace fpe
