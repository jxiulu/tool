// general ai stuff

#pragma once

#include <curl/curl.h>
#include <curl/easy.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

using nlohmann::json;

namespace setman
{

static size_t write_callback(void *contents, size_t size, size_t nmemb,
                             std::string *userp)
{
    size_t total_size = size * nmemb;
    userp->append(static_cast<char *>(contents), total_size);
    return total_size;
}

namespace ai
{

class GenericResponse
{
  public:
    virtual bool process() { return true; }

    GenericResponse &invalidate(const std::string &why)
    {
        valid_ = false;
        error_ = why;
        return *this;
    }

    GenericResponse(std::string raw, long http_code)
        : raw_(std::move(raw)), http_code_(http_code), valid_(true)
    {
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

    std::string error() const
    {
        if (valid_)
            return "[RESPONSE IS VALID]";
        return error_;
    }

    const json *find(const std::string &field) const
    {
        if (!json_.contains(field)) {
            return nullptr;
        }
        return &json_[field];
    }

    json *find(const std::string &field)
    {
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

class GenericClient
{
  public:
    GenericClient(const std::string &api_key, CURL *curl)
        : api_key_(api_key), curl_(curl)
    {
    }

    ~GenericClient() noexcept
    {
        if (headers_)
            curl_slist_free_all(headers_);
        if (curl_)
            curl_easy_cleanup(curl_);
    }

    const std::string &api_key() const { return api_key_; }

    void new_key(const std::string &key)
    {
        api_key_ = key;
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    }

  protected:
    std::string api_key_;
    CURL *curl_;
    struct curl_slist *headers_ = nullptr;
};

enum class role {
    system,
    user,
    assistant,
    tool,
};

struct Message {
    role role;
    std::string content;
};

class GenericRequest
{
  public:
    GenericRequest() = default;
    virtual ~GenericRequest() = default;

    // Message management
    GenericRequest &operator<<(const Message &msg)
    {
        messages_.push_back(msg);
        return *this;
    }

    GenericRequest &add_message(const Message &msg)
    {
        messages_.push_back(msg);
        return *this;
    }

    // Convenience message factory methods
    constexpr static Message user(const std::string &content)
    {
        return {role::user, content};
    }
    constexpr static Message assistant(const std::string &content)
    {
        return {role::assistant, content};
    }
    constexpr static Message system(const std::string &content)
    {
        return {role::system, content};
    }
    constexpr static Message tool(const std::string &content)
    {
        return {role::tool, content};
    }

    // Message history accessors
    constexpr const std::vector<Message> &history() const { return messages_; }
    constexpr int message_count() const { return messages_.size(); }

    void clear_history() { messages_.clear(); }
    void set_history(const std::vector<Message> &history)
    {
        messages_ = history;
    }

    // Model configuration
    const std::string &model() const { return model_; }
    GenericRequest &set_model(const std::string &model)
    {
        model_ = model;
        return *this;
    }

    // Temperature (0.0 - 2.0, controls randomness)
    std::optional<double> temperature() const { return temperature_; }
    GenericRequest &set_temperature(double temp)
    {
        temperature_ = temp;
        return *this;
    }

    // Max tokens to generate
    std::optional<int> max_tokens() const { return max_tokens_; }
    GenericRequest &set_max_tokens(int tokens)
    {
        max_tokens_ = tokens;
        return *this;
    }

    // Top-p (nucleus sampling, 0.0 - 1.0)
    std::optional<double> top_p() const { return top_p_; }
    GenericRequest &set_top_p(double p)
    {
        top_p_ = p;
        return *this;
    }

    // Top-k (limits token selection)
    std::optional<int> top_k() const { return top_k_; }
    GenericRequest &set_top_k(int k)
    {
        top_k_ = k;
        return *this;
    }

    // System prompt (for models that support it separately)
    const std::optional<std::string> &system_prompt() const
    {
        return system_prompt_;
    }
    GenericRequest &set_system_prompt(const std::string &prompt)
    {
        system_prompt_ = prompt;
        return *this;
    }

    // Stop sequences
    const std::vector<std::string> &stop_sequences() const
    {
        return stop_sequences_;
    }
    GenericRequest &add_stop_sequence(const std::string &seq)
    {
        stop_sequences_.push_back(seq);
        return *this;
    }
    GenericRequest &set_stop_sequences(const std::vector<std::string> &seqs)
    {
        stop_sequences_ = seqs;
        return *this;
    }

    // Frequency penalty (-2.0 to 2.0)
    std::optional<double> frequency_penalty() const
    {
        return frequency_penalty_;
    }
    GenericRequest &set_frequency_penalty(double penalty)
    {
        frequency_penalty_ = penalty;
        return *this;
    }

    // Presence penalty (-2.0 to 2.0)
    std::optional<double> presence_penalty() const { return presence_penalty_; }
    GenericRequest &set_presence_penalty(double penalty)
    {
        presence_penalty_ = penalty;
        return *this;
    }

    // Streaming support
    bool stream() const { return stream_; }
    GenericRequest &set_stream(bool enable)
    {
        stream_ = enable;
        return *this;
    }

    // user id
    const std::optional<std::string> &user_id() const { return user_id_; }
    GenericRequest &set_user_id(const std::string &id)
    {
        user_id_ = id;
        return *this;
    }

    // Custom endpoint
    const std::string &endpoint() const { return endpoint_; }
    GenericRequest &set_endpoint(const std::string &endpoint)
    {
        endpoint_ = endpoint;
        return *this;
    }

    // Generate JSON payload (to be overridden by specific implementations)
    virtual json payload() const = 0;

  protected:
    // Core parameters
    std::vector<Message> messages_;
    std::string model_;
    std::string endpoint_;

    // Generation parameters
    std::optional<double> temperature_;
    std::optional<int> max_tokens_;
    std::optional<double> top_p_;
    std::optional<int> top_k_;
    std::optional<std::string> system_prompt_;
    std::vector<std::string> stop_sequences_;
    std::optional<double> frequency_penalty_;
    std::optional<double> presence_penalty_;

    // Additional options
    bool stream_ = false;
    std::optional<std::string> user_id_;
};

} // namespace ai

} // namespace setman
