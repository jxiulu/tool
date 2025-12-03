// deepl client implementation

#include "deepl.hpp"
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

DeepLResponse DeepLClient::translate(const DeepLRequest &request)
{
    std::string response_body;
    std::string request_body = request.payload().dump();
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, request_body.size());
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &request_body);

    CURLcode ec = curl_easy_perform(curl_);
    if (ec != CURLE_OK) {
        DeepLResponse res("", 0);
        res.invalidate(std::string("[CURL ERROR] ") + curl_easy_strerror(ec));
        return res;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);

    return DeepLResponse(response_body, http_code);
}

bool DeepLResponse::process()
{
    json *translations = find("translations");

    if (!translations) {
        invalidate("[DEEPL] Missing 'translations' field in response");
        return false;
    }

    if (!translations->is_array()) {
        invalidate("[DEEPL] 'translations' field is not an array");
        return false;
    }

    if (translations->empty()) {
        invalidate("[DEEPL] 'translations' array is empty");
        return false;
    }

    // Reserve space for content
    content_.reserve(translations->size());

    // Validate each translation object has required fields
    // we'll only validate the absolutely necessary fields.
    for (size_t i = 0; i < translations->size(); i++) {
        const auto &translation = (*translations)[i];

        if (!translation.contains("text")) {
            invalidate("[DEEPL] Translation at index " + std::to_string(i) +
                       " missing 'text' field");
            return false;
        }

        if (!translation.contains("detected_source_language")) {
            invalidate("[DEEPL] Translation at index " + std::to_string(i) +
                       " missing 'detected_source_language' field");
            return false;
        }

        // Validate field types
        if (!translation["text"].is_string()) {
            invalidate("[DEEPL] Translation 'text' at index " +
                       std::to_string(i) + " is not a string");
            return false;
        }

        if (!translation["detected_source_language"].is_string()) {
            invalidate(
                "[DEEPL] Translation 'detected_source_language' at index " +
                std::to_string(i) + " is not a string");
            return false;
        }

        // Extract translated text and store as string_view
        const auto &text_str =
            translation["text"].get_ref<const std::string &>();
        content_.emplace_back(text_str);
    }

    return true;
}

std::optional<int> DeepLResponse::billed_characters() const
{
    if (!json_.contains("billed_characters"))
        return std::nullopt;

    const auto &billed = json_["billed_characters"];

    if (!billed.is_number_integer())
        return std::nullopt;

    return billed.get<int>();
}

std::optional<std::string> DeepLResponse::model_type_used() const
{
    if (!json_.contains("model_type_used"))
        return std::nullopt;

    const auto &model = json_["model_type_used"];

    if (!model.is_string())
        return std::nullopt;

    return model.get<std::string>();
}

} // namespace setman::ai
