# cpp-jsonlite

Small C++20 JSON parser and serializer. Objects, arrays, strings, numbers, bool, null. Good for configs and fixtures, not a full RFC parser (no \u escapes).

```cpp
auto doc = kit::Json::parse(R"({"port":8080})");
double port = std::get<double>(doc.at("port").v);
```

MIT
