#include "google.hpp"
#include "curl_helpers.hpp"
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
    : api_key_(api_key), curl_(curl), headers_(nullptr)
{
    headers_ = curl_helpers::add_json_header(headers_);
}

GoogleClient::~GoogleClient()
{
    if (headers_)
        curl_slist_free_all(headers_);
}

google_request &google_request::add_text(const std::string &text)
{
    content part;
    part.part_type = content_type::text;
    part.text_content = text;
    parts.push_back(part);
    return *this;
}

google_request &
google_request::add_inline_image(const std::string &base64_data,
                                 const std::string &mime_type)
{
    content part;
    part.part_type = content_type::inline_image;
    part.data = base64_data;
    part.mime_type = mime_type;
    parts.push_back(part);
    return *this;
}

google_request &google_request::add_file_uri(const std::string &file_uri,
                                             const std::string &mime_type)
{
    content part;
    part.part_type = content_type::file_uri;
    part.data = file_uri;
    part.mime_type = mime_type;
    parts.push_back(part);
    return *this;
}

google_request &
google_request::add_safety_setting(const std::string &category,
                                   const std::string &threshold)
{
    safety_settings.push_back({category, threshold});
    return *this;
}

google_request &google_request::set_model(const std::string &model_name)
{
    model = model_name;
    return *this;
}

google_request &google_request::set_temperature(double temp)
{
    temperature = temp;
    return *this;
}

google_request &google_request::set_max_tokens(int tokens)
{
    max_tokens = tokens;
    return *this;
}

google_request &google_request::set_top_p(double p)
{
    top_p = p;
    return *this;
}

google_request &google_request::set_top_k(int k)
{
    top_k = k;
    return *this;
}

google_request &google_request::set_candidate_count(int count)
{
    candidate_count = count;
    return *this;
}

json google_request::to_json() const
{
    json payload;

    json parts_array = json::array();
    for (const auto &part : parts) {
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

    payload["contents"] = json::array();
    payload["contents"].push_back({{"parts", parts_array}});

    json gen_config;
    if (temperature.has_value())
        gen_config["temperature"] = *temperature;
    if (max_tokens.has_value())
        gen_config["maxOutputTokens"] = *max_tokens;
    if (top_p.has_value())
        gen_config["topP"] = *top_p;
    if (top_k.has_value())
        gen_config["topK"] = *top_k;
    if (candidate_count.has_value())
        gen_config["candidateCount"] = *candidate_count;
    if (!stop_sequences.empty())
        gen_config["stopSequences"] = stop_sequences;

    if (!gen_config.empty())
        payload["generationConfig"] = gen_config;

    if (!safety_settings.empty()) {
        json safety_array = json::array();
        for (const auto &setting : safety_settings) {
            safety_array.push_back({{"category", setting.category},
                                    {"threshold", setting.threshold}});
        }
        payload["safetySettings"] = safety_array;
    }

    return payload;
}

google_response GoogleClient::send(const google_request &req)
{
    std::string url =
        req.endpoint + req.model + ":generateContent?key=" + api_key_;

    auto http_resp =
        curl_helpers::post_json(curl_, url, req.to_json(), headers_);

    if (!http_resp.error.empty()) {
        return {.content = {},
                .valid = false,
                .error = "[CURL ERROR] " + http_resp.error,
                .raw_json = ""};
    }

    return google_response::parse(http_resp.body, http_resp.http_code);
}

google_response google_response::parse(const std::string &raw, long http_code)
{
    google_response result;
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

    if (parsed.contains("promptFeedback")) {
        const auto &feedback = parsed["promptFeedback"];
        if (feedback.contains("blockReason")) {
            result.prompt_feedback = feedback["blockReason"].get<std::string>();
            result.error =
                "[GOOGLE] Prompt blocked: " + *result.prompt_feedback;
            return result;
        }
    }

    if (!parsed.contains("candidates")) {
        result.error = "[GOOGLE] Missing 'candidates' field in response";
        return result;
    }

    const auto &candidates = parsed["candidates"];
    if (!candidates.is_array() || candidates.empty()) {
        result.error = "[GOOGLE] 'candidates' field is empty or not an array";
        return result;
    }

    for (size_t i = 0; i < candidates.size(); i++) {
        const auto &candidate = candidates[i];

        if (i == 0 && candidate.contains("finishReason")) {
            result.finish_reason = candidate["finishReason"].get<std::string>();
        }

        if (i == 0 && candidate.contains("safetyRatings")) {
            const auto &ratings = candidate["safetyRatings"];
            if (ratings.is_array()) {
                for (const auto &rating : ratings) {
                    if (rating.contains("category") &&
                        rating.contains("probability")) {
                        result.safety_ratings.push_back(
                            {rating["category"].get<std::string>(),
                             rating["probability"].get<std::string>()});
                    }
                }
            }
        }

        if (!candidate.contains("content") ||
            !candidate["content"].contains("parts")) {
            result.error = "[GOOGLE] Candidate missing content or parts";
            return result;
        }

        const auto &parts = candidate["content"]["parts"];
        if (!parts.is_array()) {
            result.error = "[GOOGLE] Parts is not an array";
            return result;
        }

        std::string full_text;
        for (const auto &part : parts) {
            if (part.contains("text")) {
                full_text += part["text"].get<std::string>();
            }
        }

        if (!full_text.empty()) {
            result.content.push_back(full_text);
        }
    }

    if (parsed.contains("usageMetadata")) {
        const auto &usage = parsed["usageMetadata"];

        if (usage.contains("promptTokenCount") &&
            usage["promptTokenCount"].is_number_integer()) {
            result.prompt_tokens = usage["promptTokenCount"].get<int>();
        }

        if (usage.contains("candidatesTokenCount") &&
            usage["candidatesTokenCount"].is_number_integer()) {
            result.candidates_tokens = usage["candidatesTokenCount"].get<int>();
        }

        if (usage.contains("totalTokenCount") &&
            usage["totalTokenCount"].is_number_integer()) {
            result.total_tokens = usage["totalTokenCount"].get<int>();
        }
    }

    result.valid = true;
    return result;
}

} // namespace setman::ai
