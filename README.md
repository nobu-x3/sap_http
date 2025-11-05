# sap_http

A modern, lightweight C++20 HTTP library with both client and server support.

## Features

- 🚀 **Modern C++20**: Clean, type-safe API using modern C++ features
- 🌐 **HTTP Client & Server**: Full-featured client and server in one library
- ⚡ **Async I/O**: Promise/future-based asynchronous client operations
- 🛡️ **Type-Safe Error Handling**: Comprehensive `result<T>` type for all operations
- 🔌 **Cross-Platform**: Windows, Linux, and macOS support
- 📦 **Lightweight**: No external dependencies, minimal overhead
- 🧵 **Multithreaded Server**: Optional multithreaded request handling
- 🌐 **HTTP/1.1**: Full protocol support

## Quick Start

### Installation

```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Install
sudo cmake --install build
```

### Simple HTTP Client

```cpp
#include <http/http.hpp>
#include <iostream>

int main() {
    auto future = http::Client::get("http://example.com/api");
    auto result = future.get();
    
    if (result) {
        auto& response = result.value();
        std::cout << "Status: " << response.status_code << '\n';
        std::cout << "Body: " << response.body << '\n';
    } else {
        std::cerr << "Error: " << result.error() << '\n';
    }
    return 0;
}
```

### Simple HTTP Server

```cpp
#include <http/http.hpp>
#include <iostream>

int main() {
    http::Server server;
    server.set_port(8080).multithreaded();
    
    // Add routes
    server.route("/", http::EMethod::GET, [](const http::ServerRequest& req) {
        return http::Response(200, "Hello, World!");
    });
    
    server.route("/api/data", http::EMethod::POST, [](const http::ServerRequest& req) {
        // Echo the request body
        http::Response resp(200, req.body);
        resp.headers.set("Content-Type", "application/json");
        return resp;
    });
    
    // Start server
    auto result = server.start();
    if (!result) {
        std::cerr << "Failed to start server: " << result.error() << '\n';
        return 1;
    }
    
    std::cout << "Server running on port 8080\n";
    server.run();  // Blocks until server.stop() is called
    
    return 0;
}
```

## Building Your Project

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(sap_http REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE sap::http)
```

**Requirements:**
- CMake 3.20+
- Any C++20 compatible compiler (GCC 11+, Clang 14+, MSVC 19.28+)

## API Reference

### HTTP Client

#### Making Requests

```cpp
// GET request
auto future = http::Client::get("http://api.example.com/users");
auto result = future.get();

// POST request with JSON
std::string json = R"({"name": "John", "age": 30})";
auto future = http::Client::post("http://api.example.com/users", json);

// Custom request with headers
auto url_result = http::URL::parse("http://api.example.com/resource");
if (url_result) {
    http::Request req(http::EMethod::PUT, std::move(url_result.value()));
    req.set_header("Authorization", "Bearer token123");
    req.set_header("Content-Type", "application/json");
    req.set_body(R"({"status": "updated"})");
    
    auto future = http::Client::async_send(std::move(req));
    auto result = future.get();
}
```

#### Supported Methods

```cpp
enum class EMethod {
    GET,
    POST, 
    PUT,
    DELETE,
    HEAD,
    PATCH,
    OPTIONS
};

// Convert to/from string
std::string method_str = http::method_to_string(http::EMethod::POST);  // "POST"
http::EMethod method = http::string_to_method("GET");  // EMethod::GET
```

#### URL Parsing

```cpp
auto result = http::URL::parse("http://example.com:8080/path?query=value");
if (result) {
    auto& url = result.value();
    std::cout << "Scheme: " << url.scheme << '\n';  // "http"
    std::cout << "Host: " << url.host << '\n';      // "example.com"
    std::cout << "Port: " << url.port << '\n';      // "8080"
    std::cout << "Path: " << url.path << '\n';      // "/path"
    std::cout << "Query: " << url.query << '\n';    // "?query=value"
}
```

#### Headers Management

```cpp
http::Headers h;
h.set("Content-Type", "application/json");
h.set("Authorization", "Bearer token");

