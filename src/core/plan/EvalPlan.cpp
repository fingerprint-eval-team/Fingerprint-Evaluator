#include "plan/EvalPlan.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <set>

#include "PathUtil.h"
#include "result/Csv.h"

namespace fpe {

namespace {

// 같은 이미지끼리의 비교인지: 같은 번호이거나, 픽셀 해시가 같은 복사본
bool sameImage(int a, int b, const std::vector<uint64_t>* pixelHash) {
  if (a == b) return true;
  if (!pixelHash || a < 0 || b < 0) return false;
  size_t ua = static_cast<size_t>(a), ub = static_cast<size_t>(b);
  return ua < pixelHash->size() && ub < pixelHash->size() && (*pixelHash)[ua] == (*pixelHash)[ub];
}

bool parseUint(const std::string& s, uint64_t& v) {
  if (s.empty() || s.size() > 20 || s[0] == '-' || s[0] == '+') return false;
  try {
    size_t pos = 0;
    v = std::stoull(s, &pos);
    return pos == s.size();
  } catch (const std::exception&) {
    return false;
  }
}

}  // namespace

bool EvalPlan::build(const EvalConfig& cfg, const FingerprintDB& db, EvalPlan& out,
                     std::vector<std::string>& warnings, std::string& err, const std::vector<uint64_t>* pixelHash) {
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

  // 동일 이미지 쌍·중복 쌍 제거. 파일명이 달라도 픽셀이 같은 복사본끼리의 비교는 제외하고 기록한다.
  std::sort(pairs.begin(), pairs.end());
  pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
  pairs.erase(std::remove_if(pairs.begin(), pairs.end(), [](const Pair& p) { return p.enroll == p.attempt; }),
              pairs.end());
  size_t before = pairs.size();
  pairs.erase(std::remove_if(pairs.begin(), pairs.end(),
                             [&](const Pair& p) {
                               if (!sameImage(p.enroll, p.attempt, pixelHash)) return false;
                               warnings.push_back("동일 이미지 비교 제외: " + db.images()[p.enroll].name() + " = " +
                                                  db.images()[p.attempt].name() + " (픽셀 동일)");
                               return true;
                             }),
              pairs.end());
  out.nSameImageRemoved = before - pairs.size();
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
  s += "# same_image_removed=" + std::to_string(nSameImageRemoved) + "\n";
  s += "jobId,kind,a,b,type\n";
  auto name = [&](int i) { return i >= 0 ? db.images()[i].name() : std::string(); };
  for (const auto* list : {&extractJobs, &compareJobs})
    for (const Job& j : *list)
      s += csvRow({std::to_string(j.id), std::string(1, (char)j.kind), name(j.a), name(j.b),
                   j.type ? std::string(1, j.type) : std::string()});
  return writeText(path, s);
}

bool EvalPlan::load(const std::wstring& path, const FingerprintDB& db, EvalPlan& out, std::string& err,
                    const std::vector<uint64_t>* pixelHash) {
  std::vector<uint8_t> buf;
  if (!readFile(path, buf)) {
    err = "cannot read plan " + toUtf8(path);
    return false;
  }
  out = EvalPlan{};
  std::string text(buf.begin(), buf.end());
  auto headerValue = [&](const char* key) -> std::string {
    size_t p = text.find(key);
    if (p == std::string::npos) return std::string();
    p += strlen(key);
    std::string v = text.substr(p, text.find('\n', p) - p);
    if (!v.empty() && v.back() == '\r') v.pop_back();
    return v;
  };
  std::string seedText = headerValue("# seed=");
  if (!seedText.empty() && !parseUint(seedText, out.seed)) {
    err = "plan.csv: invalid seed '" + seedText + "'";
    return false;
  }
  uint64_t removed = 0;
  if (parseUint(headerValue("# same_image_removed="), removed)) out.nSameImageRemoved = static_cast<size_t>(removed);
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
  std::set<uint32_t> ids;
  for (const std::string& line : splitLines(text)) {
    if (line.empty() || line[0] == '#' || line.rfind("jobId", 0) == 0) continue;
    auto f = parseCsvLine(line);
    if (f.size() < 5) continue;
    Job j;
    uint64_t id = 0;
    if (!parseUint(f[0], id) || id == 0 || id > 0xFFFFFFFFull || f[1].size() != 1) {
      err = "plan.csv: malformed row: " + line;
      return false;
    }
    j.id = static_cast<uint32_t>(id);
    char kind = f[1][0];
    if (kind != 'E' && kind != 'A' && kind != 'C') {
      err = "plan.csv: unknown job kind: " + line;
      return false;
    }
    j.kind = static_cast<JobKind>(kind);
    j.a = db.find(f[2]);
    j.b = f[3].empty() ? -1 : db.find(f[3]);
    j.type = f[4].empty() ? 0 : f[4][0];
    if (j.a < 0 || (j.kind == JobKind::Compare && j.b < 0)) {
      err = "plan refers to missing image: " + line;
      return false;
    }
    if (j.kind == JobKind::Compare && j.type != 'G' && j.type != 'I') {
      err = "plan.csv: compare type must be G or I: " + line;
      return false;
    }
    if (!ids.insert(j.id).second) {
      err = "plan.csv: duplicate jobId: " + line;
      return false;
    }
    if (j.kind == JobKind::Compare && sameImage(j.a, j.b, pixelHash)) {  // 동일 이미지 비교 금지
      ++out.nSameImageRemoved;
      continue;
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
  if (out.compareJobs.empty()) {
    err = "plan.csv: no compare jobs";
    return false;
  }
  return true;
}

bool EvalPlan::savePairs(const std::wstring& dir, const FingerprintDB& db) const {
  std::string g = "# seed=" + std::to_string(seed) + " rule=" + genuineDesc + "\nenroll,attempt,type\n";
  std::string im = "# seed=" + std::to_string(seed) + " rule=" + impostorDesc + "\nenroll,attempt,type\n";
  for (const Job& j : compareJobs)
    (j.type == 'G' ? g : im) += csvRow({db.images()[j.a].name(), db.images()[j.b].name(), std::string(1, j.type)});
  return writeText(joinPath(dir, L"pairs_genuine.csv"), g) && writeText(joinPath(dir, L"pairs_impostor.csv"), im);
}

}  // namespace fpe
