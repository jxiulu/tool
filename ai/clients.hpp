// clients

#pragma once

#include <curl/curl.h>
#include <curl/easy.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

using nlohmann::json;

namespace ai::base {

static size_t write_callback(void *contents, size_t size, size_t nmemb,
                             std::string *userp) {
    size_t total_size = size * nmemb;
    userp->append(static_cast<char *>(contents), total_size);
    return total_size;
}

class response {
  public:
    virtual bool process();
    response &invalidate(const std::string &why) {
        valid_ = false;
        error_ = why;
        return *this;
    }

    response(std::string raw, long http_code)
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

class client {
  public:
    client(const std::string &api_key, CURL *curl)
        : api_key_(api_key), curl_(curl) {}

    ~client() noexcept {
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

} // namespace ai::base

namespace ai::deepl {

enum class models {
    latency_optimized = 0,
    quality_optimized,
    prefer_quality_optimized,
};

class request {
  public:
    request(const std::string &content, const std::string &target_lang)
        : content_({content}), target_lang_(target_lang) {}

    request(const std::vector<std::string> &content,
            const std::string &target_lang)
        : content_(content), target_lang_(target_lang) {}

    const std::optional<std::string> &source_lang() const {
        return source_lang_;
    }
    request &set_source_lang(const std::string &langcode) {
        source_lang_ = langcode;
        return *this;
    }

    const std::string &target_lang() const { return target_lang_; }

    request &set_target_lang(const std::string &langcode) {
        target_lang_ = langcode;
        return *this;
    }

    const std::vector<std::string> &content() const { return content_; }

    request &set_content(const std::vector<std::string> &content) {
        content_ = content;
        return *this;
    }

    request &set_content(const std::string &content) {
        content_.clear();
        content_.push_back(content);
        return *this;
    }

    const std::optional<std::string> &context() const { return context_; }

    request &set_context(const std::string &ctx) {
        context_ = ctx;
        return *this;
    }

    const std::optional<models> &model() const { return model_; }

    request &set_model(models model) {
        model_ = model;
        return *this;
    }

    json payload() const {
        json request;

        request["text"] = content_;
        request["target_lang"] = target_lang_;

        if (source_lang_)
            request["source_lang"] = *source_lang_;

        if (context_)
            request["context"] = *context_;

        if (model_) {
            switch (*model_) {
            case models::latency_optimized:
                request["model_type"] = "latency_optimized";
                break;
            case models::quality_optimized:
                request["model_type"] = "quality_optimized";
                break;
            case models::prefer_quality_optimized:
                request["model_type"] = "prefer_quality_optimized";
                break;
            }
        }

        return request;
    }

  private:
    std::vector<std::string> content_;
    std::string target_lang_;
    std::optional<std::string> source_lang_;
    std::optional<std::string> context_;
    std::optional<models> model_;
};

class response : public ai::base::response {
  public:
    response(const std ::string &raw, long http_code)
        : ai::base::response(raw, http_code) {}
    bool process() override;

    int *billed_characters() const;
    const std::string *model_type_used() const;

  private:
};

class client : public ai::base::client {
  public:
    client(const std::string &api_key, CURL *curl)
        : ai::base::client(api_key, curl) {
        headers_ =
            curl_slist_append(headers_, "Content-Type: application/json");
        const std::string auth = "Authorization: DeepL-Auth-Key " + api_key_;
        headers_ = curl_slist_append(headers_, auth.data());

        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
        curl_easy_setopt(curl_, CURLOPT_URL,
                         "https://api.deepl.com/v2/translate");
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION,
                         ai::base::write_callback);

        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 120L);
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 30L);
    };

    response translate(const request &req);

  private:
};

std::unique_ptr<client> new_client(const std::string &key);

} // namespace ai::deepl
