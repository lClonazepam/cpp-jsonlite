#include "jsonlite.hpp"
#include <cassert>
#include <iostream>

int main() {
  auto j = kit::Json::parse(R"({"ok":true,"n":3.5,"tags":["a","b"],"z":null})");
  assert(j.at("ok").is_bool());
  assert(std::get<bool>(j.at("ok").v));
  assert(std::get<double>(j.at("n").v) == 3.5);
  assert(std::get<kit::Json::Array>(j.at("tags").v).size() == 2);
  auto again = kit::Json::parse(j.dump());
  assert(again.at("tags").is_array());
  kit::Json built = kit::Json::object();
  built["msg"] = kit::Json::str("hi");
  assert(built.dump().find("hi") != std::string::npos);
  std::cout << "jsonlite ok " << j.dump() << "\n";
}
