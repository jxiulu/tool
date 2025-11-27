// clients

#pragma once

#include <curl/curl.h>
#include <curl/easy.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

using nlohmann::json;

namespace apis {

static size_t write_callback(void *contents, size_t size, size_t nmemb,
                             std::string *userp) {
    size_t total_size = size * nmemb;
    userp->append(static_cast<char *>(contents), total_size);
    return total_size;
}

class base_response {
  public:
    virtual bool process() { return true; }

    base_response &invalidate(const std::string &why) {
        valid_ = false;
        error_ = why;
        return *this;
    }

    base_response(std::string raw, long http_code)
        : raw_(std::move(raw)), http_code_(http_code), valid_(true) {
        try {
            json_ = json::parse(raw_);
        } catch (json::exception &e) {
            valid_ = false;
            error_ = std::string("[JSON ERROR] ") + e.what();
        }

        if (http_code_ != 200) {
            invalidate(std::string("[HTTP CODE ") + std::to_string(http_code) +
                       std::string("]"));
        }

        if (valid_) {
            valid_ = process();
        }
    };

    bool valid() const { return valid_; }
    const std::string &raw() const { return raw_; }
    const std::vector<std::string_view> &content() const { return content_; }
    operator bool() const { return valid_; }

    std::string error() const {
        if (valid_)
            return "[RESPONSE IS VALID]";
        return error_;
    }

    const json *find(const std::string &field) const {
        if (!json_.contains(field)) {
            return nullptr;
        }
        return &json_[field];
    }

    json *find(const std::string &field) {
        if (!json_.contains(field)) {
            return nullptr;
        }
        return &json_[field];
    }

  protected:
    std::vector<std::string_view> content_;
    json json_;

  private:
    std::string raw_;
    bool valid_;
    std::string error_;
    std::optional<long> http_code_;
};

class base_ai_client {
  public:
    base_ai_client(const std::string &api_key, CURL *curl)
        : api_key_(api_key), curl_(curl) {}

    ~base_ai_client() noexcept {
        if (headers_)
            curl_slist_free_all(headers_);
        if (curl_)
            curl_easy_cleanup(curl_);
    }

    const std::string &api_key() const { return api_key_; }

    void new_key(const std::string &key) {
        api_key_ = key;
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    }

  protected:
    std::string api_key_;
    CURL *curl_;
    struct curl_slist *headers_ = nullptr;
};

} // namespace apis
