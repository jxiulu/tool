// TranslationService
#pragma once

// curl
#include <curl/curl.h>

// setman
#include "ai_endpoints/deepl.hpp"
#include "ai_endpoints/openrouter.hpp"
#include "conversations/conversation.hpp"
#include "language.hpp"

namespace setman
{

class Conversation;

class TranslationService
{
  public:
    enum class SupportedClient {
        deepl,
        openrouter,
    };

    TranslationService(CURL *curl, enum language target_language,
                       setman::ai::OpenRouterClient *openrouter,
                       setman::ai::DeepLClient *deepl)
        : curl_(curl), target_language_(target_language)
    {
        if (!openrouter)
            client_choice_ = SupportedClient::deepl;
        else if (!deepl)
            client_choice_ = SupportedClient::openrouter;

        // placeholder. prefer deepl for now.
    }

    ~TranslationService() = default;

    void choose_client(SupportedClient choice) { client_choice_ = choice; }
    setman::ai::OpenRouterClient *openrouter_client() const
    {
        return openrouter_;
    }
    setman::ai::DeepLClient *deepl_client() const { return deepl_; }

    constexpr language target_language() const { return target_language_; }
    TranslationService &set_target_language(language target)
    {
        target_language_ = target;
        return *this;
    }

    constexpr std::optional<language> source_language() const
    {
        return source_language_;
    }
    TranslationService &set_source_language(language source)
    {
        source_language_ = source;
        return *this;
    }
    TranslationService &auto_detect_source_language()
    {
        source_language_ = std::nullopt;
        return *this;
    }

    std::string translate(const std::string &message);

    void translate(Conversation &conversation); // todo

  private:
    CURL *curl_;

    enum language target_language_;
    std::optional<language> source_language_;

    SupportedClient client_choice_;

    setman::ai::OpenRouterClient *openrouter_;
    setman::ai::DeepLClient *deepl_;
};

} // namespace setman
