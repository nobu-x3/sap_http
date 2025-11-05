#pragma once

#include "core/result.h"
#include "core/types.h"
#include <algorithm>
#include <chrono>
#include <future>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
namespace http {

enum class EMethod { GET, POST, PUT, DELETE, HEAD, PATCH, OPTIONS };

inline std::string method_to_string(EMethod m) {
    switch (m) {
    case EMethod::GET: return "GET";
    case EMethod::POST: return "POST";
    case EMethod::PUT: return "PUT";
    case EMethod::DELETE: return "DELETE";
    case EMethod::HEAD: return "HEAD";
    case EMethod::PATCH: return "PATCH";
    case EMethod::OPTIONS: return "OPTIONS";
    }
    return "GET";
}

struct URL {
    std::string scheme;
    std::string host;
    std::string port;
    std::string path;
    std::string query;

    static stl::result<URL> parse(std::string_view raw_url);
    std::string full_path() const { return path + query; }
};

struct Headers {
    std::map<std::string, std::string> data;

    void set(std::string_view key, std::string_view value);
    std::string get(std::string_view key) const;
    bool has(std::string_view key) const;
};

struct Request {
    EMethod method = EMethod::GET;
    URL url;
    Headers headers;
    std::string body;
    std::chrono::milliseconds timeout{30000};

    Request(http::EMethod m, http::URL u);
    void set_header(std::string_view key, std::string_view value);
    void set_body(std::string data);
};

struct Response {
    i32 status_code{0};
    std::string status_text;
    Headers headers;
    std::string body;
    Response() = default;
	Response(i32 code, std::string body_content = "");
    inline bool is_success() const { return status_code >= 200 && status_code < 300; }
};

class Client {
private:
    static stl::result<i32> connect_socket(const URL& u);
    static stl::result<> send_request(i32 sock, const Request& req);
    static stl::result<Response> read_response(i32 sock);

public:
    static std::future<stl::result<Response>> async_send(Request req);
    static stl::result<Response> send(const Request& req);
    static std::future<stl::result<Response>> get(std::string_view url_str);
    static std::future<stl::result<Response>> post(std::string_view url_str, std::string body);
};

} // namespace http