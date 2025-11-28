// deepl client

#pragma once

#include "ai.hpp"
#include <curl/curl.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

using nlohmann::json;

namespace apis::deepl {

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

    const std::vector<std::string> &contents() const { return content_; }

    request &set_contents(const std::vector<std::string> &content) {
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

class response : public apis::ai::base_response {
  public:
    response(const std::string &raw, long http_code)
        : apis::ai::base_response(raw, http_code) {}
    bool process() override;

    std::optional<int> billed_characters() const;
    std::optional<std::string> model_type_used() const;

  private:
};

class client : public apis::ai::base_client {
  public:
    client(const std::string &api_key, CURL *curl)
        : apis::ai::base_client(api_key, curl) {
        // caller should validate curl
        headers_ =
            curl_slist_append(headers_, "Content-Type: application/json");
        const std::string auth = "Authorization: DeepL-Auth-Key " + api_key_;
        headers_ = curl_slist_append(headers_, auth.data());

        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
        curl_easy_setopt(curl_, CURLOPT_URL,
                         "https://api.deepl.com/v2/translate");
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, apis::write_callback);

        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 120L);
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 30L);
    };

    response translate(const request &req);

  private:
};

std::unique_ptr<client> new_client(const std::string &key);

} // namespace apis::deepl
