// Conversation
#pragma once

#include "services/ai.hpp"
#include "unordered_set"

namespace setman
{

namespace conversation
{

struct message {
    std::string name;
    std::string message;
};

class Conversation
{
  public:
  private:
};

class ConversationModule
{
  public:
    using Conversations = std::unordered_set<std::unique_ptr<Conversation>>;

  private:
    Conversations conversations_;
};

} // namespace conversation

} // namespace setman
