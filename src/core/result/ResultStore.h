#pragma once
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

#include "Status.h"

namespace fpe {

struct ExtractRecord {
  uint32_t jobId = 0;
  char kind = 'E';          // 'E' 등록, 'A' 비교용 추출
  std::string image;        // 101_1
  Status status = Status::OK;
  int retVal = 0;
  int errCode = 0;
  std::string errMsg;
  int64_t elapsedUs = 0;
  int size = 0;
  uint32_t sehCode = 0;
};

struct MatchRecord {
  uint32_t jobId = 0;
  std::string enroll, attempt;
  char type = 'G';
  Status status = Status::OK;
  double score = 0;
  int errCode = 0;
  std::string errMsg;
  int64_t elapsedUs = 0;
  uint32_t sehCode = 0;
};

// 결과를 한 줄씩 즉시 CSV에 기록한다 (중단돼도 결과 보존 → 이어하기 가능). 스레드 안전.
class ResultStore {
 public:
  ~ResultStore();
  bool open(const std::wstring& runDir, std::string& err);
  void append(const ExtractRecord& r);
  void append(const MatchRecord& r);
  void close();

  // 같은 jobId가 여러 번 있으면 마지막 기록을 사용한다
  static bool load(const std::wstring& runDir, std::vector<ExtractRecord>& ex, std::vector<MatchRecord>& mt,
                   std::string& err);

 private:
  std::mutex mu_;
  FILE* fe_ = nullptr;
  FILE* fm_ = nullptr;
};

}  // namespace fpe
