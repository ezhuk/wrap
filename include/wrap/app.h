#pragma once

#include <folly/json/JSONSchema.h>
#include <folly/json/json.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace wrap {
class Request final {
public:
  Request(proxygen::HTTPMessage const* msg, folly::IOBuf* body) : msg_(msg), body_(body) {}
  ~Request() = default;

  std::string getMethod() const { return msg_->getMethodString(); }

  std::string getURL() const { return msg_->getURL(); }

  std::string getParam(std::string const& name) const {
    auto iter = params_.find(name);
    if (params_.end() != iter) {
      return iter->second;
    }
    return "";
  }

  void setParam(std::string const& name, std::string const& data) { params_[name] = data; }

  std::string getQueryParam(std::string const& name) const {
    return msg_->getDecodedQueryParam(name);
  }

  std::string body() const { return body_ ? body_->toString() : std::string{}; }

  folly::dynamic json() const { return folly::parseJson(body()); }

private:
  proxygen::HTTPMessage const* msg_;
  folly::IOBuf* body_;
  std::unordered_map<std::string, std::string> params_;
};

class Response final {
public:
  explicit Response(proxygen::ResponseBuilder* res) : res_(res) {}
  ~Response() = default;

  Response& status(std::uint16_t code, std::string const& message) {
    res_->status(code, message);
    return *this;
  }

  Response& header(std::string const& name, std::string const& data) {
    res_->header(name, data);
    return *this;
  }

  Response& body(std::string const& body) {
    res_->body(body);
    return *this;
  }

private:
  proxygen::ResponseBuilder* res_;
};

template <typename T>
struct IdPolicy {
  static std::string generate() { return fmt::format("{}", folly::Random::rand64()); }
};

template <class Owner, class Member>
struct field_t {
  std::string_view name;
  Member Owner::* ptr;
};

template <class Owner, class Member>
constexpr field_t<Owner, Member> field(std::string_view name, Member Owner::* ptr) {
  return {name, ptr};
}

