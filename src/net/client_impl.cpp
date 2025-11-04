#include "net/http.h"
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace http {

stl::result<int> Client::connect_socket(const URL &u) {
  struct addrinfo hints{}, *res = nullptr;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(u.host.c_str(), u.port.c_str(), &hints, &res) != 0) {
    return stl::make_error<int>("Failed to resolve host: " + u.host);
  }
  int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sock < 0) {
    freeaddrinfo(res);
    return stl::make_error<int>("Failed to create socket");
  }
  if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
    freeaddrinfo(res);
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return stl::make_error<int>("Failed to connect to host");
  }
  freeaddrinfo(res);
  return sock;
}

stl::result<> Client::send_request(int sock, const Request &req) {
  std::ostringstream ss;
  ss << method_to_string(req.method) << " " << req.url.full_path()
     << " HTTP/1.1\r\n";
  ss << "Host: " << req.url.host << "\r\n";
  for (const auto &[key, value] : req.headers.data) {
    ss << key << ": " << value << "\r\n";
  }
  ss << "\r\n";
  if (!req.body.empty()) {
    ss << req.body;
  }
  std::string request_str = ss.str();
  size_t sent = 0;
  while (sent < request_str.size()) {
    int n =
        ::send(sock, request_str.c_str() + sent, request_str.size() - sent, 0);
    if (n <= 0) {
      return stl::make_error<>("Failed to send request");
    }
    sent += n;
  }
  return stl::result_success();
}

stl::result<Response> Client::read_response(int sock) {
  Response resp;
  std::string buffer;
  char chunk[4096];
  bool headers_done = false;
  size_t content_length = 0;
  bool is_chunked = false;
  while (true) {
    int n = recv(sock, chunk, sizeof(chunk), 0);
    if (n <= 0)
      break;
    buffer.append(chunk, n);
    if (!headers_done) {
      auto header_end = buffer.find("\r\n\r\n");
      if (header_end != std::string::npos) {
        std::string header_section = buffer.substr(0, header_end);
        std::istringstream iss(header_section);
        std::string line;
        // Parse status line
        if (std::getline(iss, line)) {
          std::istringstream status_stream(line);
          std::string http_version;
          status_stream >> http_version >> resp.status_code;
          std::getline(status_stream, resp.status_text);
          if (!resp.status_text.empty() && resp.status_text[0] == ' ') {
            resp.status_text.erase(0, 1);
          }
        }
        // Parse headers
        while (std::getline(iss, line) && !line.empty() && line != "\r") {
          if (line.back() == '\r')
            line.pop_back();
          auto colon = line.find(':');
          if (colon != std::string::npos) {
            auto key = line.substr(0, colon);
            auto value = line.substr(colon + 1);
            if (!value.empty() && value[0] == ' ')
              value.erase(0, 1);
            resp.headers.set(key, value);
          }
        }
        buffer.erase(0, header_end + 4);
        headers_done = true;
        auto cl = resp.headers.get("content-length");
        if (!cl.empty()) {
          content_length = std::stoull(cl);
        }
        auto te = resp.headers.get("transfer-encoding");
        if (te.find("chunked") != std::string::npos) {
          is_chunked = true;
        }
      }
    }
    if (headers_done && !is_chunked && content_length > 0) {
      if (buffer.size() >= content_length) {
        resp.body = buffer.substr(0, content_length);
        break;
      }
    }
  }
  if (!headers_done) {
    return stl::make_error<Response>("Failed to parse response headers");
  }
  if (!is_chunked && content_length == 0 && !buffer.empty()) {
    resp.body = buffer;
  } else if (!is_chunked && content_length > 0) {
    resp.body = buffer.substr(0, content_length);
  }
  return resp;
}

std::future<stl::result<Response>> Client::async_send(Request req) {
  return std::async(std::launch::async,
                    [req = std::move(req)]() -> stl::result<Response> {
                      auto sock_result = connect_socket(req.url);
                      if (!sock_result) {
                        return stl::make_error<Response>(sock_result.error());
                      }
                      int sock = sock_result.value();
                      auto send_result = send_request(sock, req);
                      if (!send_result) {
#ifdef _WIN32
                        closesocket(sock);
#else
            close(sock);
#endif
                        return stl::make_error<Response>(send_result.error());
                      }
                      auto resp_result = read_response(sock);

#ifdef _WIN32
                      closesocket(sock);
#else
        close(sock);
#endif
                      return resp_result;
                    });
}

stl::result<Response> Client::send(const Request &req) {
  auto future = async_send(Request(req));
  return future.get();
}

std::future<stl::result<Response>> Client::get(std::string_view url_str) {
  auto url_result = URL::parse(url_str);
  if (!url_result) {
    std::promise<stl::result<Response>> p;
    p.set_value(stl::make_error<Response>(url_result.error()));
    return p.get_future();
  }
  return async_send(Request(EMethod::GET, std::move(url_result.value())));
}

std::future<stl::result<Response>> Client::post(std::string_view url_str,
                                                std::string body) {
  auto url_result = URL::parse(url_str);
  if (!url_result) {
    std::promise<stl::result<Response>> p;
    p.set_value(stl::make_error<Response>(url_result.error()));
    return p.get_future();
  }
  Request req(EMethod::POST, std::move(url_result.value()));
  req.set_body(std::move(body));
  return async_send(std::move(req));
}

} // namespace http