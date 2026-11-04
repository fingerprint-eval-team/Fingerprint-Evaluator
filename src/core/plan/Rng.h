#pragma once
// 재현 가능한 난수. std::shuffle / uniform_int_distribution은 표준 라이브러리 구현마다 결과가
// 다를 수 있으므로 쓰지 않고, 출력이 표준으로 고정된 mt19937_64 위에 직접 구현한다.
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace fpe {

class Rng {
 public:
  explicit Rng(uint64_t seed) : eng_(seed) {}

  uint64_t next() { return eng_(); }

  // [0, n) 균등 정수 (거부 샘플링으로 편향 제거)
  uint64_t below(uint64_t n) {
    if (n <= 1) return 0;
    const uint64_t limit = UINT64_MAX - UINT64_MAX % n;
    uint64_t x;
    do {
      x = eng_();
    } while (x >= limit);
    return x % n;
  }

  // Fisher–Yates
  template <class T>
  void shuffle(std::vector<T>& v) {
    for (size_t i = v.size(); i > 1; --i) std::swap(v[i - 1], v[(size_t)below(i)]);
  }

  // 규칙마다 독립된 하위 seed: 한 규칙을 바꿔도 다른 규칙 결과는 유지된다
  static uint64_t derive(uint64_t seed, const char* tag) {
    uint64_t h = 1469598103934665603ULL;
    for (; *tag; ++tag) h = (h ^ (uint8_t)*tag) * 1099511628211ULL;
    return seed ^ h;
  }

 private:
  std::mt19937_64 eng_;
};

}  // namespace fpe
