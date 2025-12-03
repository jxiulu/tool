#pragma once

#include "materials.hpp"
#include "Element.hpp"
#include <unordered_map>
#include <unordered_set>

namespace setman
{
class Error;

namespace materials
{

class Image : public File
{
  public:
    Image(const setman::Episode *episode, const fs::path &path, material type);

    constexpr const std::unordered_set<Element*> references() const {
        return references_;
    }

    std::expected<int, Error> width() const;
    std::expected<int, Error> height() const;

  private:
    mutable bool dimensions_cached_ = false;
    mutable int cached_width_ = -1;
    mutable int cached_height_ = -1;

    void cache_dimensions() const;
    
    std::unordered_set<Element*> references_;
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
    Reference(const fs::path &path, const setman::Episode *episode,
              anime_object subject);

  private:
    anime_object subject;
    std::unordered_map<anime_object, std::string> keys_;
};



} // namespace materials

} // namespace setman
