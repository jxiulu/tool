// openrouter client

#pragma once

#include "ai.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

using nlohmann::json;

namespace apis::openrouter
{

class Request : public apis::ai::GenericRequest
{
  public:
    Request() { endpoint_ = "https://openrouter.ai/api/v1/chat/completions"; }

    json payload() const override;

    // OpenRouter-specific options
    const std::optional<std::vector<std::string>> &models() const
    {
        return models_;
    }
    Request &set_models(const std::vector<std::string> &models)
    {
        models_ = models;
        return *this;
    }

    const std::optional<std::string> &route() const { return route_; }
    Request &set_route(const std::string &route)
    {
        route_ = route;
        return *this;
    }

    const std::optional<std::vector<std::string>> &provider_order() const
    {
        return provider_order_;
    }
    Request &set_provider_order(const std::vector<std::string> &providers)
    {
        provider_order_ = providers;
        return *this;
    }

  private:
    std::optional<std::vector<std::string>> models_;
    std::optional<std::string> route_;
    std::optional<std::vector<std::string>> provider_order_;
};

class Response : public apis::ai::GenericResponse
{
  public:
    Response(std::string raw, long http_code)
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

class Client : public apis::ai::GenericClient
{
  public:
    Client(const std::string &api_key, CURL *curl);

    Response chat(const Request &request);
};

std::unique_ptr<Client> new_client(const std::string &key);

} // namespace apis::openrouter
