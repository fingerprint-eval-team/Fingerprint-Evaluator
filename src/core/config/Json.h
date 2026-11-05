#pragma once
// 설정 파일용 경량 JSON (객체·배열·문자열·숫자·불리언·null)
#include <string>
#include <utility>
#include <vector>

namespace fpe {

class Json {
 public:
  enum class Type { Null, Bool, Number, String, Array, Object };

  Json() = default;
  Json(bool b) : type_(Type::Bool), b_(b) {}
  Json(double n) : type_(Type::Number), n_(n) {}
  Json(int n) : type_(Type::Number), n_(n) {}
  Json(long long n) : type_(Type::Number), n_(static_cast<double>(n)) {}
  Json(const char* s) : type_(Type::String), s_(s) {}
  Json(std::string s) : type_(Type::String), s_(std::move(s)) {}
  static Json array() {
    Json j;
    j.type_ = Type::Array;
    return j;
  }
  static Json object() {
    Json j;
    j.type_ = Type::Object;
    return j;
  }

  Type type() const { return type_; }
  bool isNull() const { return type_ == Type::Null; }

  bool asBool(bool def = false) const { return type_ == Type::Bool ? b_ : def; }
  double asNumber(double def = 0) const { return type_ == Type::Number ? n_ : def; }
  int asInt(int def = 0) const { return type_ == Type::Number ? static_cast<int>(n_) : def; }
  std::string asString(const std::string& def = "") const { return type_ == Type::String ? s_ : def; }

  // 객체
  Json& operator[](const std::string& key);
  const Json& get(const std::string& key) const;
  bool has(const std::string& key) const;
  // 배열
  void push(Json v);
  size_t size() const { return type_ == Type::Array ? arr_.size() : obj_.size(); }
  const Json& at(size_t i) const { return arr_[i]; }

  std::string dump(int indent = 2) const;
  static bool parse(const std::string& text, Json& out, std::string& err);

 private:
  void dumpTo(std::string& out, int indent, int level) const;
  Type type_ = Type::Null;
  bool b_ = false;
  double n_ = 0;
  std::string s_;
  std::vector<Json> arr_;
  std::vector<std::pair<std::string, Json>> obj_;  // 입력 순서 유지
};

}  // namespace fpe
