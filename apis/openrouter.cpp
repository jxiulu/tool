// openrouter client implementation

#include "openrouter.hpp"
#include <curl/curl.h>

namespace apis::openrouter
{

std::unique_ptr<Client> new_client(const std::string &key)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return nullptr;

    return std::make_unique<Client>(key, curl);
}

Client::Client(const std::string &api_key, CURL *curl)
    : GenericClient(api_key, curl)
{
    headers_ = curl_slist_append(headers_, "Content-Type: application/json");
    headers_ = curl_slist_append(
        headers_, std::string("Authorization: Bearer " + api_key).c_str());

    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, apis::write_callback);
}

json Request::payload() const
{
    json payload;

    // Model selection
    if (models_.has_value() && !models_->empty()) {
        payload["models"] = *models_;
    } else if (!model_.empty()) {
        payload["model"] = model_;
    }

    // Messages
    json messages_array = json::array();
    for (const auto &msg : messages_) {
        json msg_obj;
        switch (msg.role) {
        case apis::ai::role::system:
            msg_obj["role"] = "system";
            break;
        case apis::ai::role::user:
            msg_obj["role"] = "user";
            break;
        case apis::ai::role::assistant:
            msg_obj["role"] = "assistant";
            break;
        case apis::ai::role::tool:
            msg_obj["role"] = "tool";
            break;
        }
        msg_obj["content"] = msg.content;
        messages_array.push_back(msg_obj);
    }
    payload["messages"] = messages_array;

    // Optional parameters
    if (temperature_.has_value())
        payload["temperature"] = *temperature_;
    if (max_tokens_.has_value())
        payload["max_tokens"] = *max_tokens_;
    if (top_p_.has_value())
        payload["top_p"] = *top_p_;
    if (top_k_.has_value())
        payload["top_k"] = *top_k_;
    if (frequency_penalty_.has_value())
        payload["frequency_penalty"] = *frequency_penalty_;
    if (presence_penalty_.has_value())
        payload["presence_penalty"] = *presence_penalty_;

    if (!stop_sequences_.empty())
        payload["stop"] = stop_sequences_;

    if (stream_)
        payload["stream"] = true;

    // OpenRouter-specific options
    if (route_.has_value())
        payload["route"] = *route_;
    if (provider_order_.has_value())
        payload["provider"] = {{"order", *provider_order_}};

    return payload;
}

Response Client::chat(const Request &request)
{
    std::string response_body;
    std::string request_body = request.payload().dump();

    curl_easy_setopt(curl_, CURLOPT_URL, request.endpoint().c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, request_body.size());
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body);

    CURLcode ec = curl_easy_perform(curl_);
    if (ec != CURLE_OK) {
        Response res("", 0);
        res.invalidate(std::string("[CURL ERROR] ") + curl_easy_strerror(ec));
        return res;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);

    return Response(response_body, http_code);
}

bool Response::process()
{
    // Extract model used
    if (json_.contains("model")) {
        const auto &model = json_["model"];
        if (model.is_string()) {
            model_used_ = model.get<std::string>();
        }
    }

    // Extract choices array
    json *choices = find("choices");
    if (!choices) {
        invalidate("[OPENROUTER] Missing 'choices' field in response");
        return false;
    }

    if (!choices->is_array()) {
        invalidate("[OPENROUTER] 'choices' field is not an array");
        return false;
    }

    if (choices->empty()) {
        invalidate("[OPENROUTER] 'choices' array is empty");
        return false;
    }

    content_.reserve(choices->size());

    // Process each choice
    for (size_t i = 0; i < choices->size(); i++) {
        const auto &choice = (*choices)[i];

        if (!choice.contains("message")) {
            invalidate("[OPENROUTER] Choice at index " + std::to_string(i) +
                       " missing 'message' field");
            return false;
        }

        const auto &message = choice["message"];
        if (!message.contains("content")) {
            invalidate("[OPENROUTER] Message at index " + std::to_string(i) +
                       " missing 'content' field");
            return false;
        }

        if (!message["content"].is_string()) {
            invalidate("[OPENROUTER] Message 'content' at index " +
                       std::to_string(i) + " is not a string");
            return false;
        }

        // Extract content
        const auto &content_str =
            message["content"].get_ref<const std::string &>();
        content_.emplace_back(content_str);

        // Extract finish_reason from first choice
        if (i == 0 && choice.contains("finish_reason")) {
            const auto &reason = choice["finish_reason"];
            if (reason.is_string()) {
                finish_reason_ = reason.get<std::string>();
            }
        }
    }

    // Extract usage information
    if (json_.contains("usage")) {
        const auto &usage = json_["usage"];

        if (usage.contains("prompt_tokens") &&
            usage["prompt_tokens"].is_number_integer()) {
            prompt_tokens_ = usage["prompt_tokens"].get<int>();
        }

        if (usage.contains("completion_tokens") &&
            usage["completion_tokens"].is_number_integer()) {
            completion_tokens_ = usage["completion_tokens"].get<int>();
        }

        if (usage.contains("total_tokens") &&
            usage["total_tokens"].is_number_integer()) {
            total_tokens_ = usage["total_tokens"].get<int>();
        }
    }

    return true;
}

} // namespace apis::openrouter
