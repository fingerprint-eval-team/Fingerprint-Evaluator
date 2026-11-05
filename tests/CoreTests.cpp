// FPEvalCore 단위 테스트 (외부 프레임워크 없이 동작하는 최소 테스트 러너)
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include "PathUtil.h"
#include "config/EvalConfig.h"
#include "config/Json.h"
#include "metrics/MetricsCalculator.h"
#include "plan/EvalPlan.h"
#include "plan/Rng.h"
#include "plan/Rules.h"
#include "report/ZipWriter.h"
#include "result/Csv.h"

using namespace fpe;

static int g_fail = 0, g_pass = 0;
#define CHECK(cond)                                              \
  do {                                                           \
    if (cond) {                                                  \
      ++g_pass;                                                  \
    } else {                                                     \
      ++g_fail;                                                  \
      printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
    }                                                            \
  } while (0)
#define NEAR(a, b) CHECK(std::fabs((a) - (b)) < 1e-9)

// 손가락 10개 x 8장 가상 DB (FVC 구성과 동일)
static FingerprintDB fakeDb() {
  std::vector<FingerprintImage> v;
  for (int f = 101; f <= 110; ++f)
    for (int i = 1; i <= 8; ++i) v.push_back({f, i, L""});
  FingerprintDB db;
  db.setImages(v);
  return db;
}

