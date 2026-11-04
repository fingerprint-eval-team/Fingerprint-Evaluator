#pragma once
// 그래프 그리기 인터페이스. ChartRenderer는 이 인터페이스에만 그리므로
// 같은 코드로 SVG(브라우저·HTML)와 EMF(Word 삽입) 벡터 그림이 동시에 나온다.
#include <cstdint>
#include <string>
#include <vector>

namespace fpe {

struct Color {
  uint8_t r, g, b;
};
struct Pt {
  double x, y;
};
enum class Align { Left, Center, Right };

class ICanvas {
 public:
  virtual ~ICanvas() = default;
  virtual void line(double x1, double y1, double x2, double y2, Color c, double width, bool dashed = false) = 0;
  virtual void polyline(const std::vector<Pt>& pts, Color c, double width) = 0;
  virtual void polygon(const std::vector<Pt>& pts, Color fill, double alpha) = 0;
  virtual void rect(double x, double y, double w, double h, Color stroke, double width) = 0;
  virtual void fillRect(double x, double y, double w, double h, Color fill, double alpha) = 0;
  virtual void circle(double cx, double cy, double r, Color fill) = 0;
  // y는 글자 기준선(baseline)
  virtual void text(double x, double y, const std::wstring& s, double size, Color c, Align a, bool bold = false,
                    bool vertical = false) = 0;
  virtual bool finish() = 0;
};

namespace colors {
constexpr Color Ink{30, 34, 44};
constexpr Color Grid{220, 224, 230};
constexpr Color Axis{90, 98, 112};
constexpr Color Genuine{46, 140, 87};
constexpr Color Impostor{214, 69, 65};
constexpr Color Curve{40, 90, 200};
constexpr Color Guide{120, 120, 120};
}  // namespace colors

}  // namespace fpe
