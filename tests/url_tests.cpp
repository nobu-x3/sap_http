#ifdef SAP_HTTP_USE_MODULES
import http;
#else
#include "net/http.h"
#endif
#include <gtest/gtest.h>

TEST(UrlTest, ParseBasicHttp) {
  auto result = http::URL::parse("http://example.com/path");
  ASSERT_TRUE(result.has_value());

  auto &url = result.value();
  EXPECT_EQ(url.scheme, "http");
  EXPECT_EQ(url.host, "example.com");
  EXPECT_EQ(url.port, "80");
  EXPECT_EQ(url.path, "/path");
  EXPECT_EQ(url.query, "");
}

TEST(UrlTest, ParseHttpsWithDefaultPort) {
  auto result = http::URL::parse("https://secure.example.com/api");
  ASSERT_TRUE(result.has_value());

  auto &url = result.value();
  EXPECT_EQ(url.scheme, "https");
  EXPECT_EQ(url.host, "secure.example.com");
  EXPECT_EQ(url.port, "443");
  EXPECT_EQ(url.path, "/api");
}

TEST(UrlTest, ParseWithCustomPort) {
  auto result = http::URL::parse("http://example.com:8080/path");
  ASSERT_TRUE(result.has_value());

  auto &url = result.value();
  EXPECT_EQ(url.host, "example.com");
  EXPECT_EQ(url.port, "8080");
}

TEST(UrlTest, ParseWithQuery) {
  auto result = http::URL::parse("http://example.com/search?q=test&page=1");
  ASSERT_TRUE(result.has_value());

  auto &url = result.value();
  EXPECT_EQ(url.path, "/search");
  EXPECT_EQ(url.query, "?q=test&page=1");
}

TEST(UrlTest, ParseQueryWithoutPath) {
  auto result = http::URL::parse("http://example.com?query=value");
  ASSERT_TRUE(result.has_value());

  auto &url = result.value();
  EXPECT_EQ(url.path, "/");
  EXPECT_EQ(url.query, "?query=value");
}

TEST(UrlTest, ParseRootPath) {
  auto result = http::URL::parse("http://example.com/");
  ASSERT_TRUE(result.has_value());

  auto &url = result.value();
  EXPECT_EQ(url.path, "/");
}

TEST(UrlTest, ParseNoPath) {
  auto result = http::URL::parse("http://example.com");
  ASSERT_TRUE(result.has_value());

  auto &url = result.value();
  EXPECT_EQ(url.path, "/");
}

TEST(UrlTest, ParseIPAddress) {
  auto result = http::URL::parse("http://192.168.1.1:3000/api");
  ASSERT_TRUE(result.has_value());

  auto &url = result.value();
  EXPECT_EQ(url.host, "192.168.1.1");
  EXPECT_EQ(url.port, "3000");
}

TEST(UrlTest, ParseLocalhost) {
  auto result = http::URL::parse("http://localhost:5000/test");
  ASSERT_TRUE(result.has_value());

  auto &url = result.value();
  EXPECT_EQ(url.host, "localhost");
  EXPECT_EQ(url.port, "5000");
}

TEST(UrlTest, FullPathWithoutQuery) {
  auto result = http::URL::parse("http://example.com/api/v1/users");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value().full_path(), "/api/v1/users");
}

TEST(UrlTest, FullPathWithQuery) {
  auto result = http::URL::parse("http://example.com/search?q=test");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value().full_path(), "/search?q=test");
}

TEST(UrlTest, InvalidUrlNoScheme) {
  auto result = http::URL::parse("example.com/path");
  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(result.has_error());
}

TEST(UrlTest, InvalidUrlEmptyString) {
  auto result = http::URL::parse("");
  EXPECT_FALSE(result.has_value());
}

TEST(UrlTest, ParseComplexPath) {
  auto result = http::URL::parse("http://example.com/api/v1/users/123/profile");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value().path, "/api/v1/users/123/profile");
}

TEST(UrlTest, ParseMultipleQueryParams) {
  auto result = http::URL::parse(
      "http://example.com/search?q=test&page=2&limit=10&sort=asc");
  ASSERT_TRUE(result.has_value());

  auto &url = result.value();
  EXPECT_EQ(url.query, "?q=test&page=2&limit=10&sort=asc");
}