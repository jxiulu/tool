// Conversations

#pragma once

#include <chrono>
#include <string>
#include <vector>

// setman
#include "language.hpp"

namespace setman
{

class Company;

namespace conversations
{

struct message {
    std::string username;
    std::string original;
    language original_language;

    std::string translation;
    language translated_language;
    std::chrono::system_clock::time_point timestamp;

    message(const std::string &username, const std::string &content,
            language original)
        : username(username), original(content), original_language(original),
          timestamp(std::chrono::system_clock::now())
    {
    }
};

class Conversation
{
  public:
    Conversation(Company *company) : company_(company) {}
    ~Conversation() = default;

    constexpr const std::vector<message> &messages() const { return messages_; }

  private:
    Company *company_;

    std::vector<message> messages_;
};

} // namespace conversations

} // namespace setman
