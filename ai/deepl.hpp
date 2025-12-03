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

namespace setman::ai
{

enum class deepl_model {
    latency_optimized = 0,
    quality_optimized,
    prefer_quality_optimized,
};

class DeepLRequest
{
  public:
    DeepLRequest(const std::string &content, const std::string &target_lang)
        : content_({content}), target_lang_(target_lang)
    {
    }

    DeepLRequest(const std::vector<std::string> &content,
            const std::string &target_lang)
        : content_(content), target_lang_(target_lang)
    {
    }

    const std::optional<std::string> &source_lang() const
    {
        return source_lang_;
    }
    DeepLRequest &set_source_lang(const std::string &langcode)
    {
        source_lang_ = langcode;
        return *this;
    }

    const std::string &target_lang() const { return target_lang_; }

    DeepLRequest &set_target_lang(const std::string &langcode)
    {
        target_lang_ = langcode;
        return *this;
    }

    const std::vector<std::string> &contents() const { return content_; }

    DeepLRequest &set_contents(const std::vector<std::string> &content)
    {
        content_ = content;
        return *this;
    }

    DeepLRequest &set_content(const std::string &content)
    {
        content_.clear();
        content_.push_back(content);
        return *this;
    }

    const std::optional<std::string> &context() const { return context_; }

    DeepLRequest &set_context(const std::string &ctx)
    {
        context_ = ctx;
        return *this;
    }

    const std::optional<deepl_model> &model() const { return model_; }

    DeepLRequest &set_model(enum deepl_model model)
    {
        model_ = model;
        return *this;
    }

    json payload() const
    {
        json request;

        request["text"] = content_;
        request["target_lang"] = target_lang_;

        if (source_lang_)
            request["source_lang"] = *source_lang_;

        if (context_)
            request["context"] = *context_;

        if (model_) {
            switch (*model_) {
            case deepl_model::latency_optimized:
                request["model_type"] = "latency_optimized";
                break;
            case deepl_model::quality_optimized:
                request["model_type"] = "quality_optimized";
                break;
            case deepl_model::prefer_quality_optimized:
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
    std::optional<enum deepl_model> model_;
};

class DeepLResponse : public GenericResponse
{
  public:
    DeepLResponse(const std::string &raw, long http_code)
        : GenericResponse(raw, http_code)
    {
    }
    bool process() override;

    std::optional<int> billed_characters() const;
    std::optional<std::string> model_type_used() const;

  private:
};

class DeepLClient : public GenericClient
{
  public:
    DeepLClient(const std::string &api_key, CURL *curl)
        : GenericClient(api_key, curl)
    {
        // caller should validate curl
        headers_ =
            curl_slist_append(headers_, "Content-Type: application/json");
        const std::string auth = "Authorization: DeepL-Auth-Key " + api_key_;
        headers_ = curl_slist_append(headers_, auth.data());

        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
        curl_easy_setopt(curl_, CURLOPT_URL,
                         "https://api.deepl.com/v2/translate");
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, setman::write_callback);

        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 120L);
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 30L);
    };

    DeepLResponse translate(const DeepLRequest &req);

  private:
};

std::unique_ptr<DeepLClient> new_deepl_client(const std::string &key);

} // namespace setman::ai
