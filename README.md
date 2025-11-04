# sap_http

A modern, lightweight C++20 HTTP client library with support for both C++20 modules and traditional headers.

## Features

- 🚀 **Modern C++20**: Uses modules or headers based on your preference
- ⚡ **Async I/O**: Promise/future-based asynchronous operations
- 🛡️ **Type-Safe Error Handling**: Comprehensive `result<T>` type for all operations
- 🔌 **Cross-Platform**: Windows, Linux, and macOS support
- 📦 **Lightweight**: No external dependencies, minimal overhead
- 🌐 **HTTP/1.1**: Full protocol support with chunked encoding

## Quick Start

### Installation

```bash

# Build with C++20 modules (default)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Or build with traditional headers
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSAP_HTTP_USE_MODULES=OFF
cmake --build build

# Install
sudo cmake --install build
```

### Simple GET Request

**With C++20 Modules:**
```cpp
import http;
#include <iostream>

int main() {
    auto future = http::client::get("http://example.com/api");
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

**With Traditional Headers:**
```cpp
#include <sap/http/http.hpp>
#include <iostream>

int main() {
    auto future = http::client::get("http://example.com/api");
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

## Building Your Project

### Using C++20 Modules

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.28)
project(my_app CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(sap_http REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE sap::http)
```

**Requirements:**
- CMake 3.28+
- GCC 11+, Clang 16+, or MSVC 19.30+

### Using Traditional Headers

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
- Any C++20 compatible compiler

## API Reference

### Making Requests

#### GET Request
```cpp
// Simple GET
auto future = http::client::get("http://api.example.com/users");
auto result = future.get();

if (result && result.value().is_success()) {
    std::cout << result.value().body << '\n';
}
```

#### POST Request
```cpp
// POST with JSON body
std::string json = R"({"name": "John", "age": 30})";
auto future = http::client::post("http://api.example.com/users", json);
auto result = future.get();

if (result && result.value().is_success()) {
    std::cout << "Created! Status: " << result.value().status_code << '\n';
}
```

#### Custom Request
```cpp
// Build a custom request
auto url_result = http::url::parse("http://api.example.com/resource/123");
if (!url_result) {
    std::cerr << "Invalid URL: " << url_result.error() << '\n';
    return;
}

http::request req(http::method::PUT, std::move(url_result.value()));
req.set_header("Authorization", "Bearer your_token_here");
req.set_header("Content-Type", "application/json");
req.set_body(R"({"status": "updated"})");

auto future = http::client::async_send(std::move(req));
auto result = future.get();
```

### URL Parsing

```cpp
auto result = http::url::parse("http://example.com:8080/path?query=value");
if (result) {
    auto& url = result.value();
    std::cout << "Scheme: " << url.scheme << '\n';  // "http"
    std::cout << "Host: " << url.host << '\n';      // "example.com"
    std::cout << "Port: " << url.port << '\n';      // "8080"
    std::cout << "Path: " << url.path << '\n';      // "/path"
    std::cout << "Query: " << url.query << '\n';    // "?query=value"
}
```

### Headers

```cpp
http::headers h;
h.set("Content-Type", "application/json");
h.set("Authorization", "Bearer token");

std::string content_type = h.get("content-type");  // Case-insensitive
bool has_auth = h.has("Authorization");
```

### Error Handling

The library uses a `result<T>` type for error handling:

```cpp
auto result = http::client::get("http://example.com").get();

// Check if request succeeded
if (result.has_value()) {
    auto& response = result.value();
    
    // Check HTTP status
    if (response.is_success()) {  // 2xx status codes
        std::cout << "Success: " << response.body << '\n';
    } else {
        std::cout << "HTTP Error: " << response.status_code << '\n';
    }
} else {
    // Network or parsing error
    std::cerr << "Request failed: " << result.error() << '\n';
}
```

### HTTP Methods

```cpp
// Available methods
http::method::GET
http::method::POST
http::method::PUT
http::method::DELETE
http::method::HEAD
http::method::PATCH
http::method::OPTIONS

// Convert to string
std::string method_str = http::method_to_string(http::method::POST);  // "POST"
```

### Async Operations

```cpp
// Launch multiple requests concurrently
std::vector<std::future<stl::result<http::response>>> futures;

futures.push_back(http::client::get("http://api.example.com/users"));
futures.push_back(http::client::get("http://api.example.com/posts"));
futures.push_back(http::client::get("http://api.example.com/comments"));

// Wait for all to complete
for (auto& future : futures) {
    auto result = future.get();
    if (result) {
        std::cout << "Status: " << result.value().status_code << '\n';
    }
}
```

## CMake Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `SAP_HTTP_BUILD_SHARED` | `ON` | Build shared library |
| `SAP_HTTP_BUILD_STATIC` | `ON` | Build static library |
| `SAP_HTTP_BUILD_TESTS` | `ON` | Build test suite |
| `SAP_HTTP_INSTALL` | `ON` | Enable installation |
| `SAP_HTTP_USE_MODULES` | `ON` | Use C++20 modules |

**Examples:**

```bash
# Build only static library without modules
cmake -B build -DSAP_HTTP_BUILD_SHARED=OFF -DSAP_HTTP_USE_MODULES=OFF

# Build without tests
cmake -B build -DSAP_HTTP_BUILD_TESTS=OFF

# Development build with tests
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

## Running Tests

```bash
# Build with tests
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run with verbose output
ctest --test-dir build -V
```

The test suite includes:
- ✅ URL parsing (15 tests)
- ✅ Header management (8 tests)
- ✅ Request building (7 tests)
- ✅ Client operations (2 tests)
- ✅ Response handling (2 tests)
- 🌐 Integration tests (4 tests, require internet connection and httbingo to be up)

## Complete Examples

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
    
    auto url_result = http::url::parse("http://api.example.com/users");
    if (!url_result) {
        std::cerr << "Invalid URL\n";
        return 1;
    }
    
    http::request req(http::method::POST, std::move(url_result.value()));
    req.set_header("Content-Type", "application/json");
    req.set_header("Authorization", "Bearer your_token");
    req.set_body(std::move(json_data));
    
    auto future = http::client::async_send(std::move(req));
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
    auto url_result = http::url::parse("http://example.com/data.json");
    if (!url_result) return 1;
    
    http::request req(http::method::GET, std::move(url_result.value()));
    req.set_header("Accept", "application/json");
    req.set_header("User-Agent", "MyApp/1.0");
    
    auto future = http::client::async_send(std::move(req));
    auto result = future.get();
    
    if (result && result.value().is_success()) {
        std::ofstream file("data.json");
        file << result.value().body;
        std::cout << "Downloaded successfully!\n";
    }
    
    return 0;
}
```

### Multiple Concurrent Requests

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
    
    std::vector<std::future<stl::result<http::response>>> futures;
    
    // Launch all requests
    for (const auto& url : urls) {
        futures.push_back(http::client::get(url));
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

## Modules vs Headers: What's the Difference?

| Aspect | C++20 Modules | Traditional Headers |
|--------|---------------|---------------------|
| **Import/Include** | `import http;` | `#include <http/http.hpp>` |
| **Namespace** | `http::client::get()` | `http::client::get()` |
| **Result Type** | `stl::result<T>` | `stl::result<T>` |
| **Compilation** | Faster after initial build | Standard |
| **Compiler Support** | GCC 11+, Clang 16+, MSVC 19.30+ | Any C++20 |
| **CMake Version** | 3.28+ | 3.20+ |
**Note:** Both modes provide identical functionality and performance. Choose based on your project's requirements and compiler support.

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Linux | ✅ Fully Supported | Tested on Ubuntu 20.04+ |
| macOS | :question: Untested | Tested on macOS 12+ |
| Windows | :question: Untested | MSVC 2019+, MinGW-w64 |

## Troubleshooting

### Module Compilation Errors

If you get module compilation errors:
```bash
# Switch to header mode
cmake -B build -DSAP_HTTP_USE_MODULES=OFF
```

### Linker Errors on Windows

Ensure `ws2_32.lib` is linked (handled automatically by CMake).

### "result not found" Error

Make sure to import/include the core module:
```cpp
import http;  // Includes result type
// or
#include <sap/http/http.hpp>
```

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## Roadmap

- [ ] HTTPS/TLS support
- [ ] HTTP/2 support
- [ ] WebSocket support
- [ ] Connection pooling
- [ ] Request/response compression
- [ ] Cookie management
- [ ] Proxy support