#pragma once
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "config/Json.h"

namespace fpe {

// 리포트 항목 식별자
namespace item {
constexpr const char* FTE = "FTE";
constexpr const char* REJ = "REJ";
constexpr const char* EER = "EER";
constexpr const char* EERstar = "EERstar";
constexpr const char* ZeroFMR = "ZeroFMR";
constexpr const char* ZeroFNMR = "ZeroFNMR";
constexpr const char* FMR100 = "FMR100";
constexpr const char* FMR1000 = "FMR1000";
constexpr const char* Time = "Time";
constexpr const char* Size = "Size";
constexpr const char* DET = "DET";
constexpr const char* Dist = "Dist";
constexpr const char* Rates = "FMRFNMR";
constexpr const char* Errors = "Errors";
}  // namespace item

std::vector<std::string> allReportItems();

struct EvalConfig {
  // 대상
  std::wstring dllPath;
  std::wstring dbPath;
  int iType = 1;
  std::wstring templateDir;   // 비우면 결과 폴더/templates
  std::wstring outputDir;

  // 시험 규칙
  std::string enrollRule = "All";        // All | First | Index | RandomPerFinger
  int enrollIndex = 1;
  std::string genuineRule = "Symmetric"; // All | Symmetric | RandomN
  int genuineN = 200;
  std::string impostorRule = "FirstOne"; // All | FirstOne | SelectedIndex | RandomN
  int impostorN = 2000;
  std::vector<int> impostorIndices{1};
  bool impostorSymmetric = true;
  std::wstring pairsFile;                // 비어 있지 않으면 부분 평가(파일의 쌍만)
  uint64_t seed = 20261104;

  // 실행
  int workers = 4;
  int timeoutSec = 10;
  bool anonymize = true;                 // Worker에 넘기는 파일명을 일련번호로 바꿈

  // 리포트
  bool html = true;
  bool docx = true;
  std::set<std::string> items;           // 비어 있으면 allReportItems()로 채움
  std::string eerMethod = "FVC";         // FVC | Nearest
  std::string denominator = "predefined"; // predefined | attempted

  Json toJson() const;
  static EvalConfig fromJson(const Json& j);
  bool save(const std::wstring& path) const;
  static bool load(const std::wstring& path, EvalConfig& out, std::string& err);
  bool hasItem(const char* id) const { return items.count(id) > 0; }
};

}  // namespace fpe
