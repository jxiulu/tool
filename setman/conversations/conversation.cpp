// Conversation
// implementation
#include "conversations/conversation.hpp"

// setman
#include "uuid.hpp"

namespace setman
{

//
// Conversation
//

Conversation::Conversation(Company *company) : company_(company) {}

Conversation::Message *
Conversation::find_message(const boost::uuids::uuid &uuid)
{
    for (auto &entry : history_) {
        if (entry.uuid() == uuid)
            return &entry;
    }
    return nullptr;
}

const boost::uuids::uuid &Conversation::new_message(const std::string &username,
                                                    const std::string &content)
{
    history_.push_back({username, content});
    return history_.back().uuid();
}

size_t Conversation::length() const { return history_.size(); }

ConstVecSubrange<Conversation::Message> Conversation::last(size_t number) const
{
    auto start =
        history_.size() > number ? history_.end() - number : history_.begin();

    return {start, history_.end()};
}

void Conversation::remove_message(const boost::uuids::uuid &uuid)
{
    std::erase_if(history_,
                  [&uuid](const Message &msg) { return msg.uuid() == uuid; });
}

// search

std::vector<const Conversation::Message *> Conversation::search(Search &search)
{
    std::vector<const Message *> results;

    auto contains = [&](const std::string &text) {
        if (search.case_sensitive_) {
            return text.find(search.query_) != std::string::npos;
        } else {
            std::string lower_text = text;
            std::string lower_query = search.query_;
            std::transform(lower_text.begin(), lower_text.end(),
                           lower_text.begin(), ::tolower);
            std::transform(lower_query.begin(), lower_query.end(),
                           lower_query.begin(), ::tolower);
            return lower_text.find(lower_query) != std::string::npos;
        }
    };

    for (const auto &msg : history_) {
        // todo
        // claude placeholder
        if (search.filter_language_ != language::not_determined &&
            msg.original_language() != search.filter_language_) {
            continue;
        }

        bool match = false;

        // Search in original text
        if (contains(msg.original())) {
            match = true;
        }

        // Search in translation if enabled
        // if (search.data().search_translation && msg.translation().has_value()) {
        //     if (contains(msg.translation().value())) {
        //         match = true;
        //     }
        // }

        // Search in username if enabled
        if (search.search_username_ && contains(msg.username())) {
            match = true;
        }

        if (match) {
            results.push_back(&msg);
        }
    }

    return results;
}
Conversation::Search Conversation::query(const std::string &query)
{
    return Conversation::Search(query);
}

//
// Conversation::Message
//

Conversation::Message::Message(const std::string &username,
                               const std::string &content)
    : username_(username), original_(content),
      timestamp_(std::chrono::system_clock::now()),
      uuid_(setman::generate_uuid()),
      original_language_(language::not_determined),
      translated_language_(language::not_determined), translation_(std::nullopt)
{
}

Conversation::Message::Message(const std::string &username,
                               const std::string &content,
                               const boost::uuids::uuid &uuid)
    : username_(username), original_(content),
      timestamp_(std::chrono::system_clock::now()), uuid_(uuid),
      original_language_(language::not_determined),
      translated_language_(language::not_determined), translation_(std::nullopt)
{
}

void Conversation::Message::new_translation(const std::string &content,
                                            language language)
{
    translation_ = content;
    translated_language_ = language;
}
void Conversation::Message::clear_translation()
{
    translated_language_ = language::not_determined;
    translation_ = std::nullopt;
}

} // namespace setman
