#pragma once
#include <windows.h>

#include <string>

#include "chart/ICanvas.h"

namespace fpe {

// GDI Enhanced Metafile(.emf) 벡터 그림. Word에 그대로 삽입된다.
// GDI는 투명도를 지원하지 않으므로 alpha는 흰 배경과 섞은 색으로 대신한다.
class EmfCanvas : public ICanvas {
 public:
  EmfCanvas(const std::wstring& path, int width, int height);
  ~EmfCanvas() override;
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
  // 소수점 좌표 정밀도를 위해 논리 좌표를 S배로 확대
  static constexpr int S = 10;
  int X(double v) const { return (int)(v * S + 0.5); }
  HDC dc_ = nullptr;
};

}  // namespace fpe
