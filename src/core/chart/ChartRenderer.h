#pragma once
#include <string>

#include "chart/ICanvas.h"
#include "metrics/MetricsCalculator.h"

namespace fpe {

class ChartRenderer {
 public:
  static constexpr int W = 640, H = 480;
  explicit ChartRenderer(std::wstring title) : title_(std::move(title)) {}

  void drawDET(ICanvas& c, const MetricsResult& m) const;           // DET Curve (log-log)
  void drawDistribution(ICanvas& c, const MetricsResult& m) const;  // Score Distribution
  void drawRates(ICanvas& c, const MetricsResult& m) const;         // FMR(t), FNMR(t)

 private:
  std::wstring title_;
};

}  // namespace fpe
