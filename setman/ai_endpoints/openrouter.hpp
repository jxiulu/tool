// openrouter client

#pragma once

#include "ai.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

using nlohmann::json;

namespace setman::ai
{

class OpenRouterRequest : public GenericRequest
{
  public:
    OpenRouterRequest() { endpoint_ = "https://openrouter.ai/api/v1/chat/completions"; }

    json payload() const override;

    // OpenRouter-specific options
    const std::optional<std::vector<std::string>> &models() const
    {
        return models_;
    }
    OpenRouterRequest &set_models(const std::vector<std::string> &models)
    {
        models_ = models;
        return *this;
    }

    const std::optional<std::string> &route() const { return route_; }
    OpenRouterRequest &set_route(const std::string &route)
    {
        route_ = route;
        return *this;
    }

    const std::optional<std::vector<std::string>> &provider_order() const
    {
        return provider_order_;
    }
    OpenRouterRequest &set_provider_order(const std::vector<std::string> &providers)
    {
        provider_order_ = providers;
        return *this;
    }

  private:
    std::optional<std::vector<std::string>> models_;
    std::optional<std::string> route_;
    std::optional<std::vector<std::string>> provider_order_;
};

class OpenRouterResponse : public GenericResponse
{
  public:
    OpenRouterResponse(std::string raw, long http_code)
        : GenericResponse(std::move(raw), http_code)
    {
    }

    bool process() override;

    // Usage information
    std::optional<int> prompt_tokens() const { return prompt_tokens_; }
    std::optional<int> completion_tokens() const { return completion_tokens_; }
    std::optional<int> total_tokens() const { return total_tokens_; }

    // Model used
    std::optional<std::string> model_used() const { return model_used_; }

    // Finish reason
    std::optional<std::string> finish_reason() const { return finish_reason_; }

  private:
    std::optional<int> prompt_tokens_;
    std::optional<int> completion_tokens_;
    std::optional<int> total_tokens_;
    std::optional<std::string> model_used_;
    std::optional<std::string> finish_reason_;
};

class OpenRouterClient : public GenericClient
{
  public:
    OpenRouterClient(const std::string &api_key, CURL *curl);

    OpenRouterResponse chat(const OpenRouterRequest &request);
};

std::unique_ptr<OpenRouterClient> new_openrouter_client(const std::string &key);

} // namespace setman::ai
