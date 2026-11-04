#pragma once
#include <set>
#include <string>
#include <vector>

#include "metrics/MetricsCalculator.h"

namespace fpe {

struct ReportFigure {
  std::string id;        // det, score_dist, fmr_fnmr
  std::string caption;
  std::wstring svgPath;  // 절대 경로
  std::wstring emfPath;
  std::string svgRel;    // 리포트 기준 상대 경로 (figures/det.svg)
};

struct ReportError {
  std::string job, status, message;
  int code = 0;
};

// 리포트에 들어갈 모든 내용. 선택하지 않은 항목은 이 단계에서 빠진다.
struct ReportModel {
  std::string title;
  std::vector<std::pair<std::string, std::string>> info;     // 평가 환경
  std::vector<std::pair<std::string, std::string>> summary;  // 요약 지표
  std::vector<std::vector<std::string>> counts;              // 시도 수 표 (첫 행 = 머리글)
  std::vector<ReportFigure> figures;
  std::vector<ReportError> errors;
  size_t totalErrors = 0;
  bool showErrors = true;
};

// 템플릿 메서드: 섹션 순서는 기반 클래스가 정하고, 형식별 출력만 파생 클래스가 구현한다.
class IReportWriter {
 public:
  virtual ~IReportWriter() = default;
  bool write(const ReportModel& m, const std::wstring& path, std::string& err);

 protected:
  virtual void begin(const ReportModel& m) = 0;
  virtual void heading(int level, const std::string& text) = 0;
  virtual void paragraph(const std::string& text) = 0;
  virtual void table(const std::vector<std::vector<std::string>>& rows, bool headerRow) = 0;
  virtual void figure(const ReportFigure& f) = 0;
  virtual bool end(const std::wstring& path, std::string& err) = 0;
};

class HtmlReportWriter : public IReportWriter {
 protected:
  void begin(const ReportModel& m) override;
  void heading(int level, const std::string& text) override;
  void paragraph(const std::string& text) override;
  void table(const std::vector<std::vector<std::string>>& rows, bool headerRow) override;
  void figure(const ReportFigure& f) override;
  bool end(const std::wstring& path, std::string& err) override;

 private:
  std::string html_;
};

class DocxReportWriter : public IReportWriter {
 protected:
  void begin(const ReportModel& m) override;
  void heading(int level, const std::string& text) override;
  void paragraph(const std::string& text) override;
  void table(const std::vector<std::vector<std::string>>& rows, bool headerRow) override;
  void figure(const ReportFigure& f) override;
  bool end(const std::wstring& path, std::string& err) override;

 private:
  std::string body_;
  std::vector<ReportFigure> images_;
};

}  // namespace fpe
