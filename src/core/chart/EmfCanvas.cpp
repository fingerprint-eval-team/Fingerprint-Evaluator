#include "chart/EmfCanvas.h"

#include <cmath>

namespace fpe {

namespace {
COLORREF blend(Color c, double a) {
  auto mix = [a](uint8_t v) { return (BYTE)std::lround(v * a + 255 * (1 - a)); };
  return RGB(mix(c.r), mix(c.g), mix(c.b));
}
}  // namespace

EmfCanvas::EmfCanvas(const std::wstring& path, int width, int height) {
  // 프레임은 0.01mm 단위, 96dpi 기준 크기
  RECT frame{0, 0, width * 2540 / 96, height * 2540 / 96};
  HDC ref = GetDC(nullptr);
  dc_ = CreateEnhMetaFileW(ref, path.c_str(), &frame, L"FPEval\0Chart\0");
  if (dc_) {
    double pxPerHmmX = (double)GetDeviceCaps(ref, HORZRES) / (GetDeviceCaps(ref, HORZSIZE) * 100.0);
    double pxPerHmmY = (double)GetDeviceCaps(ref, VERTRES) / (GetDeviceCaps(ref, VERTSIZE) * 100.0);
    SetMapMode(dc_, MM_ANISOTROPIC);
    SetWindowExtEx(dc_, width * S, height * S, nullptr);
    SetViewportExtEx(dc_, (int)(frame.right * pxPerHmmX), (int)(frame.bottom * pxPerHmmY), nullptr);
    SetBkMode(dc_, TRANSPARENT);
    HBRUSH white = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RECT all{0, 0, width * S, height * S};
    FillRect(dc_, &all, white);
  }
  ReleaseDC(nullptr, ref);
}

EmfCanvas::~EmfCanvas() {
  if (dc_) DeleteEnhMetaFile(CloseEnhMetaFile(dc_));
}

void EmfCanvas::line(double x1, double y1, double x2, double y2, Color c, double width, bool dashed) {
  if (!dc_) return;
  LOGBRUSH lb{BS_SOLID, RGB(c.r, c.g, c.b), 0};
  HPEN pen = ExtCreatePen(PS_GEOMETRIC | (dashed ? PS_DASH : PS_SOLID) | PS_ENDCAP_FLAT, (DWORD)X(width), &lb, 0, nullptr);
  HGDIOBJ old = SelectObject(dc_, pen);
  MoveToEx(dc_, X(x1), X(y1), nullptr);
  LineTo(dc_, X(x2), X(y2));
  SelectObject(dc_, old);
  DeleteObject(pen);
}

void EmfCanvas::polyline(const std::vector<Pt>& pts, Color c, double width) {
  if (!dc_ || pts.size() < 2) return;
  std::vector<POINT> p;
  for (auto& q : pts) p.push_back({X(q.x), X(q.y)});
  LOGBRUSH lb{BS_SOLID, RGB(c.r, c.g, c.b), 0};
  HPEN pen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_JOIN_ROUND, (DWORD)X(width), &lb, 0, nullptr);
  HGDIOBJ old = SelectObject(dc_, pen);
  Polyline(dc_, p.data(), (int)p.size());
  SelectObject(dc_, old);
  DeleteObject(pen);
}

void EmfCanvas::polygon(const std::vector<Pt>& pts, Color fill, double alpha) {
  if (!dc_ || pts.size() < 3) return;
  std::vector<POINT> p;
  for (auto& q : pts) p.push_back({X(q.x), X(q.y)});
  HBRUSH br = CreateSolidBrush(blend(fill, alpha));
  HGDIOBJ ob = SelectObject(dc_, br), op = SelectObject(dc_, GetStockObject(NULL_PEN));
  Polygon(dc_, p.data(), (int)p.size());
  SelectObject(dc_, ob);
  SelectObject(dc_, op);
  DeleteObject(br);
}

void EmfCanvas::rect(double x, double y, double w, double h, Color stroke, double width) {
  std::vector<Pt> p{{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}, {x, y}};
  polyline(p, stroke, width);
}

void EmfCanvas::fillRect(double x, double y, double w, double h, Color fill, double alpha) {
  polygon({{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}, fill, alpha);
}

void EmfCanvas::circle(double cx, double cy, double r, Color fill) {
  if (!dc_) return;
  HBRUSH br = CreateSolidBrush(RGB(fill.r, fill.g, fill.b));
  HGDIOBJ ob = SelectObject(dc_, br), op = SelectObject(dc_, GetStockObject(NULL_PEN));
  Ellipse(dc_, X(cx - r), X(cy - r), X(cx + r), X(cy + r));
  SelectObject(dc_, ob);
  SelectObject(dc_, op);
  DeleteObject(br);
}

void EmfCanvas::text(double x, double y, const std::wstring& s, double size, Color c, Align a, bool bold,
                     bool vertical) {
  if (!dc_) return;
  HFONT f = CreateFontW(-X(size), 0, vertical ? 900 : 0, vertical ? 900 : 0, bold ? FW_BOLD : FW_NORMAL, 0, 0, 0,
                        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Malgun Gothic");
  HGDIOBJ old = SelectObject(dc_, f);
  SetTextColor(dc_, RGB(c.r, c.g, c.b));
  UINT al = TA_BASELINE | (a == Align::Left ? TA_LEFT : a == Align::Center ? TA_CENTER : TA_RIGHT);
  SetTextAlign(dc_, al);
  TextOutW(dc_, X(x), X(y), s.c_str(), (int)s.size());
  SelectObject(dc_, old);
  DeleteObject(f);
}

bool EmfCanvas::finish() {
  if (!dc_) return false;
  HENHMETAFILE h = CloseEnhMetaFile(dc_);
  dc_ = nullptr;
  if (!h) return false;
  DeleteEnhMetaFile(h);
  return true;
}

}  // namespace fpe
