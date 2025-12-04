#include "openrouter.hpp"
#include "curl_helpers.hpp"
#include <curl/curl.h>

namespace setman::ai
{

std::unique_ptr<OpenRouterClient> new_openrouter_client(const std::string &key)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return nullptr;

    return std::make_unique<OpenRouterClient>(key, curl);
}

OpenRouterClient::OpenRouterClient(const std::string &api_key, CURL *curl)
    : api_key_(api_key), curl_(curl), headers_(nullptr)
{
    headers_ = curl_helpers::add_json_header(headers_);
    headers_ = curl_helpers::add_bearer_auth(headers_, api_key_);
}

OpenRouterClient::~OpenRouterClient()
{
    if (headers_)
        curl_slist_free_all(headers_);
}

openrouter_request &openrouter_request::add_message(role r,
                                                    const std::string &content)
{
    messages.push_back({r, content});
    return *this;
}

openrouter_request &openrouter_request::set_model(const std::string &model_name)
{
    model = model_name;
    return *this;
}

openrouter_request &
openrouter_request::set_models(const std::vector<std::string> &model_list)
{
    models = model_list;
    return *this;
}

openrouter_request &openrouter_request::set_temperature(double temp)
{
    temperature = temp;
    return *this;
}

openrouter_request &openrouter_request::set_max_tokens(int tokens)
{
    max_tokens = tokens;
    return *this;
}

openrouter_request &openrouter_request::set_top_p(double p)
{
    top_p = p;
    return *this;
}

openrouter_request &openrouter_request::set_top_k(int k)
{
    top_k = k;
    return *this;
}

openrouter_request &openrouter_request::set_frequency_penalty(double penalty)
{
    frequency_penalty = penalty;
    return *this;
}

openrouter_request &openrouter_request::set_presence_penalty(double penalty)
{
    presence_penalty = penalty;
    return *this;
}

openrouter_request &openrouter_request::set_stream(bool enable)
{
    stream = enable;
    return *this;
}

openrouter_request &openrouter_request::set_route(const std::string &route_str)
{
    route = route_str;
    return *this;
}

openrouter_request &openrouter_request::set_provider_order(
    const std::vector<std::string> &providers)
{
    provider_order = providers;
    return *this;
}

json openrouter_request::to_json() const
{
    json payload;

    if (models.has_value() && !models->empty()) {
        payload["models"] = *models;
    } else if (!model.empty()) {
        payload["model"] = model;
    }

    json messages_array = json::array();
    for (const auto &msg : messages) {
        json msg_obj;
        switch (msg.role) {
        case role::system:
            msg_obj["role"] = "system";
            break;
        case role::user:
            msg_obj["role"] = "user";
            break;
        case role::assistant:
            msg_obj["role"] = "assistant";
            break;
        case role::tool:
            msg_obj["role"] = "tool";
            break;
        }
        msg_obj["content"] = msg.content;
        messages_array.push_back(msg_obj);
    }
    payload["messages"] = messages_array;

    if (temperature.has_value())
        payload["temperature"] = *temperature;
    if (max_tokens.has_value())
        payload["max_tokens"] = *max_tokens;
    if (top_p.has_value())
        payload["top_p"] = *top_p;
    if (top_k.has_value())
        payload["top_k"] = *top_k;
    if (frequency_penalty.has_value())
        payload["frequency_penalty"] = *frequency_penalty;
    if (presence_penalty.has_value())
        payload["presence_penalty"] = *presence_penalty;

    if (!stop_sequences.empty())
        payload["stop"] = stop_sequences;

    if (stream)
        payload["stream"] = true;

    if (route.has_value())
        payload["route"] = *route;
    if (provider_order.has_value())
        payload["provider"] = {{"order", *provider_order}};

    return payload;
}

openrouter_response OpenRouterClient::chat(const openrouter_request &req)
{
    auto http_resp =
        curl_helpers::post_json(curl_, req.endpoint, req.to_json(), headers_);

    if (!http_resp.error.empty()) {
        return {.content = {},
                .valid = false,
                .error = "[CURL ERROR] " + http_resp.error,
                .raw_json = ""};
    }

    return openrouter_response::parse(http_resp.body, http_resp.http_code);
}

openrouter_response openrouter_response::parse(const std::string &raw,
                                               long http_code)
{
    openrouter_response result;
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

    if (parsed.contains("model") && parsed["model"].is_string()) {
        result.model_used = parsed["model"].get<std::string>();
    }

    if (!parsed.contains("choices")) {
        result.error = "[OPENROUTER] Missing 'choices' field in response";
        return result;
    }

    const auto &choices = parsed["choices"];
    if (!choices.is_array() || choices.empty()) {
        result.error = "[OPENROUTER] 'choices' field is empty or not an array";
        return result;
    }

    for (size_t i = 0; i < choices.size(); i++) {
        const auto &choice = choices[i];

        if (!choice.contains("message")) {
            result.error = "[OPENROUTER] Choice at index " +
                           std::to_string(i) + " missing 'message' field";
            return result;
        }

        const auto &msg = choice["message"];
        if (!msg.contains("content") || !msg["content"].is_string()) {
            result.error = "[OPENROUTER] Message at index " +
                           std::to_string(i) + " missing or invalid 'content'";
            return result;
        }

        result.content.push_back(msg["content"].get<std::string>());

        if (i == 0 && choice.contains("finish_reason") &&
            choice["finish_reason"].is_string()) {
            result.finish_reason = choice["finish_reason"].get<std::string>();
        }
    }

    if (parsed.contains("usage")) {
        const auto &usage = parsed["usage"];

        if (usage.contains("prompt_tokens") &&
            usage["prompt_tokens"].is_number_integer()) {
            result.prompt_tokens = usage["prompt_tokens"].get<int>();
        }

        if (usage.contains("completion_tokens") &&
            usage["completion_tokens"].is_number_integer()) {
            result.completion_tokens = usage["completion_tokens"].get<int>();
        }

        if (usage.contains("total_tokens") &&
            usage["total_tokens"].is_number_integer()) {
            result.total_tokens = usage["total_tokens"].get<int>();
        }
    }

    result.valid = true;
    return result;
}

} // namespace setman::ai
