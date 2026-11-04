#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "config/EvalConfig.h"
#include "db/FingerprintDB.h"
#include "plan/Rules.h"

namespace fpe {

enum class JobKind : char { Enroll = 'E', Attempt = 'A', Compare = 'C' };

struct Job {
  uint32_t id = 0;     // 실행 순서 = id 순서
  JobKind kind = JobKind::Enroll;
  int a = -1;          // Enroll/Attempt: 이미지, Compare: 등록 이미지
  int b = -1;          // Compare: 비교 이미지
  char type = 0;       // Compare: 'G' / 'I'
};

// 평가 계획: 무엇을 어떤 순서로 실행할지 전부 담는다. 파일로 저장·불러와 재현한다.
struct EvalPlan {
  uint64_t seed = 0;
  std::vector<Job> extractJobs;  // STEP 3 (Enroll + Attempt 섞음)
  std::vector<Job> compareJobs;  // STEP 4 (Genuine + Impostor 섞음)
  size_t nEnroll = 0, nAttempt = 0, nGenuine = 0, nImpostor = 0;
  std::string enrollDesc, genuineDesc, impostorDesc;

  static bool build(const EvalConfig& cfg, const FingerprintDB& db, EvalPlan& out,
                    std::vector<std::string>& warnings, std::string& err);

  bool save(const std::wstring& path, const FingerprintDB& db) const;
  static bool load(const std::wstring& path, const FingerprintDB& db, EvalPlan& out, std::string& err);
  bool savePairs(const std::wstring& dir, const FingerprintDB& db) const;  // pairs_genuine.csv, pairs_impostor.csv
};

}  // namespace fpe
