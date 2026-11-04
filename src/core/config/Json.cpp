#include "config/Json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace fpe {

Json& Json::operator[](const std::string& key) {
  if (type_ != Type::Object) {
    *this = object();
  }
  for (auto& kv : obj_)
    if (kv.first == key) return kv.second;
  obj_.emplace_back(key, Json());
  return obj_.back().second;
}

const Json& Json::get(const std::string& key) const {
  static const Json kNull;
  if (type_ != Type::Object) return kNull;
  for (auto& kv : obj_)
    if (kv.first == key) return kv.second;
  return kNull;
}

bool Json::has(const std::string& key) const {
  if (type_ != Type::Object) return false;
  for (auto& kv : obj_)
    if (kv.first == key) return true;
  return false;
}

void Json::push(Json v) {
  if (type_ != Type::Array) *this = array();
  arr_.push_back(std::move(v));
}

namespace {
void escape(std::string& out, const std::string& s) {
  out += '"';
  for (unsigned char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          char b[8];
          snprintf(b, sizeof b, "\\u%04x", c);
          out += b;
        } else {
          out += (char)c;
        }
    }
  }
  out += '"';
}
}  // namespace

void Json::dumpTo(std::string& out, int indent, int level) const {
  auto nl = [&](int lv) {
    if (indent <= 0) return;
    out += '\n';
    out.append((size_t)(lv * indent), ' ');
  };
  switch (type_) {
    case Type::Null: out += "null"; break;
    case Type::Bool: out += b_ ? "true" : "false"; break;
    case Type::Number: {
      char b[64];
      if (std::floor(n_) == n_ && std::fabs(n_) < 1e15)
        snprintf(b, sizeof b, "%lld", (long long)n_);
      else
        snprintf(b, sizeof b, "%.10g", n_);
      out += b;
      break;
    }
    case Type::String: escape(out, s_); break;
    case Type::Array:
      out += '[';
      for (size_t i = 0; i < arr_.size(); ++i) {
        if (i) out += ',';
        nl(level + 1);
        arr_[i].dumpTo(out, indent, level + 1);
      }
      if (!arr_.empty()) nl(level);
      out += ']';
      break;
    case Type::Object:
      out += '{';
      for (size_t i = 0; i < obj_.size(); ++i) {
        if (i) out += ',';
        nl(level + 1);
        escape(out, obj_[i].first);
        out += indent > 0 ? ": " : ":";
        obj_[i].second.dumpTo(out, indent, level + 1);
      }
      if (!obj_.empty()) nl(level);
      out += '}';
      break;
  }
}

std::string Json::dump(int indent) const {
  std::string s;
  dumpTo(s, indent, 0);
  return s;
}

namespace {
struct Parser {
  const std::string& t;
  size_t i = 0;
  std::string err;

  void ws() {
    while (i < t.size() && (t[i] == ' ' || t[i] == '\n' || t[i] == '\r' || t[i] == '\t')) ++i;
  }
  bool fail(const char* m) {
    if (err.empty()) err = std::string(m) + " at " + std::to_string(i);
    return false;
  }
  bool str(std::string& out) {
    if (t[i] != '"') return fail("expected string");
    ++i;
    while (i < t.size() && t[i] != '"') {
      char c = t[i++];
      if (c == '\\' && i < t.size()) {
        char e = t[i++];
        switch (e) {
          case 'n': out += '\n'; break;
          case 't': out += '\t'; break;
          case 'r': out += '\r'; break;
          case 'b': out += '\b'; break;
          case 'f': out += '\f'; break;
          case 'u': {
            if (i + 4 > t.size()) return fail("bad \\u");
            unsigned cp = (unsigned)strtoul(t.substr(i, 4).c_str(), nullptr, 16);
            i += 4;
            if (cp < 0x80) out += (char)cp;
            else if (cp < 0x800) { out += (char)(0xC0 | (cp >> 6)); out += (char)(0x80 | (cp & 0x3F)); }
            else { out += (char)(0xE0 | (cp >> 12)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
            break;
          }
          default: out += e;
        }
      } else {
        out += c;
      }
    }
    if (i >= t.size()) return fail("unterminated string");
    ++i;
    return true;
  }
  bool value(Json& out) {
    ws();
    if (i >= t.size()) return fail("unexpected end");
    char c = t[i];
    if (c == '{') {
      ++i;
      out = Json::object();
      ws();
      if (i < t.size() && t[i] == '}') { ++i; return true; }
      for (;;) {
        ws();
        std::string k;
        if (!str(k)) return false;
        ws();
        if (i >= t.size() || t[i] != ':') return fail("expected ':'");
        ++i;
        Json v;
        if (!value(v)) return false;
        out[k] = std::move(v);
        ws();
        if (i < t.size() && t[i] == ',') { ++i; continue; }
        if (i < t.size() && t[i] == '}') { ++i; return true; }
        return fail("expected ',' or '}'");
      }
    }
    if (c == '[') {
      ++i;
      out = Json::array();
      ws();
      if (i < t.size() && t[i] == ']') { ++i; return true; }
      for (;;) {
        Json v;
        if (!value(v)) return false;
        out.push(std::move(v));
        ws();
        if (i < t.size() && t[i] == ',') { ++i; continue; }
        if (i < t.size() && t[i] == ']') { ++i; return true; }
        return fail("expected ',' or ']'");
      }
    }
    if (c == '"') {
      std::string s;
      if (!str(s)) return false;
      out = Json(std::move(s));
      return true;
    }
    if (t.compare(i, 4, "true") == 0) { i += 4; out = Json(true); return true; }
    if (t.compare(i, 5, "false") == 0) { i += 5; out = Json(false); return true; }
    if (t.compare(i, 4, "null") == 0) { i += 4; out = Json(); return true; }
    char* end = nullptr;
    double d = strtod(t.c_str() + i, &end);
    if (end == t.c_str() + i) return fail("bad value");
    i = (size_t)(end - t.c_str());
    out = Json(d);
    return true;
  }
};
}  // namespace

bool Json::parse(const std::string& text, Json& out, std::string& err) {
  Parser p{text};
  if (text.size() >= 3 && (unsigned char)text[0] == 0xEF) p.i = 3;  // UTF-8 BOM
  if (!p.value(out)) {
    err = p.err;
    return false;
  }
  return true;
}

}  // namespace fpe
