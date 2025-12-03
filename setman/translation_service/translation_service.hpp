// TranslationService
#pragma once

//curl
#include <curl/curl.h>

//std
#include <memory>
#include <unordered_set>

// setman
#include "ai_endpoints/ai.hpp"
#include "ai_endpoints/google.hpp"

namespace setman
{

namespace translation_service
{

enum class language {
    en,
    jp,
};

enum available_clients {
    gemini,
    deepl,
    openrouter,
};


class TranslationService
{
  public:
    template<typename T>
    using Resources = std::unordered_set<std::unique_ptr<T>>;

    TranslationService(CURL *curl, available_clients client_choice,
                       const std::string &client_key, language target_language)
        : curl_(curl), target_language_(target_language)
    {
        switch (client_choice) {
        case (gemini):
            clients_.insert(
                std::make_unique<setman::ai::GoogleClient>(client_key, curl_));
            break;
        case (deepl):
            break;
        case (openrouter):
            break;
        }
    }

    static std::unique_ptr<TranslationService>
    create(language target_language, available_clients client_choice,
             const std::string &client_key)
    {
        CURL *curl = curl_easy_init();
        if (!curl)
            return nullptr;

        return std::make_unique<TranslationService>(
            curl, client_choice, client_key, target_language);
    }

    // todo, general translation w/ context support

  private:
    CURL *curl_;
    enum language target_language_;
    Resources<setman::ai::GenericClient> clients_;
};

} // namespace translation_service

} // namespace setman
