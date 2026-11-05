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
constexpr const char* FMR10000 = "FMR10000";
constexpr const char* Time = "Time";
constexpr const char* Size = "Size";
constexpr const char* DET = "DET";
constexpr const char* Dist = "Dist";
constexpr const char* Rates = "FMRFNMR";
constexpr const char* Errors = "Errors";
}  // namespace item

// 순서 = GUI 체크박스 순서(IDC_ITEM0 + i). 새 항목은 뒤에 추가한다.
std::vector<std::string> allReportItems();
std::set<std::string> defaultReportItems();

struct EvalConfig {
  // 대상
  std::wstring dllPath;
  std::wstring dbPath;
  int iType = 1;
  std::wstring templateDir;   // 비우면 결과 폴더/templates
  std::wstring outputDir;     // 결과(설정·계획·로그 CSV·캐시) 폴더
  std::wstring reportDir;     // 비우면 결과 폴더. 지정하면 <reportDir>/<실행 이름>/report.*
  std::wstring logDir;        // 비우면 결과 폴더. 지정하면 <logDir>/<실행 이름>/eval.log

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
  // 기본값은 전체 항목. 설정 파일에 items가 있으면 비어 있어도 그대로 따른다(사용자가 전부 해제한 경우).
  std::set<std::string> items = defaultReportItems();
  std::string eerMethod = "FVC";         // FVC | Nearest
  std::string denominator = "predefined"; // predefined | attempted

  Json toJson() const;
  static EvalConfig fromJson(const Json& j);
  bool save(const std::wstring& path) const;
  static bool load(const std::wstring& path, EvalConfig& out, std::string& err);
  bool hasItem(const char* id) const { return items.count(id) > 0; }
};

}  // namespace fpe
