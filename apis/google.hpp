// google gemini api

#pragma once

#include "ai.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

using nlohmann::json;

namespace apis::google
{

// Content part - can be text or image
struct Content {
    enum class type { text, inline_image, file_uri };

    type part_type;
    std::string text_content; // for text
    std::string mime_type;    // for images
    std::string data;         // base64 data or file_uri
};

class Request : public apis::ai::GenericRequest
{
  public:
    Request()
    {
        endpoint_ = "https://generativelanguage.googleapis.com/v1beta/models/";
        model_ = "gemini-2.0-flash-exp";
    }

    json payload() const override;

    // Add content parts (text, images)
    Request &add_text(const std::string &text)
    {
        Content part;
        part.part_type = Content::type::text;
        part.text_content = text;
        parts_.push_back(part);
        return *this;
    }

    Request &add_inline_image(const std::string &base64_data,
                              const std::string &mime_type = "image/jpeg")
    {
        Content part;
        part.part_type = Content::type::inline_image;
        part.data = base64_data;
        part.mime_type = mime_type;
        parts_.push_back(part);
        return *this;
    }

    Request &add_file_uri(const std::string &file_uri,
                          const std::string &mime_type = "image/jpeg")
    {
        Content part;
        part.part_type = Content::type::file_uri;
        part.data = file_uri;
        part.mime_type = mime_type;
        parts_.push_back(part);
        return *this;
    }

    // Safety settings
    struct safety_setting {
        std::string category;
        std::string threshold;
    };

    Request &add_safety_setting(const std::string &category,
                                const std::string &threshold)
    {
        safety_settings_.push_back({category, threshold});
        return *this;
    }

    // Generation config shortcuts
    Request &set_candidate_count(int count)
    {
        candidate_count_ = count;
        return *this;
    }

  private:
    std::vector<Content> parts_;
    std::vector<safety_setting> safety_settings_;
    std::optional<int> candidate_count_;
};

class Response : public apis::ai::GenericResponse
{
  public:
    Response(std::string raw, long http_code)
        : GenericResponse(std::move(raw), http_code)
    {
    }

    bool process() override;

    // Prompt feedback
    std::optional<std::string> prompt_feedback() const
    {
        return prompt_feedback_;
    }

    // Finish reason for first candidate
    std::optional<std::string> finish_reason() const { return finish_reason_; }

    // Safety ratings
    struct safety_rating {
        std::string category;
        std::string probability;
    };
    const std::vector<safety_rating> &safety_ratings() const
    {
        return safety_ratings_;
    }

    // Usage metadata
    std::optional<int> prompt_token_count() const { return prompt_tokens_; }
    std::optional<int> candidates_token_count() const
    {
        return candidates_tokens_;
    }
    std::optional<int> total_token_count() const { return total_tokens_; }

  private:
    std::optional<std::string> prompt_feedback_;
    std::optional<std::string> finish_reason_;
    std::vector<safety_rating> safety_ratings_;
    std::optional<int> prompt_tokens_;
    std::optional<int> candidates_tokens_;
    std::optional<int> total_tokens_;
};

class Client : public apis::ai::GenericClient
{
  public:
    Client(const std::string &api_key, CURL *curl);

    Response send(const Request &request);
};

std::unique_ptr<Client> new_client(const std::string &key);

} // namespace apis::google
