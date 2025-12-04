#pragma once

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

struct deepl_request {
    std::vector<std::string> texts;
    std::string target_lang;
    std::optional<std::string> source_lang;
    std::optional<std::string> context;
    std::optional<deepl_model> model;

    deepl_request(const std::string &text, const std::string &target)
        : texts({text}), target_lang(target)
    {
    }

    deepl_request(const std::vector<std::string> &texts_vec,
                  const std::string &target)
        : texts(texts_vec), target_lang(target)
    {
    }

    deepl_request &set_source_lang(const std::string &langcode);
    deepl_request &set_target_lang(const std::string &langcode);
    deepl_request &set_texts(const std::vector<std::string> &texts_vec);
    deepl_request &set_text(const std::string &text);
    deepl_request &set_context(const std::string &ctx);
    deepl_request &set_model(deepl_model mdl);

    json to_json() const;
};

struct deepl_response {
    std::vector<std::string> content;
    bool valid;
    std::string error;
    std::string raw_json;

    std::optional<int> billed_characters;
    std::optional<std::string> model_type_used;

    static deepl_response parse(const std::string &raw, long http_code);
};

class DeepLClient
{
  public:
    DeepLClient(const std::string &api_key, CURL *curl);
    ~DeepLClient();

    deepl_response translate(const deepl_request &req);

    void set_api_key(const std::string &key) { api_key_ = key; }

  private:
    std::string api_key_;
    CURL *curl_;
    struct curl_slist *headers_;
};

std::unique_ptr<DeepLClient> new_deepl_client(const std::string &key);

} // namespace setman::ai
