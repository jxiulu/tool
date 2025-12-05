#include "image.hpp"
#include "material.hpp"
#include "error.hpp"

namespace setman
{

namespace materials
{

Image::Image(const setman::Episode *parent, const fs::path &path, material type)
    : File(parent, path, type)
{
}

void Image::cache_dimensions() const
{
    auto dims = image_dimensions_of(file_);
    if (dims.has_value()) {
        cached_width_ = dims.value().first;
        cached_height_ = dims.value().second;
        dimensions_cached_ = true;
    } else {
        cached_width_ = -1;
        cached_height_ = -1;
        dimensions_cached_ = false;
    }
}

std::expected<int, Error> Image::width() const
{
    if (!dimensions_cached_) {
        cache_dimensions();
    }

    if (dimensions_cached_ && cached_width_ >= 0) {
        return cached_width_;
    }

    return Error(Code::file_not_valid, "Could not determine image width");
}

std::expected<int, Error> Image::height() const
{
    if (!dimensions_cached_) {
        cache_dimensions();
    }

    if (dimensions_cached_ && cached_height_ >= 0) {
        return cached_height_;
    }

    return Error(Code::file_not_valid, "Could not determine image height");
}

//
// Keyframe
//

Keyframe::Keyframe(const setman::Episode *parent, const fs::path &path,
                   const char cel, const enum stage type)
    : Image(parent, path, material::keyframe), cel_(cel), type_(type)
{
}

std::string Keyframe::identifier() const
{
    std::string identifier = file_name();
    if (identifier.front() == cel_)
        identifier.erase(0, 1);
    return identifier;
}

} // namespace materials

} // namespace setman
