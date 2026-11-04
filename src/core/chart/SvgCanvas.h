#pragma once
#include <string>

#include "chart/ICanvas.h"

namespace fpe {

class SvgCanvas : public ICanvas {
 public:
  SvgCanvas(std::wstring path, int width, int height);
  void line(double x1, double y1, double x2, double y2, Color c, double width, bool dashed) override;
  void polyline(const std::vector<Pt>& pts, Color c, double width) override;
  void polygon(const std::vector<Pt>& pts, Color fill, double alpha) override;
  void rect(double x, double y, double w, double h, Color stroke, double width) override;
  void fillRect(double x, double y, double w, double h, Color fill, double alpha) override;
  void circle(double cx, double cy, double r, Color fill) override;
  void text(double x, double y, const std::wstring& s, double size, Color c, Align a, bool bold,
            bool vertical) override;
  bool finish() override;

 private:
  std::wstring path_;
  std::string body_;
  int w_, h_;
};

}  // namespace fpe
