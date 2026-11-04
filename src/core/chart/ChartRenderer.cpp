#include "chart/ChartRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace fpe {

namespace {
// 그림 영역
constexpr double L = 78, T = 46, R = 612, B = 420;

std::wstring pct(double v, int prec = 2) {
  wchar_t b[32];
  swprintf(b, 32, L"%.*f%%", prec, v * 100);
  return b;
}

void frame(ICanvas& c, const std::wstring& title, const std::wstring& xl, const std::wstring& yl) {
  c.text((L + R) / 2, 28, title, 15, colors::Ink, Align::Center, true);
  c.text((L + R) / 2, 462, xl, 12, colors::Axis, Align::Center);
  c.text(22, (T + B) / 2, yl, 12, colors::Axis, Align::Center, false, true);
}

void legend(ICanvas& c, double x, double y, const std::vector<std::pair<Color, std::wstring>>& items) {
  double w = 0;
  for (auto& it : items) w = std::max(w, 7.0 * it.second.size() + 34);
  c.fillRect(x, y, w, 22.0 * items.size() + 8, Color{255, 255, 255}, 0.92);
  c.rect(x, y, w, 22.0 * items.size() + 8, colors::Grid, 1);
  for (size_t i = 0; i < items.size(); ++i) {
    double yy = y + 18 + 22.0 * i;
    c.fillRect(x + 8, yy - 9, 16, 10, items[i].first, 0.85);
    c.text(x + 30, yy, items[i].second, 11, colors::Ink, Align::Left);
  }
}
}  // namespace

void ChartRenderer::drawDET(ICanvas& c, const MetricsResult& m) const {
  // 축 범위: 표본 수에 맞춘 최소 decade (예: impostor 5,760건 → 10^-4)
  double nmax = (double)std::max<size_t>({m.nG, m.nI, 10});
  int dmin = std::clamp((int)std::floor(std::log10(1.0 / nmax)), -6, -2);
  auto px = [&](double v) {
    double lv = std::log10(std::max(v, std::pow(10.0, dmin)));
    return L + (lv - dmin) / (0 - dmin) * (R - L);
  };
  auto py = [&](double v) {
    double lv = std::log10(std::max(v, std::pow(10.0, dmin)));
    return B - (lv - dmin) / (0 - dmin) * (B - T);
  };

  frame(c, L"DET Curve — " + title_, L"FMR", L"FNMR");
  for (int d = dmin; d <= 0; ++d) {
    for (int k = 1; k < 10 && d < 0; ++k) {
      double v = k * std::pow(10.0, d);
      c.line(px(v), T, px(v), B, colors::Grid, k == 1 ? 1.0 : 0.5);
      c.line(L, py(v), R, py(v), colors::Grid, k == 1 ? 1.0 : 0.5);
    }
    wchar_t lab[16];
    swprintf(lab, 16, L"1e%d", d);
    c.text(px(std::pow(10.0, d)), B + 18, d == 0 ? L"1" : lab, 11, colors::Axis, Align::Center);
    c.text(L - 8, py(std::pow(10.0, d)) + 4, d == 0 ? L"1" : lab, 11, colors::Axis, Align::Right);
  }
  c.rect(L, T, R - L, B - T, colors::Axis, 1);
  c.line(px(std::pow(10.0, dmin)), py(std::pow(10.0, dmin)), px(1), py(1), colors::Guide, 1, true);  // FMR = FNMR

  // 고정 FMR 기준선
  struct Ref { double fmr; const wchar_t* name; double val; };
  Ref refs[] = {{1e-2, L"FMR100", m.fmr100}, {1e-3, L"FMR1000", m.fmr1000}, {1e-4, L"FMR10000", m.fmr10000}};
  for (auto& rf : refs) {
    if (std::log10(rf.fmr) < dmin) continue;
    c.line(px(rf.fmr), T, px(rf.fmr), B, colors::Axis, 1, true);
    c.text(px(rf.fmr) + 4, T + 14, rf.name, 10, colors::Axis, Align::Left);
    c.circle(px(rf.fmr), py(rf.val), 3.5, colors::Ink);
  }

  std::vector<Pt> pts;
  for (const auto& p : m.det) pts.push_back({px(p.fmr), py(p.fnmr)});
  c.polyline(pts, colors::Curve, 2);
  c.circle(px(m.eer), py(m.eer), 5, colors::Impostor);
  c.text(px(m.eer) + 9, py(m.eer) - 6, L"EER " + pct(m.eer), 11, colors::Impostor, Align::Left, true);
}

