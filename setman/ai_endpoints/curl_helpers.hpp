#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>

using nlohmann::json;

namespace setman::ai::curl_helpers
{

struct http_response {
    std::string body;
    long http_code;
    std::string error;
};

size_t write_callback(void *contents, size_t size, size_t nmemb,
                      std::string *userp);

http_response post_json(CURL *curl, const std::string &url, const json &payload,
                        struct curl_slist *headers);

struct curl_slist *add_json_header(struct curl_slist *headers = nullptr);

struct curl_slist *add_bearer_auth(struct curl_slist *headers,
                                   const std::string &token);

} // namespace setman::ai::curl_helpers
