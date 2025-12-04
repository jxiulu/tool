#include "deepl.hpp"
#include "curl_helpers.hpp"
#include <curl/curl.h>

namespace setman::ai
{

std::unique_ptr<DeepLClient> new_deepl_client(const std::string &key)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return nullptr;

    return std::make_unique<DeepLClient>(key, curl);
}

DeepLClient::DeepLClient(const std::string &api_key, CURL *curl)
    : api_key_(api_key), curl_(curl), headers_(nullptr)
{
    headers_ = curl_helpers::add_json_header(headers_);
    std::string auth = "Authorization: DeepL-Auth-Key " + api_key_;
    headers_ = curl_slist_append(headers_, auth.c_str());

    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 30L);
}

DeepLClient::~DeepLClient()
{
    if (headers_)
        curl_slist_free_all(headers_);
}

deepl_request &deepl_request::set_source_lang(const std::string &langcode)
{
    source_lang = langcode;
    return *this;
}

deepl_request &deepl_request::set_target_lang(const std::string &langcode)
{
    target_lang = langcode;
    return *this;
}

deepl_request &deepl_request::set_texts(const std::vector<std::string> &texts_vec)
{
    texts = texts_vec;
    return *this;
}

deepl_request &deepl_request::set_text(const std::string &text)
{
    texts.clear();
    texts.push_back(text);
    return *this;
}

deepl_request &deepl_request::set_context(const std::string &ctx)
{
    context = ctx;
    return *this;
}

deepl_request &deepl_request::set_model(deepl_model mdl)
{
    model = mdl;
    return *this;
}

json deepl_request::to_json() const
{
    json payload;

    payload["text"] = texts;
    payload["target_lang"] = target_lang;

    if (source_lang)
        payload["source_lang"] = *source_lang;

    if (context)
        payload["context"] = *context;

    if (model) {
        switch (*model) {
        case deepl_model::latency_optimized:
            payload["model_type"] = "latency_optimized";
            break;
        case deepl_model::quality_optimized:
            payload["model_type"] = "quality_optimized";
            break;
        case deepl_model::prefer_quality_optimized:
            payload["model_type"] = "prefer_quality_optimized";
            break;
        }
    }

    return payload;
}

deepl_response DeepLClient::translate(const deepl_request &req)
{
    std::string url = "https://api.deepl.com/v2/translate";

    auto http_resp = curl_helpers::post_json(curl_, url, req.to_json(), headers_);

    if (!http_resp.error.empty()) {
        return {.content = {},
                .valid = false,
                .error = "[CURL ERROR] " + http_resp.error,
                .raw_json = ""};
    }

    return deepl_response::parse(http_resp.body, http_resp.http_code);
}

deepl_response deepl_response::parse(const std::string &raw, long http_code)
{
    deepl_response result;
    result.raw_json = raw;
    result.valid = false;

    if (http_code != 200) {
        result.error = "[HTTP CODE " + std::to_string(http_code) + "]";
        return result;
    }

    json parsed;
    try {
        parsed = json::parse(raw);
    } catch (json::exception &e) {
        result.error = std::string("[JSON ERROR] ") + e.what();
        return result;
    }

    if (!parsed.contains("translations")) {
        result.error = "[DEEPL] Missing 'translations' field in response";
        return result;
    }

    const auto &translations = parsed["translations"];
    if (!translations.is_array() || translations.empty()) {
        result.error = "[DEEPL] 'translations' field is empty or not an array";
        return result;
    }

    for (size_t i = 0; i < translations.size(); i++) {
        const auto &translation = translations[i];

        if (!translation.contains("text")) {
            result.error = "[DEEPL] Translation at index " +
                           std::to_string(i) + " missing 'text' field";
            return result;
        }

        if (!translation["text"].is_string()) {
            result.error = "[DEEPL] Translation 'text' at index " +
                           std::to_string(i) + " is not a string";
            return result;
        }

        result.content.push_back(translation["text"].get<std::string>());
    }

    if (parsed.contains("billed_characters") &&
        parsed["billed_characters"].is_number_integer()) {
        result.billed_characters = parsed["billed_characters"].get<int>();
    }

    if (parsed.contains("model_type_used") &&
        parsed["model_type_used"].is_string()) {
        result.model_type_used = parsed["model_type_used"].get<std::string>();
    }

    result.valid = true;
    return result;
}

} // namespace setman::ai
