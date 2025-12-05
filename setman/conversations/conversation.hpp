// Conversations

#pragma once

#include <chrono>
#include <ranges>
#include <string>
#include <vector>

// boost
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

// setman
#include "language.hpp"

namespace setman
{

class Company;

template <typename T>
using VecSubrange = std::ranges::subrange<typename std::vector<T>::iterator>;

template <typename T>
using ConstVecSubrange =
    std::ranges::subrange<typename std::vector<T>::const_iterator>;

class Conversation
{
  private:
    class Message
    {
      public:
        Message(const std::string &username, const std::string &content);

        Message(const std::string &username, const std::string &content,
                const boost::uuids::uuid &uuid);

        const std::string &username() const { return username_; }
        void correct_username(const std::string &username);

        const std::string &original() const { return original_; }
        void correct_original(const std::string &content)
        {
            original_ = content;
        }

        language original_language() const { return original_language_; }
        void correct_original_language(language language)
        {
            original_language_ = language;
        }

        const std::optional<std::string> &translation() const
        {
            return translation_;
        }
        void new_translation(const std::string &content, language language);
        void clear_translation();

        const std::chrono::system_clock::time_point &timestamp() const
        {
            return timestamp_;
        }

        const boost::uuids::uuid &uuid() const { return uuid_; }

      private:
        std::string username_;
        std::string original_;
        language original_language_;

        std::optional<std::string> translation_;
        language translated_language_;
        std::chrono::system_clock::time_point timestamp_;
        boost::uuids::uuid uuid_;
    };

    class Search
    {
      public:
        friend class Conversation;
        Search(const std::string query)
            : query_(query), case_sensitive_(false),
              filter_language_(language::not_determined),
              search_username_(false)
        {
        }

        // public interface for setting query options

        Search &case_sensitive(bool opt)
        {
            case_sensitive_ = opt;
            return *this;
        }
        Search &language(language opt)
        {
            filter_language_ = opt;
            return *this;
        }
        Search &search_username(bool opt)
        {
            search_username_ = opt;
            return *this;
        }

      private:
        std::string query_;
        bool case_sensitive_;
        enum language filter_language_;
        bool search_username_;
    };

  public:
    Conversation(Company *company);
    ~Conversation() = default;

    const std::vector<Message> &history() const { return history_; }

    ConstVecSubrange<Message> last(size_t number) const;
    Message &last() { return history_.back(); }

    size_t length() const;
    const Message *find_message(const boost::uuids::uuid &uuid);
    const boost::uuids::uuid &new_message(const std::string &username,
                                          const std::string &content);

    void remove_message(const boost::uuids::uuid &uuid);
    void clear() { history_.clear(); }

    std::vector<const Message *> search(Search &search);
    static Search query(const std::string &query);

  private:
    Company *company_;

    std::vector<Message> history_;
};

} // namespace setman