// Case-insensitive access
std::string content_type = h.get("content-type");
bool has_auth = h.has("Authorization");
```

#### Error Handling

All client operations return `stl::result<T>`:

```cpp
auto result = http::Client::get("http://example.com").get();

// Check if request succeeded
if (result.has_value()) {
    auto& response = result.value();
    
    // Check HTTP status
    if (response.is_success()) {  // 2xx status codes
        std::cout << "Success: " << response.body << '\n';
    } else {
        std::cout << "HTTP Error " << response.status_code << '\n';
    }
} else {
    // Network or parsing error
    std::cerr << "Request failed: " << result.error() << '\n';
}
```

#### Async Operations

```cpp
// Launch multiple requests concurrently
std::vector<std::future<stl::result<http::Response>>> futures;

futures.push_back(http::Client::get("http://api.example.com/users"));
futures.push_back(http::Client::get("http://api.example.com/posts"));
futures.push_back(http::Client::get("http://api.example.com/comments"));

// Wait for all to complete
for (auto& future : futures) {
    auto result = future.get();
    if (result) {
        std::cout << "Status: " << result.value().status_code << '\n';
    }
}
```

### HTTP Server

#### Creating a Server

```cpp
http::Server server;

// Configure
server.set_port(8080)      // Set port (default: 8080)
      .multithreaded();    // Enable multithreaded mode

// Start
auto result = server.start();
if (result) {
    server.run();  // Blocking
}

// Or run in background thread
std::thread server_thread([&server]() {
    if (server.start()) {
        server.run();
    }
});
```

#### Defining Routes

```cpp
// Simple GET endpoint
server.route("/health", http::EMethod::GET, [](const http::ServerRequest& req) {
    return http::Response(200, R"({"status": "healthy"})");
});

// POST with request processing
server.route("/api/users", http::EMethod::POST, [](const http::ServerRequest& req) {
    // Access request data
    std::string body = req.body;
    std::string content_type = req.headers.get("Content-Type");
    
    // Create response
    http::Response resp(201, R"({"id": 123, "created": true})");
    resp.headers.set("Content-Type", "application/json");
    resp.headers.set("Location", "/api/users/123");
    return resp;
});

// DELETE endpoint
server.route("/api/users", http::EMethod::DELETE, [](const http::ServerRequest& req) {
    return http::Response(204);  // No content
});
```

#### ServerRequest Object

```cpp
struct ServerRequest {
    EMethod method;                             // HTTP method
    std::string path;                           // Request path
    std::string query;                          // Query string
    Headers headers;                            // Request headers
    std::string body;                           // Request body
    std::map<std::string, std::string> params;  // URL parameters
};
```

#### Response Object

```cpp
// Simple response
http::Response resp(200, "Hello World");

// Response with custom headers
http::Response resp(201, R"({"id": 1})");
resp.headers.set("Content-Type", "application/json");
resp.headers.set("X-Custom-Header", "value");

// Check success
if (resp.is_success()) {  // 2xx status codes
    // ...
}
```

## Complete Examples

### REST API Server

```cpp
#include <http/http.hpp>
#include <iostream>
#include <string>
#include <map>
#include <mutex>

// Simple in-memory database
struct Database {
    std::map<int, std::string> users;
    std::mutex mtx;
    int next_id = 1;
};