namespace detail {
template <class Tuple, class Fn, std::size_t... I>
constexpr void for_each_impl(Tuple&& t, Fn&& fn, std::index_sequence<I...>) {
  (fn(std::get<I>(t)), ...);
}

template <class Tuple, class Fn>
constexpr void for_each(Tuple&& t, Fn&& fn) {
  constexpr std::size_t N = std::tuple_size_v<std::remove_reference_t<Tuple>>;
  for_each_impl(std::forward<Tuple>(t), std::forward<Fn>(fn), std::make_index_sequence<N>{});
}

template <class T>
struct is_optional : std::false_type {};
template <class U>
struct is_optional<std::optional<U>> : std::true_type {};
template <class T>
inline constexpr bool is_optional_v = is_optional<T>::value;

template <class T>
struct unwrap_optional {
  using type = T;
};
template <class U>
struct unwrap_optional<std::optional<U>> {
  using type = U;
};
template <class T>
using unwrap_optional_t = typename unwrap_optional<T>::type;

template <class T>
struct is_vector : std::false_type {};
template <class U, class A>
struct is_vector<std::vector<U, A>> : std::true_type {};
template <class T>
inline constexpr bool is_vector_v = is_vector<T>::value;
template <class T>
struct vector_value;
template <class U, class A>
struct vector_value<std::vector<U, A>> {
  using type = U;
};
template <class T>
using vector_value_t = typename vector_value<T>::type;

template <class T>
struct is_umap_str : std::false_type {};
template <class V, class H, class E, class A>
struct is_umap_str<std::unordered_map<std::string, V, H, E, A>> : std::true_type {};
template <class T>
inline constexpr bool is_umap_str_v = is_umap_str<T>::value;
template <class T>
struct umap_value;
template <class V, class H, class E, class A>
struct umap_value<std::unordered_map<std::string, V, H, E, A>> {
  using type = V;
};
template <class T>
using umap_value_t = typename umap_value<T>::type;

inline bool from_dynamic(std::string& out, folly::dynamic const& d) {
  if (!d.isString()) return false;
  out = d.asString();
  return true;
}

template <class U>
bool from_dynamic(std::optional<U>& out, folly::dynamic const& d) {
  if (d.isNull()) {
    out.reset();
    return true;
  }
  U tmp{};
  if (!from_dynamic(tmp, d)) return false;
  out = std::move(tmp);
  return true;
}

template <class U>
bool from_dynamic(std::vector<U>& out, folly::dynamic const& d) {
  if (!d.isArray()) return false;
  out.clear();
  out.reserve(d.size());
  for (auto const& el : d) {
    U tmp{};
    if (!from_dynamic(tmp, el)) return false;
    out.push_back(std::move(tmp));
  }
  return true;
}

inline bool from_dynamic(
    std::unordered_map<std::string, std::string>& out, folly::dynamic const& d
) {
  if (!d.isObject()) return false;
  out.clear();
  for (auto const& it : d.items()) {
    if (!it.second.isString()) return false;
    out.emplace(it.first.asString(), it.second.asString());
  }
  return true;
}

inline folly::dynamic to_dynamic(std::string const& v) { return folly::dynamic(v); }

template <class U>
folly::dynamic to_dynamic(std::optional<U> const& v) {
  if (!v) return nullptr;
  return to_dynamic(*v);
}

template <class U>
folly::dynamic to_dynamic(std::vector<U> const& v) {
  folly::dynamic arr = folly::dynamic::array;
  for (auto const& el : v) arr.push_back(to_dynamic(el));
  return arr;
}

inline folly::dynamic to_dynamic(std::unordered_map<std::string, std::string> const& m) {
  folly::dynamic obj = folly::dynamic::object;
  for (auto const& [k, v] : m) obj[k] = v;
  return obj;
}

template <class T>
folly::dynamic schema_for_type();

template <class T>
folly::dynamic schema_for_object() {
  folly::dynamic root = folly::dynamic::object;
  root["type"] = "object";

  folly::dynamic props = folly::dynamic::object;
  folly::dynamic required = folly::dynamic::array;

  auto tup = T::meta();
  for_each(tup, [&](auto const& f) {
    using Owner = T;
    using MemberT = std::remove_reference_t<decltype(std::declval<Owner&>().*(f.ptr))>;

    props[f.name.data()] = schema_for_type<MemberT>();

    if (!is_optional_v<MemberT>) {
      required.push_back(f.name.data());
    }
  });

  root["properties"] = std::move(props);
  if (required.size() > 0) root["required"] = std::move(required);
  return root;
}

template <class T>
folly::dynamic schema_for_type() {
  using U0 = unwrap_optional_t<std::remove_cvref_t<T>>;
  if constexpr (requires { U0::meta(); }) {
    return schema_for_object<U0>();
  } else if constexpr (is_vector_v<U0>) {
    return folly::dynamic::object("type", "array")("items", schema_for_type<vector_value_t<U0>>());
  } else if constexpr (is_umap_str_v<U0>) {
    return folly::dynamic::object("type", "object")(
        "additionalProperties", schema_for_type<umap_value_t<U0>>()
    );
  } else if constexpr (std::is_same_v<U0, std::string>) {
    return folly::dynamic::object("type", "string");
  } else if constexpr (std::is_same_v<U0, bool>) {
    return folly::dynamic::object("type", "boolean");
  } else if constexpr (std::is_integral_v<U0>) {
    return folly::dynamic::object("type", "integer");
  } else if constexpr (std::is_floating_point_v<U0>) {
    return folly::dynamic::object("type", "number");
  } else {
    static_assert(sizeof(U0) == 0, "No JSON schema mapping for this type");
  }
}
}  // namespace detail

template <typename T>
class Model {
public:
  static std::optional<T> parse(std::string const& str) {
    try {
      auto const json = folly::parseJson(str);
      static auto const validator = [] {
        folly::dynamic s = detail::schema_for_object<T>();
        return folly::jsonschema::makeValidator(s);
      }();
      validator->validate(json);
      auto obj = make_from_meta(json);
      if (!obj) return std::nullopt;
      auto& self = *static_cast<Model*>(&(*obj));
      if (json.count("_id"))
        self.id_ = json["_id"].asString();
      else
        self.id_ = IdPolicy<T>::generate();
      return obj;
    } catch (...) {
      return std::nullopt;
    }
  }

  std::string const& _id() const { return id_; }

  folly::dynamic dump() const { return dump_from_meta(*static_cast<T const*>(this)); }

protected:
  std::string id_;

private:
  static std::optional<T> make_from_meta(folly::dynamic const& d) {
    if (!d.isObject()) return std::nullopt;

    T obj{};
    bool ok = true;

    auto tup = T::meta();
    detail::for_each(tup, [&](auto const& f) {
      using Owner = T;
      using MemberT = std::remove_reference_t<decltype(std::declval<Owner&>().*(f.ptr))>;

      auto it = d.find(f.name.data());
      if (it == d.items().end()) {
        if (!detail::is_optional_v<MemberT>) ok = false;
        return;
      }

      ok = ok && detail::from_dynamic(obj.*(f.ptr), it->second);
    });

    if (!ok) return std::nullopt;
    return obj;
  }

