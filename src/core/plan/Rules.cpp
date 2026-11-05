#include "plan/Rules.h"

#include <algorithm>
#include <set>

#include "PathUtil.h"
#include "result/Csv.h"

namespace fpe {

std::vector<int> AllEnroll::select(const FingerprintDB& db, Rng&) const {
  std::vector<int> v;
  for (int i = 0; i < (int)db.images().size(); ++i) v.push_back(i);
  return v;
}

std::vector<int> IndexEnroll::select(const FingerprintDB& db, Rng&) const {
  std::vector<int> v;
  for (int f : db.fingers())
    for (int i : db.imagesOf(f))
      if (db.images()[i].index == k_) v.push_back(i);
  return v;
}

std::vector<int> RandomPerFingerEnroll::select(const FingerprintDB& db, Rng& rng) const {
  std::vector<int> v;
  for (int f : db.fingers()) {
    auto imgs = db.imagesOf(f);
    if (!imgs.empty()) v.push_back(imgs[(size_t)rng.below(imgs.size())]);
  }
  return v;
}

std::vector<Pair> GenuineAll::generate(const FingerprintDB& db, const std::vector<int>& e, Rng&) const {
  std::vector<Pair> out;
  for (int en : e)
    for (int a : db.imagesOf(db.images()[en].finger))
      if (a != en) out.push_back({en, a, 'G'});  // 동일 이미지 비교 금지
  return out;
}

std::vector<Pair> ImpostorAll::generate(const FingerprintDB& db, const std::vector<int>& e, Rng&) const {
  std::vector<Pair> out;
  for (int en : e) {
    int fe = db.images()[en].finger;
    for (int f : db.fingers())
      if (f != fe)
        for (int a : db.imagesOf(f)) out.push_back({en, a, 'I'});
  }
  return out;
}

namespace {
// 손가락마다 등록 집합 안에서 가장 앞 순번 이미지
std::vector<int> firstEnrolledPerFinger(const FingerprintDB& db, const std::vector<int>& e) {
  std::map<int, int> first;
  for (int en : e) {
    int f = db.images()[en].finger;
    auto it = first.find(f);
    if (it == first.end() || db.images()[en].index < db.images()[it->second].index) first[f] = en;
  }
  std::vector<int> v;
  for (auto& kv : first) v.push_back(kv.second);
  return v;
}
}  // namespace

std::vector<Pair> ImpostorFirstOne::generate(const FingerprintDB& db, const std::vector<int>& e, Rng&) const {
  std::vector<Pair> out;
  for (int en : firstEnrolledPerFinger(db, e)) {
    int fe = db.images()[en].finger;
    for (int f : db.fingers()) {
      if (f == fe) continue;
      auto imgs = db.imagesOf(f);
      if (!imgs.empty()) out.push_back({en, imgs.front(), 'I'});
    }
  }
  return out;
}

std::vector<Pair> ImpostorSelectedIndex::generate(const FingerprintDB& db, const std::vector<int>& e, Rng&) const {
  std::vector<Pair> out;
  std::set<int> want(idx_.begin(), idx_.end());
  for (int en : e) {
    int fe = db.images()[en].finger;
    for (int f : db.fingers())
      if (f != fe)
        for (int a : db.imagesOf(f))
          if (want.count(db.images()[a].index)) out.push_back({en, a, 'I'});
  }
  return out;
}

std::string ImpostorSelectedIndex::describe() const {
  std::string s = "다른 손가락의 순번 {";
  for (size_t i = 0; i < idx_.size(); ++i) s += (i ? "," : "") + std::to_string(idx_[i]);
  return s + "}만";
}

std::vector<Pair> SymmetricFilter::generate(const FingerprintDB& db, const std::vector<int>& e, Rng& rng) const {
  std::vector<Pair> out;
  std::set<std::pair<int, int>> seen;
  for (const Pair& p : inner_->generate(db, e, rng)) {
    auto key = std::minmax(p.enroll, p.attempt);
    if (seen.insert({key.first, key.second}).second) out.push_back(p);
  }
  return out;
}

std::vector<Pair> RandomSample::generate(const FingerprintDB& db, const std::vector<int>& e, Rng& rng) const {
  std::vector<Pair> all = inner_->generate(db, e, rng);
  if (n_ <= 0 || (size_t)n_ >= all.size()) return all;
  // 부분 Fisher–Yates로 n개 선택 후 원래 순서로 정렬(사람이 읽기 쉽게)
  std::vector<size_t> idx(all.size());
  for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
  for (size_t i = 0; i < (size_t)n_; ++i) std::swap(idx[i], idx[i + (size_t)rng.below(idx.size() - i)]);
  idx.resize((size_t)n_);
  std::sort(idx.begin(), idx.end());
  std::vector<Pair> out;
  for (size_t i : idx) out.push_back(all[i]);
  return out;
}

std::unique_ptr<IEnrollRule> RuleFactory::enroll(const EvalConfig& c) {
  if (c.enrollRule == "First") return std::make_unique<IndexEnroll>(1);
  if (c.enrollRule == "Index") return std::make_unique<IndexEnroll>(c.enrollIndex);
  if (c.enrollRule == "RandomPerFinger") return std::make_unique<RandomPerFingerEnroll>();
  return std::make_unique<AllEnroll>();
}

std::unique_ptr<IPairRule> RuleFactory::genuine(const EvalConfig& c) {
  if (c.genuineRule == "All") return std::make_unique<GenuineAll>();
  if (c.genuineRule == "RandomN") return std::make_unique<RandomSample>(std::make_unique<GenuineAll>(), c.genuineN);
  return std::make_unique<SymmetricFilter>(std::make_unique<GenuineAll>());
}

std::unique_ptr<IPairRule> RuleFactory::impostor(const EvalConfig& c) {
  std::unique_ptr<IPairRule> r;
  if (c.impostorRule == "All") {
    r = std::make_unique<ImpostorAll>();
  } else if (c.impostorRule == "SelectedIndex") {
    r = std::make_unique<ImpostorSelectedIndex>(c.impostorIndices);
  } else if (c.impostorRule == "RandomN") {
    r = std::make_unique<RandomSample>(std::make_unique<ImpostorAll>(), c.impostorN);
  } else {
    r = std::make_unique<ImpostorFirstOne>();
  }
  if (c.impostorSymmetric) r = std::make_unique<SymmetricFilter>(std::move(r));
  return r;
}

bool loadPairsFile(const std::wstring& path, const FingerprintDB& db, std::vector<Pair>& out,
                   std::vector<std::string>& warnings, std::string& err) {
  std::vector<uint8_t> buf;
  if (!readFile(path, buf)) {
    err = "cannot read pairs file " + toUtf8(path);
    return false;
  }
  std::string text(buf.begin(), buf.end());
  size_t pos = 0;
  int lineNo = 0;
  while (pos < text.size()) {
    size_t e = text.find('\n', pos);
    std::string line = text.substr(pos, e == std::string::npos ? std::string::npos : e - pos);
    pos = e == std::string::npos ? text.size() : e + 1;
    ++lineNo;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty() || line[0] == '#' || line.rfind("enroll", 0) == 0) continue;
    auto f = parseCsvLine(line);
    if (f.size() < 3) {
      warnings.push_back("line " + std::to_string(lineNo) + ": need 3 fields");
      continue;
    }
    int a = db.find(f[0]), b = db.find(f[1]);
    char t = f[2].empty() ? 'G' : (char)toupper(f[2][0]);
    if (a < 0 || b < 0) {
      warnings.push_back("line " + std::to_string(lineNo) + ": unknown image");
      continue;
    }
    if (a == b) {
      warnings.push_back("line " + std::to_string(lineNo) + ": same image pair removed");
      continue;
    }
    bool sameFinger = db.images()[a].finger == db.images()[b].finger;
    if ((t == 'G') != sameFinger) {
      warnings.push_back("line " + std::to_string(lineNo) + ": type corrected by finger id");
      t = sameFinger ? 'G' : 'I';
    }
    out.push_back({a, b, t});
  }
  if (out.empty()) {
    err = "no valid pairs in file";
    return false;
  }
  return true;
}

}  // namespace fpe
