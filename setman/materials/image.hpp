#pragma once

#include "material.hpp"
#include <chrono>
#include <unordered_map>
#include <unordered_set>

namespace setman
{
class Error;

namespace materials
{

class Element;

template <typename T> using Mentions = std::unordered_set<T *>;

class Image : public File
{
  public:
    Image(const setman::Episode *episode, const fs::path &path, material type);

    constexpr const std::unordered_set<Element *> references() const
    {
        return references_;
    }

    std::expected<int, Error> width() const;
    std::expected<int, Error> height() const;

  private:
    mutable bool dimensions_cached_ = false;
    mutable int cached_width_ = -1;
    mutable int cached_height_ = -1;

    void cache_dimensions() const;

    std::unordered_set<Element *> references_;
};

class Keyframe : public Image
{
  public:
    Keyframe(const setman::Episode *episode, const fs::path &path,
             const char cel, const stage stage);

    constexpr char cel() const { return cel_; }
    constexpr stage stage() const { return type_; }

    std::string identifier() const;

  private:
    enum stage type_;
    char cel_;
};

class Reference : public Image
{
  public:
    Reference(const setman::Episode *episode, const fs::path &file,
              const Mentions<Element> &subjects)
        : Image(episode, file, material::reference), subjects_(subjects)
    {
    }

    constexpr const Mentions<Element> &subjects() const { return subjects_; }

    constexpr const std::optional<std::chrono::year_month_day> &date() const
    {
        return date_;
    }
    void set_date(const std::chrono::year_month_day date) { date_ = date; }

    constexpr const std::optional<std::string> &id() const { return id_; }
    void set_id(const std::string &id) { id_ = id; }

  private:
    std::optional<std::chrono::year_month_day> date_;
    std::optional<std::string> id_;

    Mentions<Element> subjects_;
};

} // namespace materials

} // namespace setman
