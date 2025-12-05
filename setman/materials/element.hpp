// ProductionElement

#pragma once

#include "material.hpp"
#include "uuid.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <string>
#include <unordered_set>
#include <vector>

namespace setman
{

class Series;

namespace materials
{

class Element
{
  public:
    Element(Series *series, const std::string &name)
        : series_(series), name_(name), uuid_(setman::generate_uuid())
    {
    }
    Element(Series *series, const std::string &name,
            const boost::uuids::uuid &uuid)
        : series_(series), name_(name), uuid_(uuid)
    {
    }
    ~Element() = default;

    constexpr const setman::Series *series() const { return series_; }

    constexpr const boost::uuids::uuid &uuid() const { return uuid_; }

    constexpr const std::string &name() const { return name_; }
    void rename(const std::string &name) { name_ = name; }
    void alias_to_name(int alias_index);

    // alias

    constexpr const std::vector<std::string> &aliases() const
    {
        return aliases_;
    }
    constexpr const std::string &alias_at(int index) const
    {
        return aliases_.at(index);
    }

    void add_alias(std::string &alias) { aliases_.push_back(std::move(alias)); }
    void delete_alias(int index) { aliases_.erase(aliases_.begin() + index); }
    void rename_alias(int index, std::string &new_alias)
    {
        aliases_.at(index) = std::move(new_alias);
    }
    bool has_alias(const std::string &alias);

    // tags

    constexpr const std::vector<std::string> &tags() const { return tags_; }
    constexpr const std::string &tag_at(int index) const
    {
        return tags_.at(index);
    }

    void add_tag(std::string &tag) { tags_.push_back(std::move(tag)); }
    void delete_tag(int index) { tags_.erase(tags_.begin() + index); }
    void rename_tag(int index, std::string &new_tag)
    {
        tags_.at(index) = new_tag;
    }
    bool has_tag(const std::string &tag);

    // mentions

    constexpr const std::unordered_set<GenericMaterial *> &mentions() const
    {
        return mentions_;
    }

  private:
    const setman::Series *series_;
    const boost::uuids::uuid uuid_;
    std::string name_;
    std::vector<std::string> aliases_;
    std::vector<std::string> tags_;

    std::unordered_set<GenericMaterial *> mentions_;
};

} // namespace materials
//
} // namespace setman
