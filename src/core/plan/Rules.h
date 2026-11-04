#pragma once
// 등록·본인비교·타인비교 조합 규칙 (전략 패턴)
#include <memory>
#include <string>
#include <vector>

#include "config/EvalConfig.h"
#include "db/FingerprintDB.h"
#include "plan/Rng.h"

namespace fpe {

struct Pair {
  int enroll = -1;   // FingerprintDB 이미지 번호
  int attempt = -1;
  char type = 'G';   // 'G' genuine, 'I' impostor
  bool operator<(const Pair& o) const {
    return enroll != o.enroll ? enroll < o.enroll : attempt != o.attempt ? attempt < o.attempt : type < o.type;
  }
  bool operator==(const Pair& o) const { return enroll == o.enroll && attempt == o.attempt && type == o.type; }
};

class IEnrollRule {
 public:
  virtual ~IEnrollRule() = default;
  virtual std::vector<int> select(const FingerprintDB& db, Rng& rng) const = 0;
  virtual std::string describe() const = 0;
};

class IPairRule {
 public:
  virtual ~IPairRule() = default;
  virtual std::vector<Pair> generate(const FingerprintDB& db, const std::vector<int>& enrollSet, Rng& rng) const = 0;
  virtual std::string describe() const = 0;
};

// ---- 등록 규칙 ----
class AllEnroll : public IEnrollRule {
 public:
  std::vector<int> select(const FingerprintDB& db, Rng&) const override;
  std::string describe() const override { return "모든 이미지를 각각 등록"; }
};

class IndexEnroll : public IEnrollRule {
 public:
  explicit IndexEnroll(int k) : k_(k) {}
  std::vector<int> select(const FingerprintDB& db, Rng&) const override;
  std::string describe() const override { return "각 손가락의 " + std::to_string(k_) + "번 이미지 등록"; }

 private:
  int k_;
};

class RandomPerFingerEnroll : public IEnrollRule {
 public:
  std::vector<int> select(const FingerprintDB& db, Rng& rng) const override;
  std::string describe() const override { return "손가락마다 무작위 1장 등록"; }
};

// ---- 본인비교 규칙 ----
class GenuineAll : public IPairRule {
 public:
  std::vector<Pair> generate(const FingerprintDB& db, const std::vector<int>& e, Rng&) const override;
  std::string describe() const override { return "모든 조합(순서 있음)"; }
};

// ---- 타인비교 규칙 ----
class ImpostorAll : public IPairRule {
 public:
  std::vector<Pair> generate(const FingerprintDB& db, const std::vector<int>& e, Rng&) const override;
  std::string describe() const override { return "모든 조합"; }
};

class ImpostorFirstOne : public IPairRule {
 public:
  std::vector<Pair> generate(const FingerprintDB& db, const std::vector<int>& e, Rng&) const override;
  std::string describe() const override { return "손가락별 첫 1장끼리"; }
};

class ImpostorSelectedIndex : public IPairRule {
 public:
  explicit ImpostorSelectedIndex(std::vector<int> idx) : idx_(std::move(idx)) {}
  std::vector<Pair> generate(const FingerprintDB& db, const std::vector<int>& e, Rng&) const override;
  std::string describe() const override;

 private:
  std::vector<int> idx_;
};

// ---- 데코레이터: 다른 규칙의 결과를 가공 ----
class SymmetricFilter : public IPairRule {  // (a,b)와 (b,a) 중 하나만
 public:
  explicit SymmetricFilter(std::unique_ptr<IPairRule> inner) : inner_(std::move(inner)) {}
  std::vector<Pair> generate(const FingerprintDB& db, const std::vector<int>& e, Rng& rng) const override;
  std::string describe() const override { return inner_->describe() + ", 대칭 제거"; }

 private:
  std::unique_ptr<IPairRule> inner_;
};

class RandomSample : public IPairRule {  // 무작위 n개
 public:
  RandomSample(std::unique_ptr<IPairRule> inner, int n) : inner_(std::move(inner)), n_(n) {}
  std::vector<Pair> generate(const FingerprintDB& db, const std::vector<int>& e, Rng& rng) const override;
  std::string describe() const override { return inner_->describe() + " 중 무작위 " + std::to_string(n_) + "개"; }

 private:
  std::unique_ptr<IPairRule> inner_;
  int n_;
};

// 설정 → 규칙 객체 (팩토리)
struct RuleFactory {
  static std::unique_ptr<IEnrollRule> enroll(const EvalConfig& c);
  static std::unique_ptr<IPairRule> genuine(const EvalConfig& c);
  static std::unique_ptr<IPairRule> impostor(const EvalConfig& c);
};

// 부분 평가용 쌍 파일 읽기: "enroll,attempt,type" (예: 101_1,101_2,G). '#'로 시작하는 줄은 주석.
bool loadPairsFile(const std::wstring& path, const FingerprintDB& db, std::vector<Pair>& out,
                   std::vector<std::string>& warnings, std::string& err);

}  // namespace fpe
