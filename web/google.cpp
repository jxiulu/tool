// google gemini api implementation

#include "google.hpp"
#include <curl/curl.h>

namespace setman::ai
{

std::unique_ptr<GoogleClient> new_google_client(const std::string &key)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return nullptr;

    return std::make_unique<GoogleClient>(key, curl);
}

GoogleClient::GoogleClient(const std::string &api_key, CURL *curl)
    : GenericClient(api_key, curl)
{
    headers_ = curl_slist_append(headers_, "Content-Type: application/json");

    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, setman::write_callback);
}

json GoogleRequest::payload() const
{
    json payload;

    // Build parts array
    json parts_array = json::array();
    for (const auto &part : parts_) {
        json part_obj;

        switch (part.part_type) {
        case content_type::text:
            part_obj["text"] = part.text_content;
            break;

        case content_type::inline_image:
            part_obj["inline_data"] = {{"mime_type", part.mime_type},
                                       {"data", part.data}};
            break;

        case content_type::file_uri:
            part_obj["file_data"] = {{"mime_type", part.mime_type},
                                     {"file_uri", part.data}};
            break;
        }

        parts_array.push_back(part_obj);
    }

    // Build contents
    payload["contents"] = json::array();
    payload["contents"].push_back({{"parts", parts_array}});

    // Generation config
    json gen_config;
    if (temperature_.has_value())
        gen_config["temperature"] = *temperature_;
    if (max_tokens_.has_value())
        gen_config["maxOutputTokens"] = *max_tokens_;
    if (top_p_.has_value())
        gen_config["topP"] = *top_p_;
    if (top_k_.has_value())
        gen_config["topK"] = *top_k_;
    if (candidate_count_.has_value())
        gen_config["candidateCount"] = *candidate_count_;
    if (!stop_sequences_.empty())
        gen_config["stopSequences"] = stop_sequences_;

    if (!gen_config.empty())
        payload["generationConfig"] = gen_config;

    // Safety settings
    if (!safety_settings_.empty()) {
        json safety_array = json::array();
        for (const auto &setting : safety_settings_) {
            safety_array.push_back({{"category", setting.category},
                                    {"threshold", setting.threshold}});
        }
        payload["safetySettings"] = safety_array;
    }

    return payload;
}

GoogleResponse GoogleClient::send(const GoogleRequest &req)
{
    std::string response_body;
    std::string request_body = req.payload().dump();

    // Build full URL with model and API key
    std::string url =
        req.endpoint() + req.model() + ":generateContent?key=" + api_key_;

    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, request_body.size());
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body);

    CURLcode ec = curl_easy_perform(curl_);
    if (ec != CURLE_OK) {
        GoogleResponse res("", 0);
        res.invalidate(std::string("[CURL ERROR] ") + curl_easy_strerror(ec));
        return res;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);

    return GoogleResponse(response_body, http_code);
}

bool GoogleResponse::process()
{
    // Check for prompt feedback (blocks)
    if (json_.contains("promptFeedback")) {
        const auto &feedback = json_["promptFeedback"];
        if (feedback.contains("blockReason")) {
            prompt_feedback_ = feedback["blockReason"].get<std::string>();
            invalidate("[GOOGLE] Prompt blocked: " + *prompt_feedback_);
            return false;
        }
    }

    // Extract candidates array
    json *candidates = find("candidates");
    if (!candidates) {
        invalidate("[GOOGLE] Missing 'candidates' field in response");
        return false;
    }

    if (!candidates->is_array()) {
        invalidate("[GOOGLE] 'candidates' field is not an array");
        return false;
    }

    if (candidates->empty()) {
        invalidate("[GOOGLE] 'candidates' array is empty");
        return false;
    }

    content_.reserve(candidates->size());

    // Process each candidate
    for (size_t i = 0; i < candidates->size(); i++) {
        const auto &candidate = (*candidates)[i];

        // Extract finish reason from first candidate
        if (i == 0 && candidate.contains("finishReason")) {
            finish_reason_ = candidate["finishReason"].get<std::string>();
        }

        // Extract safety ratings from first candidate
        if (i == 0 && candidate.contains("safetyRatings")) {
            const auto &ratings = candidate["safetyRatings"];
            if (ratings.is_array()) {
                for (const auto &rating : ratings) {
                    if (rating.contains("category") &&
                        rating.contains("probability")) {
                        safety_ratings_.push_back(
                            {rating["category"].get<std::string>(),
                             rating["probability"].get<std::string>()});
                    }
                }
            }
        }

        // Extract content
        if (!candidate.contains("content")) {
            invalidate("[GOOGLE] Candidate at index " + std::to_string(i) +
                       " missing 'content' field");
            return false;
        }

        const auto &content = candidate["content"];
        if (!content.contains("parts")) {
            invalidate("[GOOGLE] Content at index " + std::to_string(i) +
                       " missing 'parts' field");
            return false;
        }

        const auto &parts = content["parts"];
        if (!parts.is_array()) {
            invalidate("[GOOGLE] Parts at index " + std::to_string(i) +
                       " is not an array");
            return false;
        }

        // Concatenate all text parts
        std::string full_text;
        for (const auto &part : parts) {
            if (part.contains("text")) {
                full_text += part["text"].get<std::string>();
            }
        }

        if (!full_text.empty()) {
            // Store as string_view referencing the JSON string
            const auto &text_str =
                json_["candidates"][i]["content"]["parts"][0]["text"]
                    .get_ref<const std::string &>();
            content_.emplace_back(text_str);
        }
    }

    // Extract usage metadata
    if (json_.contains("usageMetadata")) {
        const auto &usage = json_["usageMetadata"];

        if (usage.contains("promptTokenCount") &&
            usage["promptTokenCount"].is_number_integer()) {
            prompt_tokens_ = usage["promptTokenCount"].get<int>();
        }

        if (usage.contains("candidatesTokenCount") &&
            usage["candidatesTokenCount"].is_number_integer()) {
            candidates_tokens_ = usage["candidatesTokenCount"].get<int>();
        }

        if (usage.contains("totalTokenCount") &&
            usage["totalTokenCount"].is_number_integer()) {
            total_tokens_ = usage["totalTokenCount"].get<int>();
        }
    }

    return true;
}

} // namespace setman::ai