static void testRng() {
  Rng a(42), b(42);
  std::vector<int> x{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, y = x;
  a.shuffle(x);
  b.shuffle(y);
  CHECK(x == y);  // 같은 seed → 같은 순서
  Rng c(43);
  std::vector<int> z{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  c.shuffle(z);
  CHECK(x != z);
  for (int i = 0; i < 1000; ++i) CHECK(a.below(7) < 7);
}

static void testRules() {
  FingerprintDB db = fakeDb();
  Rng r(1);
  auto all = AllEnroll().select(db, r);
  CHECK(all.size() == 80);
  CHECK(IndexEnroll(1).select(db, r).size() == 10);

  auto gAll = GenuineAll().generate(db, all, r);
  CHECK(gAll.size() == 560);  // 10 x 8 x 7
  auto gSym = SymmetricFilter(std::make_unique<GenuineAll>()).generate(db, all, r);
  CHECK(gSym.size() == 280);  // 10 x C(8,2)
  auto iAll = ImpostorAll().generate(db, all, r);
  CHECK(iAll.size() == 5760);  // 80 x 72
  auto iFirst = ImpostorFirstOne().generate(db, all, r);
  CHECK(iFirst.size() == 90);  // 10 x 9
  auto iFirstSym = SymmetricFilter(std::make_unique<ImpostorFirstOne>()).generate(db, all, r);
  CHECK(iFirstSym.size() == 45);
  auto rnd = RandomSample(std::make_unique<ImpostorAll>(), 100).generate(db, all, r);
  CHECK(rnd.size() == 100);

  for (auto* v : {&gAll, &iAll})
    for (auto& p : *v) CHECK(p.enroll != p.attempt);  // 동일 이미지 비교 없음
  for (auto& p : gAll) CHECK(db.images()[p.enroll].finger == db.images()[p.attempt].finger);
  for (auto& p : iAll) CHECK(db.images()[p.enroll].finger != db.images()[p.attempt].finger);
}

static void testPlanReproducible() {
  FingerprintDB db = fakeDb();
  EvalConfig c;
  c.genuineRule = "RandomN";
  c.genuineN = 50;
  c.impostorRule = "RandomN";
  c.impostorN = 300;
  c.impostorSymmetric = false;
  EvalPlan p1, p2, p3;
  std::vector<std::string> w;
  std::string err;
  CHECK(EvalPlan::build(c, db, p1, w, err));
  CHECK(EvalPlan::build(c, db, p2, w, err));
  c.seed += 1;
  CHECK(EvalPlan::build(c, db, p3, w, err));
  CHECK(p1.nGenuine == 50 && p1.nImpostor == 300);
  bool same = p1.compareJobs.size() == p2.compareJobs.size();
  for (size_t i = 0; same && i < p1.compareJobs.size(); ++i)
    same = p1.compareJobs[i].a == p2.compareJobs[i].a && p1.compareJobs[i].b == p2.compareJobs[i].b;
  CHECK(same);
  bool diff = false;
  for (size_t i = 0; i < p1.compareJobs.size() && i < p3.compareJobs.size(); ++i)
    diff |= p1.compareJobs[i].a != p3.compareJobs[i].a || p1.compareJobs[i].b != p3.compareJobs[i].b;
  CHECK(diff);
}

static MatchRecord mr(char t, double s, Status st = Status::OK) {
  MatchRecord r;
  r.type = t;
  r.score = s;
  r.status = st;
  return r;
}

static void testMetrics() {
  // genuine 5개, impostor 5개, 완전 분리 → EER 0
  std::vector<MatchRecord> m;
  for (double s : {0.6, 0.7, 0.8, 0.9, 1.0}) m.push_back(mr('G', s));
  for (double s : {0.0, 0.1, 0.2, 0.3, 0.4}) m.push_back(mr('I', s));
  MetricOptions o;
  auto r = MetricsCalculator::compute({}, m, 5, 5, o);
  NEAR(r.eer, 0.0);
  NEAR(r.zeroFMR, 0.0);
  NEAR(r.zeroFNMR, 0.0);

  // 겹침: G {0.3,0.5,0.7,0.9}, I {0.2,0.4,0.6,0.8}
  m.clear();
  for (double s : {0.3, 0.5, 0.7, 0.9}) m.push_back(mr('G', s));
  for (double s : {0.2, 0.4, 0.6, 0.8}) m.push_back(mr('I', s));
  r = MetricsCalculator::compute({}, m, 4, 4, o);
  // t=0.5: FMR = 2/4 (0.6,0.8), FNMR = 1/4 (0.3) / t=0.6: FMR 2/4, FNMR 2/4 → 정확히 같음
  NEAR(r.eer, 0.5);
  CHECK(r.eerExact);
  NEAR(r.zeroFMR, 0.75);   // t>0.8 → G 중 0.9만 통과 → FNMR 3/4
  NEAR(r.fmr100, 0.75);

  // 실패 포함: genuine 1건 등록 실패 → EER과 EER* 달라야 함
  m.clear();
  for (double s : {0.6, 0.7, 0.8, 0.9}) m.push_back(mr('G', s));
  m.push_back(mr('G', 0, Status::SkipEF));
  for (double s : {0.1, 0.2, 0.3, 0.4, 0.5}) m.push_back(mr('I', s));
  r = MetricsCalculator::compute({}, m, 5, 5, o);
  NEAR(r.eerStar, 0.0);
  CHECK(r.eer > 0.0);
  CHECK(r.nGEF == 1);
  NEAR(r.zeroFNMR, -1.0);  // 등록 실패가 있으면 FNMR=0 불가능

  // FTE
  std::vector<ExtractRecord> ex(4);
  for (auto& e : ex) e.kind = 'E';
  ex[1].status = Status::Timeout;
  r = MetricsCalculator::compute(ex, {}, 0, 0, o);
  NEAR(r.fte, 0.25);
}

// 부분 평가·전건 실패: 분모가 0인 지표는 0%가 아니라 N/A여야 한다
static void testMetricsUndefined() {
  MetricOptions o;
  std::vector<MatchRecord> m;
  for (double s : {0.6, 0.7, 0.8}) m.push_back(mr('G', s));
  auto r = MetricsCalculator::compute({}, m, 3, 0, o);  // genuine 쌍만 있는 쌍 파일
  CHECK(!r.ratesDefined);
  CHECK(isNA(r.eer) && isNA(r.fmr100) && isNA(r.fmr1000) && isNA(r.zeroFMR) && isNA(r.zeroFNMR));
  CHECK(!r.starDefined && isNA(r.eerStar));
  CHECK(r.det.empty() && r.rates.empty());
  CHECK(isNA(r.rejI));
  NEAR(r.rejG, 0.0);

  m.clear();
  for (double s : {0.1, 0.2}) m.push_back(mr('I', s));
  r = MetricsCalculator::compute({}, m, 0, 2, o);  // impostor 쌍만
  CHECK(!r.ratesDefined && isNA(r.eer));

  // 모든 genuine 비교 실패: EER은 계산되지만 EER*는 성공 점수가 없어 N/A
  m.clear();
  m.push_back(mr('G', 0, Status::Fail));
  m.push_back(mr('G', 0, Status::Crash));
  for (double s : {0.1, 0.2}) m.push_back(mr('I', s));
  r = MetricsCalculator::compute({}, m, 2, 2, o);
  CHECK(r.ratesDefined && !isNA(r.eer));
  CHECK(!r.starDefined && isNA(r.eerStar));
  NEAR(r.rejG, 1.0);

  // 등록 시도 없음 → FTE N/A, 평가 프로그램 오류(INTERNAL)는 FTE에서 제외
  r = MetricsCalculator::compute({}, {}, 0, 0, o);
  CHECK(isNA(r.fte));
  std::vector<ExtractRecord> ex(3);
  for (auto& e : ex) e.kind = 'E';
  ex[0].status = Status::InternalError;
  ex[1].status = Status::Fail;
  r = MetricsCalculator::compute(ex, {}, 0, 0, o);
  CHECK(r.nInternal == 1 && r.nEnroll == 2);
  NEAR(r.fte, 0.5);

  // 중단된 평가: 비교 결과가 하나도 없으면 계획 분모로는 FMR=FNMR=0이 되어 EER 0%처럼 보인다 → N/A
  r = MetricsCalculator::compute({}, {}, 4, 1, o);
  CHECK(r.ratesDefined && !isNA(r.eer));  // 완료로 취급하면 정의되는 값 (이 상황을 막아야 함)
  MetricOptions partial;
  partial.complete = false;
  r = MetricsCalculator::compute({}, {}, 4, 1, partial);
  CHECK(!r.complete && !r.ratesDefined && !r.starDefined);
  CHECK(isNA(r.eer) && isNA(r.eerStar) && isNA(r.fmr100) && isNA(r.fmr1000) && isNA(r.fmr10000));
  CHECK(isNA(r.zeroFMR) && isNA(r.zeroFNMR) && r.det.empty() && r.rates.empty());
  CHECK(r.nG == 4 && r.nI == 1 && r.nGDone == 0 && r.nIDone == 0);
  // 일부만 끝난 경우에도 최종 지표는 N/A, 완료 건수는 집계
  m.clear();
  m.push_back(mr('G', 0.8));
  m.push_back(mr('G', 0, Status::SkipEF));
  m.push_back(mr('I', 0.1));
  r = MetricsCalculator::compute({}, m, 4, 1, partial);
  CHECK(isNA(r.eer) && isNA(r.eerStar) && r.nGDone == 2 && r.nIDone == 1);
  NEAR(r.rejG, 0.0);
}

static std::wstring tempFile(const wchar_t* name) {
  return (std::filesystem::temp_directory_path() / name).wstring();
}

// 깨진 plan.csv는 예외 없이 false, 픽셀이 같은 이미지끼리의 비교는 계획·불러오기 모두에서 제외
static void testPlanLoadAndDuplicates() {
  FingerprintDB db = fakeDb();
  std::string err;
  EvalPlan p;
  std::wstring path = tempFile(L"fpeval_test_plan.csv");
  const char* bad[] = {"# seed=abc\njobId,kind,a,b,type\n1,E,101_1,,\n",
                       "# seed=1\njobId,kind,a,b,type\nx,E,101_1,,\n2,C,101_1,101_2,G\n",
                       "# seed=1\njobId,kind,a,b,type\n1,Z,101_1,,\n2,C,101_1,101_2,G\n",
                       "# seed=1\njobId,kind,a,b,type\n2,C,101_1,101_2,G\n2,C,101_1,101_3,G\n",
                       "# seed=1\njobId,kind,a,b,type\n2,C,101_1,101_2,Q\n"};
  for (const char* text : bad) {
    CHECK(writeText(path, text));
    bool ok = true;
    try {
      ok = EvalPlan::load(path, db, p, err);
    } catch (...) {
      ok = true;
      CHECK(!"exception escaped EvalPlan::load");
    }
    CHECK(!ok && !err.empty());
  }

  // 101_2는 101_1의 복사본(같은 해시)
  std::vector<uint64_t> hash(db.images().size());
  for (size_t i = 0; i < hash.size(); ++i) hash[i] = 1000 + i;
  int a = db.find("101_1"), b = db.find("101_2");
  hash[(size_t)b] = hash[(size_t)a];
  EvalConfig c;
  c.genuineRule = "All";
  c.impostorRule = "All";
  c.impostorSymmetric = false;
  std::vector<std::string> w;
  CHECK(EvalPlan::build(c, db, p, w, err, &hash));
  CHECK(p.nSameImageRemoved == 2);  // (101_1,101_2), (101_2,101_1)
  CHECK(p.nGenuine == 558);
  for (auto& j : p.compareJobs) CHECK(!((j.a == a && j.b == b) || (j.a == b && j.b == a)));
  CHECK(!w.empty());

  // 해시 없이 만든 계획을 해시와 함께 불러오면 그 쌍이 빠진다
  EvalPlan raw, loaded;
  CHECK(EvalPlan::build(c, db, raw, w, err));
  CHECK(raw.save(path, db));
  CHECK(EvalPlan::load(path, db, loaded, err, &hash));
  CHECK(loaded.nGenuine == 558 && loaded.nSameImageRemoved == 2);
  CHECK(loaded.seed == raw.seed);
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

// 리포트 항목: 키가 없으면 기본(전체), 사용자가 전부 해제한 빈 목록은 그대로 유지
static void testReportItemsConfig() {
  EvalConfig c;
  CHECK(c.items.size() == allReportItems().size());
  CHECK(c.hasItem(item::FMR10000));
  c.items.clear();
  EvalConfig d = EvalConfig::fromJson(c.toJson());
  CHECK(d.items.empty());
  Json j = c.toJson();
  Json noItems = Json::object();
  noItems["dllPath"] = j.get("dllPath").asString();
  EvalConfig e = EvalConfig::fromJson(noItems);
  CHECK(e.items.size() == allReportItems().size());
  c.reportDir = L"C:\\리포트";
  c.logDir = L"D:\\log";
  EvalConfig f = EvalConfig::fromJson(c.toJson());
  CHECK(f.reportDir == c.reportDir && f.logDir == c.logDir);
}

static void testJsonCsvZip() {
  EvalConfig c;
  c.dllPath = L"C:\\테스트\\a.dll";
  c.seed = 123456789012ULL;
  c.items = {"EER", "DET"};
  EvalConfig d = EvalConfig::fromJson(c.toJson());
  CHECK(d.dllPath == c.dllPath);
  CHECK(d.seed == c.seed);
  CHECK(d.items.size() == 2);

  auto f = parseCsvLine("1,\"a,b\",\"say \"\"hi\"\"\",");
  CHECK(f.size() == 4 && f[1] == "a,b" && f[2] == "say \"hi\"" && f[3].empty());
  CHECK(parseCsvLine(csvRow({"x,y", "z"}).substr(0, csvRow({"x,y", "z"}).size() - 1))[0] == "x,y");

  const char* s = "123456789";
  CHECK(ZipWriter::crc32((const uint8_t*)s, 9) == 0xCBF43926u);  // CRC-32 표준 검증값
}

int main() {
  struct T {
    const char* name;
    std::function<void()> fn;
  } tests[] = {{"Rng", testRng},
               {"Rules", testRules},
               {"PlanReproducible", testPlanReproducible},
               {"Metrics", testMetrics},
               {"MetricsUndefined", testMetricsUndefined},
               {"PlanLoadAndDuplicates", testPlanLoadAndDuplicates},
               {"ReportItemsConfig", testReportItemsConfig},
               {"JsonCsvZip", testJsonCsvZip}};
  for (auto& t : tests) {
    int before = g_fail;
    t.fn();
    printf("[%s] %s\n", g_fail == before ? "PASS" : "FAIL", t.name);
  }
  printf("\n%d checks passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