void ChartRenderer::drawDistribution(ICanvas& c, const MetricsResult& m) const {
  double maxv = 1e-9;
  for (double v : m.histG) maxv = std::max(maxv, v);
  for (double v : m.histI) maxv = std::max(maxv, v);
  maxv *= 1.1;
  auto px = [&](double s) { return L + s * (R - L); };
  auto py = [&](double v) { return B - v / maxv * (B - T); };

  frame(c, L"Score Distribution — " + title_, L"Similarity score", L"비율");
  for (int k = 0; k <= 10; ++k) {
    double s = k / 10.0;
    c.line(px(s), T, px(s), B, colors::Grid, 0.6);
    wchar_t b[8];
    swprintf(b, 8, L"%.1f", s);
    c.text(px(s), B + 18, b, 11, colors::Axis, Align::Center);
  }
  for (int k = 0; k <= 4; ++k) {
    double v = maxv * k / 4;
    c.line(L, py(v), R, py(v), colors::Grid, 0.6);
    c.text(L - 8, py(v) + 4, pct(v, 1), 10, colors::Axis, Align::Right);
  }

  auto bars = [&](const std::vector<double>& h, Color col) {
    std::vector<Pt> step;
    step.push_back({px(0), B});
    for (size_t i = 0; i < h.size(); ++i) {
      step.push_back({px(i / 100.0), py(h[i])});
      step.push_back({px((i + 1) / 100.0), py(h[i])});
    }
    step.push_back({px(1), B});
    c.polygon(step, col, 0.45);
    return step;
  };
  auto si = bars(m.histI, colors::Impostor);
  auto sg = bars(m.histG, colors::Genuine);
  c.polyline(si, colors::Impostor, 1.2);
  c.polyline(sg, colors::Genuine, 1.2);
  c.rect(L, T, R - L, B - T, colors::Axis, 1);
  legend(c, L + 12, T + 10,
         {{colors::Genuine, L"Genuine (n=" + std::to_wstring(m.gOk) + L")"},
          {colors::Impostor, L"Impostor (n=" + std::to_wstring(m.iOk) + L")"}});
}

void ChartRenderer::drawRates(ICanvas& c, const MetricsResult& m) const {
  auto px = [&](double t) { return L + t * (R - L); };
  auto py = [&](double v) { return B - v * (B - T); };
  frame(c, L"FMR(t) and FNMR(t) — " + title_, L"threshold", L"Error rate");
  for (int k = 0; k <= 10; ++k) {
    double s = k / 10.0;
    c.line(px(s), T, px(s), B, colors::Grid, 0.6);
    c.line(L, py(s), R, py(s), colors::Grid, 0.6);
    wchar_t b[8];
    swprintf(b, 8, L"%.1f", s);
    c.text(px(s), B + 18, b, 11, colors::Axis, Align::Center);
    if (k % 2 == 0) c.text(L - 8, py(s) + 4, pct(s, 0), 10, colors::Axis, Align::Right);
  }
  std::vector<Pt> fm, fn;
  for (const auto& p : m.rates) {
    fm.push_back({px(p.t), py(p.fmr)});
    fn.push_back({px(p.t), py(p.fnmr)});
  }
  c.polyline(fm, colors::Impostor, 2);
  c.polyline(fn, colors::Genuine, 2);
  double et = std::min(1.0, m.eerT);
  c.line(px(et), T, px(et), B, colors::Guide, 1, true);
  c.circle(px(et), py(m.eer), 5, colors::Ink);
  c.rect(L, T, R - L, B - T, colors::Axis, 1);
  wchar_t eerLabel[64];
  swprintf(eerLabel, 64, L"EER=%.2f%% @ t=%.3f", m.eer * 100, et);
  legend(c, R - 200, B - 84,
         {{colors::Impostor, L"FMR(t)"}, {colors::Genuine, L"FNMR(t)"}, {colors::Ink, eerLabel}});
}

}  // namespace fpe
