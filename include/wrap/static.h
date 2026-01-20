#pragma once

#include "app.h"

namespace wrap {
inline std::string mime_type(std::string const& ext) {
  if (ext == ".html") {
    return "text/html";
  }
  if (ext == ".css") {
    return "text/css";
  }
  if (ext == ".js") {
    return "application/javascript";
  }
  if (ext == ".json") {
    return "application/json";
  }
  if (ext == ".png") {
    return "image/png";
  }
  if (ext == ".jpg" || ext == ".jpeg") {
    return "image/jpeg";
  }
  if (ext == ".txt") {
    return "text/plain";
  }
  return "application/octet-stream";
}

inline App::Handler serve_static(std::filesystem::path root) {
  return [root = std::move(root)](Request const& req, Response& res) {
    namespace fs = std::filesystem;
    std::string path = req.getURL();
    if (auto q = path.find('?'); q != std::string::npos) {
      path.resize(q);
    }
    if (path.find("..") != std::string::npos) {
      detail::send_error(res, 400, "Bad Request");
      return;
    }
    fs::path file = root;
    if (path == "/" || path.empty()) {
      file /= "index.html";
    } else {
      file /= path.substr(1);
    }
    if (!fs::exists(file) || !fs::is_regular_file(file)) {
      detail::send_error(res, 404, "Not Found");
      return;
    }
    std::ifstream in(file, std::ios::binary);
    if (!in) {
      detail::send_error(res, 500, "Internal Server Error");
      return;
    }
    std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    res.status(200, "OK").header("Content-Type", mime_type(file.extension().string())).body(body);
  };
}
}  // namespace wrap
