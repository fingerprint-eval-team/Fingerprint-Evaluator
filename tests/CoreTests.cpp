// FPEvalCore 단위 테스트 (외부 프레임워크 없이 동작하는 최소 테스트 러너)
#include <cmath>
#include <cstdio>
#include <functional>
#include <set>
#include <string>
#include <vector>

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
#define CHECK(cond)                                                    \
  do {                                                                 \
    if (cond) ++g_pass;                                                \
    else {                                                             \
      ++g_fail;                                                        \
      printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);         \
    }                                                                  \
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
               {"JsonCsvZip", testJsonCsvZip}};
  for (auto& t : tests) {
    int before = g_fail;
    t.fn();
    printf("[%s] %s\n", g_fail == before ? "PASS" : "FAIL", t.name);
  }
  printf("\n%d checks passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
