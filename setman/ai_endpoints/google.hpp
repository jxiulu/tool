#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

using nlohmann::json;

namespace setman::ai
{

enum class content_type { text, inline_image, file_uri };

struct content {
    content_type part_type;
    std::string text_content;
    std::string mime_type;
    std::string data;
};

struct safety_setting {
    std::string category;
    std::string threshold;
};

struct safety_rating {
    std::string category;
    std::string probability;
};

struct google_request {
    std::string endpoint =
        "https://generativelanguage.googleapis.com/v1beta/models/";
    std::string model = "gemini-2.0-flash-exp";
    std::vector<content> parts;
    std::vector<safety_setting> safety_settings;
    std::optional<double> temperature;
    std::optional<int> max_tokens;
    std::optional<double> top_p;
    std::optional<int> top_k;
    std::optional<int> candidate_count;
    std::vector<std::string> stop_sequences;

    google_request &add_text(const std::string &text);
    google_request &add_inline_image(const std::string &base64_data,
                                     const std::string &mime_type = "image/jpeg");
    google_request &add_file_uri(const std::string &file_uri,
                                 const std::string &mime_type = "image/jpeg");
    google_request &add_safety_setting(const std::string &category,
                                       const std::string &threshold);
    google_request &set_model(const std::string &model_name);
    google_request &set_temperature(double temp);
    google_request &set_max_tokens(int tokens);
    google_request &set_top_p(double p);
    google_request &set_top_k(int k);
    google_request &set_candidate_count(int count);

    json to_json() const;
};

struct google_response {
    std::vector<std::string> content;
    bool valid;
    std::string error;
    std::string raw_json;

    std::optional<std::string> prompt_feedback;
    std::optional<std::string> finish_reason;
    std::vector<safety_rating> safety_ratings;
    std::optional<int> prompt_tokens;
    std::optional<int> candidates_tokens;
    std::optional<int> total_tokens;

    static google_response parse(const std::string &raw, long http_code);
};

class GoogleClient
{
  public:
    GoogleClient(const std::string &api_key, CURL *curl);
    ~GoogleClient();

    google_response send(const google_request &request);

    void set_api_key(const std::string &key) { api_key_ = key; }

  private:
    std::string api_key_;
    CURL *curl_;
    struct curl_slist *headers_;
};

std::unique_ptr<GoogleClient> new_google_client(const std::string &key);

} // namespace setman::ai
