#pragma once
#include <cctype>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace kit {

struct Json {
  using Object = std::map<std::string, Json>;
  using Array = std::vector<Json>;
  using Value = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;
  Value v = nullptr;

  static Json nul() { return Json{}; }
  static Json boolean(bool b) { Json j; j.v = b; return j; }
  static Json number(double n) { Json j; j.v = n; return j; }
  static Json str(std::string s) { Json j; j.v = std::move(s); return j; }
  static Json array(Array a = {}) { Json j; j.v = std::move(a); return j; }
  static Json object(Object o = {}) { Json j; j.v = std::move(o); return j; }

  bool is_null() const { return std::holds_alternative<std::nullptr_t>(v); }
  bool is_bool() const { return std::holds_alternative<bool>(v); }
  bool is_number() const { return std::holds_alternative<double>(v); }
  bool is_string() const { return std::holds_alternative<std::string>(v); }
  bool is_array() const { return std::holds_alternative<Array>(v); }
  bool is_object() const { return std::holds_alternative<Object>(v); }
  const Json& at(const std::string& key) const { return std::get<Object>(v).at(key); }
  Json& operator[](const std::string& key) {
    if (!is_object()) v = Object{};
    return std::get<Object>(v)[key];
  }
  void push(Json child) {
    if (!is_array()) v = Array{};
    std::get<Array>(v).push_back(std::move(child));
  }
  std::string dump() const { return dump_into(*this); }

  static Json parse(std::string_view in) {
    Parser p(in);
    Json j = p.parse_value();
    p.skip();
    if (!p.eof()) throw std::runtime_error("trailing data");
    return j;
  }

 private:
  struct Parser {
    std::string_view s; std::size_t i = 0;
    explicit Parser(std::string_view in) : s(in) {}
    bool eof() const { return i >= s.size(); }
    void skip() { while (!eof() && std::isspace(static_cast<unsigned char>(s[i]))) ++i; }
    char peek() { skip(); return eof() ? '\0' : s[i]; }
    char get() { skip(); if (eof()) throw std::runtime_error("unexpected end"); return s[i++]; }
    Json parse_value() {
      char c = peek();
      if (c == '{') return parse_object();
      if (c == '[') return parse_array();
      if (c == '"') return Json::str(parse_string());
      if (c == 't' || c == 'f') return Json::boolean(parse_bool());
      if (c == 'n') { parse_null(); return Json::nul(); }
      if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return Json::number(parse_number());
      throw std::runtime_error("invalid value");
    }
    Json parse_object() {
      get();
      Json::Object o;
      if (peek() == '}') { get(); return Json::object(std::move(o)); }
      while (true) {
        if (peek() != '"') throw std::runtime_error("expected key");
        auto key = parse_string();
        if (get() != ':') throw std::runtime_error("expected colon");
        o.emplace(std::move(key), parse_value());
        char n = get();
        if (n == '}') break;
        if (n != ',') throw std::runtime_error("expected comma");
      }
      return Json::object(std::move(o));
    }
    Json parse_array() {
      get();
      Json::Array a;
      if (peek() == ']') { get(); return Json::array(std::move(a)); }
      while (true) {
        a.push_back(parse_value());
        char n = get();
        if (n == ']') break;
        if (n != ',') throw std::runtime_error("expected comma");
      }
      return Json::array(std::move(a));
    }
    std::string parse_string() {
      if (get() != '"') throw std::runtime_error("expected string");
      std::string out;
      while (!eof()) {
        char c = s[i++];
        if (c == '"') return out;
        if (c == '\\') {
          if (eof()) throw std::runtime_error("bad escape");
          char e = s[i++];
          switch (e) {
            case '"': case '\\': case '/': out.push_back(e); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            default: throw std::runtime_error("unsupported escape");
          }
        } else out.push_back(c);
      }
      throw std::runtime_error("unterminated string");
    }
    bool parse_bool() {
      if (s.substr(i, 4) == "true") { i += 4; return true; }
      if (s.substr(i, 5) == "false") { i += 5; return false; }
      throw std::runtime_error("bad bool");
    }
    void parse_null() {
      if (s.substr(i, 4) != "null") throw std::runtime_error("bad null");
      i += 4;
    }
    double parse_number() {
      std::size_t start = i;
      if (s[i] == '-') ++i;
      while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i]=='.' || s[i]=='e' || s[i]=='E' || s[i]=='+' || s[i]=='-')) ++i;
      return std::stod(std::string(s.substr(start, i - start)));
    }
  };
  static std::string escape(const std::string& s) {
    std::string o; o.push_back('"');
    for (char c : s) {
      switch (c) {
        case '"': o += "\\\""; break;
        case '\\': o += "\\\\"; break;
        case '\n': o += "\\n"; break;
        case '\r': o += "\\r"; break;
        case '\t': o += "\\t"; break;
        default: o.push_back(c);
      }
    }
    o.push_back('"'); return o;
  }
  static std::string dump_into(const Json& j) {
    if (j.is_null()) return "null";
    if (j.is_bool()) return std::get<bool>(j.v) ? "true" : "false";
    if (j.is_number()) { std::ostringstream os; os.precision(15); os << std::get<double>(j.v); return os.str(); }
    if (j.is_string()) return escape(std::get<std::string>(j.v));
    if (j.is_array()) {
      std::string o = "["; bool first = true;
      for (const auto& c : std::get<Array>(j.v)) { if (!first) o += ","; first = false; o += dump_into(c); }
      return o + "]";
    }
    std::string o = "{"; bool first = true;
    for (const auto& [k, c] : std::get<Object>(j.v)) { if (!first) o += ","; first = false; o += escape(k) + ":" + dump_into(c); }
    return o + "}";
  }
};

}  // namespace kit