  static folly::dynamic dump_from_meta(T const& obj) {
    folly::dynamic out = folly::dynamic::object;
    out["_id"] = obj._id();
    auto tup = T::meta();
    detail::for_each(tup, [&](auto const& f) {
      auto const& v = obj.*(f.ptr);
      using V = std::remove_reference_t<decltype(v)>;

      if constexpr (detail::is_optional_v<V>) {
        if (v) out[f.name.data()] = detail::to_dynamic(v);
      } else {
        out[f.name.data()] = detail::to_dynamic(v);
      }
    });
    return out;
  }
};

namespace detail {
inline void write_json(Response& res, folly::dynamic const& d) {
  res.status(200, "OK").header("Content-Type", "application/json").body(folly::toJson(d));
}

template <class T>
folly::dynamic to_response(T const& v) {
  if constexpr (std::is_base_of_v<Model<T>, T>) {
    return v.dump();
  } else {
    static_assert(sizeof(T) == 0, "Unsupported response type");
  }
}

template <class T>
folly::dynamic to_response(std::vector<T> const& v) {
  folly::dynamic arr = folly::dynamic::array;
  for (auto const& el : v) {
    arr.push_back(to_response(el));
  }
  return arr;
}
}  // namespace detail

class AppOptions {
public:
  std::string host{"0.0.0.0"};
  std::uint16_t port{8080};
  std::size_t threads{0};
};

class App final {
public:
  using Handler = std::function<void(Request const&, Response&)>;

  struct Route {
    proxygen::HTTPMethod method;
    std::string path;
    Handler handler;
  };

  explicit App(AppOptions options = {});
  ~App() = default;

  App& post(std::string const& path, Handler handler);
  App& put(std::string const& path, Handler handler);
  App& get(std::string const& path, Handler handler);

  template <typename T, typename F>
  App& get(std::string const& path, F&& func) {
    return get(path, [f = std::forward<F>(func)](Request const& req, Response& res) {
      try {
        if constexpr (std::is_invocable_v<F, std::string const&>) {
          auto id = req.getParam("id");
          auto out = f(id);
          detail::write_json(res, detail::to_response(out));
        } else if constexpr (std::is_invocable_v<F>) {
          auto out = f();
          detail::write_json(res, detail::to_response(out));
        } else {
          static_assert(sizeof(F) == 0, "Unsupported GET handler signature");
        }
      } catch (...) {
        res.status(500, "Internal Server Error").body(R"({"error":"Error processing request"})");
      }
    });
  }

  template <typename T, typename F>
  App& post(std::string const& path, F&& func) {
    return post(path, [f = std::forward<F>(func)](Request const& req, Response& res) {
      auto const obj = T::parse(req.body());
      if (!obj) {
        res.status(422, "Unprocessable Entity").body(R"({"error":"Could not parse request data"})");
        return;
      }
      try {
        f(*obj);
      } catch (...) {
        res.status(500, "Internal Server Error")
            .body(R"({"error":"Error processing the request"})");
        return;
      }
      res.status(201, "Created")
          .header("Location", obj->_id())
          .header("Content-Type", "application/json")
          .body(folly::toJson(obj->dump()));
    });
  }

  template <typename T, typename F>
  App& put(std::string const& path, F&& func) {
    return put(path, [f = std::forward<F>(func)](Request const& req, Response& res) {
      auto const obj = T::parse(req.body());
      if (!obj) {
        res.status(422, "Unprocessable Entity").body(R"({"error":"Could not parse request data"})");
        return;
      }
      std::string const id = req.getParam("id");
      try {
        if constexpr (std::is_void_v<std::invoke_result_t<F, std::string const&, T const&>>) {
          f(id, *obj);
          res.status(200, "OK")
              .header("Content-Type", "application/json")
              .body(folly::toJson(obj->dump()));
        } else {
          auto out = f(id, *obj);
          res.status(200, "OK")
              .header("Content-Type", "application/json")
              .body(folly::toJson(out.dump()));
        }
      } catch (...) {
        res.status(500, "Internal Server Error")
            .body(R"({"error":"Error processing the request"})");
      }
    });
  }

  void run(std::string const& host, std::uint16_t port);

  void run();

private:
  AppOptions options_;
  std::unique_ptr<proxygen::HTTPServer> server_;
  std::vector<Route> routes_;
};
}  // namespace wrap