int main() {
    Database db;
    http::Server server;
    server.set_port(8000).multithreaded();
    
    // GET /api/users - List all users
    server.route("/api/users", http::EMethod::GET, [&db](const http::ServerRequest&) {
        std::lock_guard<std::mutex> lock(db.mtx);
        
        std::string json = "[";
        bool first = true;
        for (const auto& [id, name] : db.users) {
            if (!first) json += ",";
            json += R"({"id":)" + std::to_string(id) + R"(,"name":")" + name + R"("})";
            first = false;
        }
        json += "]";
        
        http::Response resp(200, json);
        resp.headers.set("Content-Type", "application/json");
        return resp;
    });
    
    // POST /api/users - Create user
    server.route("/api/users", http::EMethod::POST, [&db](const http::ServerRequest& req) {
        std::lock_guard<std::mutex> lock(db.mtx);
        
        // Parse name from JSON (simplified)
        std::string name = req.body;
        
        int id = db.next_id++;
        db.users[id] = name;
        
        std::string response = R"({"id":)" + std::to_string(id) + 
                              R"(,"name":")" + name + R"(","created":true})";
        
        http::Response resp(201, response);
        resp.headers.set("Content-Type", "application/json");
        return resp;
    });
    
    // DELETE /api/users - Delete all users
    server.route("/api/users", http::EMethod::DELETE, [&db](const http::ServerRequest&) {
        std::lock_guard<std::mutex> lock(db.mtx);
        db.users.clear();
        return http::Response(204);  // No content
    });
    
    std::cout << "Starting REST API server on port 8000...\n";
    if (server.start()) {
        server.run();
    }
    
    return 0;
}
```

### HTTP Client with Multiple Concurrent Requests

```cpp
#include <http/http.hpp>
#include <iostream>
#include <vector>

int main() {
    std::vector<std::string> urls = {
        "http://api.example.com/endpoint1",
        "http://api.example.com/endpoint2",
        "http://api.example.com/endpoint3"
    };
    
    std::vector<std::future<stl::result<http::Response>>> futures;
    
    // Launch all requests asynchronously
    for (const auto& url : urls) {
        futures.push_back(http::Client::get(url));
    }
    
    // Collect results
    for (size_t i = 0; i < futures.size(); ++i) {
        auto result = futures[i].get();
        if (result && result.value().is_success()) {
            std::cout << "Request " << i << " succeeded\n";
            std::cout << "Body length: " << result.value().body.size() << '\n';
        } else {
            std::cout << "Request " << i << " failed\n";
        }
    }
    
    return 0;
}
```

### Microservice with JSON API

```cpp
#include <http/http.hpp>
#include <iostream>
#include <string>

int main() {
    http::Server server;
    server.set_port(8000).multithreaded();
    
    // Health check
    server.route("/health", http::EMethod::GET, [](const http::ServerRequest&) {
        http::Response resp(200, R"({"status":"healthy","model_loaded":true})");
        resp.headers.set("Content-Type", "application/json");
        return resp;
    });
    
    // Chat endpoint
    server.route("/chat", http::EMethod::POST, [](const http::ServerRequest& req) {
        try {
            // In production, parse JSON from req.body
            std::string text = req.body;
            
            // Process the request...
            std::string response_json = R"({
                "action": null,
                "response": "Hello! I received your message.",
                "conversation_id": "conv_123"
            })";
            
            http::Response resp(200, response_json);
            resp.headers.set("Content-Type", "application/json");
            return resp;
            
        } catch (const std::exception& e) {
            std::string error = R"({"error":")" + std::string(e.what()) + R"("})";
            http::Response resp(500, error);
            resp.headers.set("Content-Type", "application/json");
            return resp;
        }
    });
    
    std::cout << "Server running on port 8000\n";
    if (server.start()) {
        server.run();
    }
    
    return 0;
}
```

### POST JSON Data

```cpp
#include <http/http.hpp>
#include <iostream>

int main() {
    std::string json_data = R"({
        "username": "john_doe",
        "email": "john@example.com",
        "age": 30
    })";
    
    auto url_result = http::URL::parse("http://api.example.com/users");
    if (!url_result) {
        std::cerr << "Invalid URL\n";
        return 1;
    }
    
    http::Request req(http::EMethod::POST, std::move(url_result.value()));
    req.set_header("Content-Type", "application/json");
    req.set_header("Authorization", "Bearer your_token");
    req.set_body(std::move(json_data));
    
    auto future = http::Client::async_send(std::move(req));
    auto result = future.get();
    
    if (result) {
        auto& response = result.value();
        std::cout << "Status: " << response.status_code << '\n';
        std::cout << "Response: " << response.body << '\n';
    }
    
    return 0;
}
```

### Download Data with Custom Headers

```cpp
#include <http/http.hpp>
#include <iostream>
#include <fstream>

