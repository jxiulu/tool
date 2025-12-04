#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

using nlohmann::json;

namespace setman::ai
{

enum class role {
    system,
    user,
    assistant,
    tool,
};

struct message {
    role role;
    std::string content;
};

struct openrouter_request {
    std::string endpoint = "https://openrouter.ai/api/v1/chat/completions";
    std::string model;
    std::vector<message> messages;
    std::optional<std::vector<std::string>> models;
    std::optional<double> temperature;
    std::optional<int> max_tokens;
    std::optional<double> top_p;
    std::optional<int> top_k;
    std::optional<double> frequency_penalty;
    std::optional<double> presence_penalty;
    std::vector<std::string> stop_sequences;
    bool stream = false;
    std::optional<std::string> route;
    std::optional<std::vector<std::string>> provider_order;

    openrouter_request &add_message(role r, const std::string &content);
    openrouter_request &set_model(const std::string &model_name);
    openrouter_request &set_models(const std::vector<std::string> &model_list);
    openrouter_request &set_temperature(double temp);
    openrouter_request &set_max_tokens(int tokens);
    openrouter_request &set_top_p(double p);
    openrouter_request &set_top_k(int k);
    openrouter_request &set_frequency_penalty(double penalty);
    openrouter_request &set_presence_penalty(double penalty);
    openrouter_request &set_stream(bool enable);
    openrouter_request &set_route(const std::string &route_str);
    openrouter_request &
    set_provider_order(const std::vector<std::string> &providers);

    json to_json() const;
};

struct openrouter_response {
    std::vector<std::string> content;
    bool valid;
    std::string error;
    std::string raw_json;

    std::optional<int> prompt_tokens;
    std::optional<int> completion_tokens;
    std::optional<int> total_tokens;
    std::optional<std::string> model_used;
    std::optional<std::string> finish_reason;

    static openrouter_response parse(const std::string &raw, long http_code);
};

class OpenRouterClient
{
  public:
    OpenRouterClient(const std::string &api_key, CURL *curl);
    ~OpenRouterClient();

    openrouter_response chat(const openrouter_request &request);

    void set_api_key(const std::string &key) { api_key_ = key; }

  private:
    std::string api_key_;
    CURL *curl_;
    struct curl_slist *headers_;
};

std::unique_ptr<OpenRouterClient> new_openrouter_client(const std::string &key);

} // namespace setman::ai
