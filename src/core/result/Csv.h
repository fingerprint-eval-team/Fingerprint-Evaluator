#pragma once
#include <string>
#include <vector>

namespace fpe {

inline std::string csvField(const std::string& s) {
  if (s.find_first_of(",\"\n\r") == std::string::npos) return s;
  std::string o = "\"";
  for (char c : s) {
    if (c == '"') o += '"';
    o += (c == '\n' || c == '\r') ? ' ' : c;
  }
  return o + "\"";
}

inline std::string csvRow(const std::vector<std::string>& f) {
  std::string s;
  for (size_t i = 0; i < f.size(); ++i) {
    if (i) s += ',';
    s += csvField(f[i]);
  }
  return s + "\n";
}

inline std::vector<std::string> parseCsvLine(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  bool q = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (q) {
      if (c == '"') {
        if (i + 1 < line.size() && line[i + 1] == '"') {
          cur += '"';
          ++i;
        } else {
          q = false;
        }
      } else {
        cur += c;
      }
    } else if (c == '"') {
      q = true;
    } else if (c == ',') {
      out.push_back(cur);
      cur.clear();
    } else if (c != '\r') {
      cur += c;
    }
  }
  out.push_back(cur);
  return out;
}

// 텍스트 → 줄 목록 (빈 줄과 '#' 주석 제외)
inline std::vector<std::string> splitLines(const std::string& text) {
  std::vector<std::string> v;
  size_t p = 0;
  while (p < text.size()) {
    size_t e = text.find('\n', p);
    std::string l = text.substr(p, e == std::string::npos ? std::string::npos : e - p);
    p = e == std::string::npos ? text.size() : e + 1;
    if (!l.empty() && l.back() == '\r') l.pop_back();
    if (!l.empty() && l[0] != '#') v.push_back(l);
  }
  return v;
}

}  // namespace fpe