int main() {
    auto url_result = http::URL::parse("http://example.com/data.json");
    if (!url_result) return 1;
    
    http::Request req(http::EMethod::GET, std::move(url_result.value()));
    req.set_header("Accept", "application/json");
    req.set_header("User-Agent", "MyApp/1.0");
    
    auto future = http::Client::async_send(std::move(req));
    auto result = future.get();
    
    if (result && result.value().is_success()) {
        std::ofstream file("data.json");
        file << result.value().body;
        std::cout << "Downloaded successfully!\n";
    }
    
    return 0;
}
```

## CMake Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `SAP_HTTP_BUILD_SHARED` | `ON` | Build shared library |
| `SAP_HTTP_BUILD_STATIC` | `ON` | Build static library |
| `SAP_HTTP_BUILD_TESTS` | `ON` | Build test suite |
| `SAP_HTTP_INSTALL` | `ON` | Enable installation |

**Examples:**

```bash
# Build only static library
cmake -B build -DSAP_HTTP_BUILD_SHARED=OFF

# Build without tests
cmake -B build -DSAP_HTTP_BUILD_TESTS=OFF

# Development build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

## Running Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run with verbose output
ctest --test-dir build -V
```

**Test Coverage:**
- ✅ URL parsing (15 tests)
- ✅ Header management (8 tests)
- ✅ Request building (9 tests)
- ✅ Client operations (2 tests)
- ✅ Response handling (3 tests)
- ✅ Server operations (5 tests)
- 🌐 Integration tests (4 tests, disabled by default)

## Naming Conventions

The library uses consistent naming conventions:

| Type | Convention | Example |
|------|-----------|---------|
| **Enums** | PascalCase with `E` prefix | `EMethod` |
| **Structs/Classes** | PascalCase | `URL`, `Headers`, `Request`, `Response`, `Client`, `Server` |
| **Functions** | snake_case | `method_to_string()`, `set_header()` |
| **Member variables** | snake_case | `status_code`, `body` |
| **Types from core** | lowercase | `i32`, `u16`, `result<T>` |

## Platform Support

| Platform | Client | Server | Notes |
|----------|--------|--------|-------|
| Linux | ✅ | ✅ | Tested on Ubuntu 20.04+ |
| macOS | ✅ | ✅ | Tested on macOS 12+ |
| Windows | ✅ | ✅ | MSVC 2019+, MinGW-w64 |

## Performance Tips

1. **Use multithreaded mode** for servers handling concurrent requests
2. **Static linking** provides better optimization opportunities
3. **Async operations** enable efficient concurrent requests
4. **Connection pooling** will be added in future versions

## Troubleshooting

### Linker Errors on Windows

Ensure `ws2_32.lib` is linked (handled automatically by CMake).

### Port Already in Use

If server fails to start:
```cpp
server.set_port(8081);  // Try different port
```

### Connection Refused

For server/client integration:
- Server binds to `127.0.0.1` (loopback)
- Use `127.0.0.1` in URLs, not `localhost`
- Allow sufficient time for server to start

### "result not found" Error

Make sure to include the library:
```cpp
#include <http/http.hpp>
```

## License

MIT License - see LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## Roadmap

### Client
- [ ] HTTPS/TLS support
- [ ] HTTP/2 support
- [ ] Connection pooling
- [ ] Request/response compression
- [ ] Cookie management
- [ ] Proxy support
- [ ] Streaming uploads/downloads

### Server
- [ ] Path parameter extraction (`/users/:id`)
- [ ] Middleware support
- [ ] Static file serving
- [ ] WebSocket support
- [ ] Request body size limits
- [ ] Rate limiting
- [ ] CORS support
- [ ] Session management

## Credits

Designed for high-performance C++20 applications with a focus on simplicity and type safety.