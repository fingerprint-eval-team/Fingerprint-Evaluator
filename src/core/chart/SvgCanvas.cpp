#include "chart/SvgCanvas.h"

#include <cstdio>

#include "PathUtil.h"

namespace fpe {

namespace {
std::string rgb(Color c) {
  char b[16];
  snprintf(b, sizeof b, "#%02x%02x%02x", c.r, c.g, c.b);
  return b;
}
std::string num(double v) {
  char b[32];
  snprintf(b, sizeof b, "%.2f", v);
  return b;
}
std::string esc(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '&') o += "&amp;";
    else o += c;
  }
  return o;
}
std::string points(const std::vector<Pt>& pts) {
  std::string s;
  for (auto& p : pts) s += num(p.x) + "," + num(p.y) + " ";
  return s;
}
}  // namespace

SvgCanvas::SvgCanvas(std::wstring path, int width, int height) : path_(std::move(path)), w_(width), h_(height) {}

void SvgCanvas::line(double x1, double y1, double x2, double y2, Color c, double width, bool dashed) {
  body_ += "<line x1=\"" + num(x1) + "\" y1=\"" + num(y1) + "\" x2=\"" + num(x2) + "\" y2=\"" + num(y2) +
           "\" stroke=\"" + rgb(c) + "\" stroke-width=\"" + num(width) + "\"" +
           (dashed ? " stroke-dasharray=\"4 3\"" : "") + "/>\n";
}

void SvgCanvas::polyline(const std::vector<Pt>& pts, Color c, double width) {
  body_ += "<polyline fill=\"none\" stroke=\"" + rgb(c) + "\" stroke-width=\"" + num(width) +
           "\" stroke-linejoin=\"round\" points=\"" + points(pts) + "\"/>\n";
}

void SvgCanvas::polygon(const std::vector<Pt>& pts, Color fill, double alpha) {
  body_ += "<polygon fill=\"" + rgb(fill) + "\" fill-opacity=\"" + num(alpha) + "\" stroke=\"none\" points=\"" +
           points(pts) + "\"/>\n";
}

void SvgCanvas::rect(double x, double y, double w, double h, Color stroke, double width) {
  body_ += "<rect x=\"" + num(x) + "\" y=\"" + num(y) + "\" width=\"" + num(w) + "\" height=\"" + num(h) +
           "\" fill=\"none\" stroke=\"" + rgb(stroke) + "\" stroke-width=\"" + num(width) + "\"/>\n";
}

void SvgCanvas::fillRect(double x, double y, double w, double h, Color fill, double alpha) {
  body_ += "<rect x=\"" + num(x) + "\" y=\"" + num(y) + "\" width=\"" + num(w) + "\" height=\"" + num(h) +
           "\" fill=\"" + rgb(fill) + "\" fill-opacity=\"" + num(alpha) + "\"/>\n";
}

void SvgCanvas::circle(double cx, double cy, double r, Color fill) {
  body_ += "<circle cx=\"" + num(cx) + "\" cy=\"" + num(cy) + "\" r=\"" + num(r) + "\" fill=\"" + rgb(fill) + "\"/>\n";
}

void SvgCanvas::text(double x, double y, const std::wstring& s, double size, Color c, Align a, bool bold,
                     bool vertical) {
  const char* anchor = a == Align::Left ? "start" : a == Align::Center ? "middle" : "end";
  body_ += "<text x=\"" + num(x) + "\" y=\"" + num(y) + "\" font-size=\"" + num(size) + "\" fill=\"" + rgb(c) +
           "\" text-anchor=\"" + anchor + "\"" + (bold ? " font-weight=\"bold\"" : "") +
           (vertical ? " transform=\"rotate(-90 " + num(x) + " " + num(y) + ")\"" : "") + ">" + esc(toUtf8(s)) +
           "</text>\n";
}

bool SvgCanvas::finish() {
  std::string s = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" +
                  std::to_string(w_) + "\" height=\"" + std::to_string(h_) + "\" viewBox=\"0 0 " +
                  std::to_string(w_) + " " + std::to_string(h_) +
                  "\" font-family=\"Segoe UI, Malgun Gothic, sans-serif\">\n<rect width=\"100%\" height=\"100%\" "
                  "fill=\"#ffffff\"/>\n" +
                  body_ + "</svg>\n";
  return writeText(path_, s);
}

}  // namespace fpe
