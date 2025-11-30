// material classes

#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <expected>
#include <filesystem>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "types.hpp"

namespace fs = std::filesystem;

namespace setman
{
class episode;

//
// Material classes
//

class material;
class file;
class folder;

class material
{
  public:
    enum class type {
        cut_folder,
        cut_cels_folder,
        cut_file,
        keyframe,
        csp,
        pur,
        notes,
        folder,
        file,
        other,
        null,
    };

    virtual ~material() = default;
    virtual constexpr bool is_folder() const = 0;
    constexpr type type() const { return type_; }

    constexpr const episode *parent_episode() const { return parent_episode_; }

    constexpr const fs::path &path() const { return path_; }
    constexpr const std::string &notes() const { return notes_; }
    constexpr const std::string &alias() const { return alias_; }
    constexpr const boost::uuids::uuid &uuid() const { return uuid_; }

    std::error_code move_to(const fs::path &location);

    void set_notes(const std::string &notes) { notes_ = notes; }
    void set_alias(const std::string &alias) { alias_ = alias; }

  private:
    const boost::uuids::uuid uuid_;

  protected:
    std::string notes_;
    std::string alias_;
    const episode *parent_episode_;
    enum type type_;
    fs::path path_;

    material(const episode *parent_episode, const fs::path &path, enum type type);
    material(const episode *parent, const fs::path &path, enum type type,
             boost::uuids::uuid uuid);
};

class file : public material
{
  public:
    constexpr bool is_folder() const override { return false; }

    file(const episode *parent_episode, const fs::path &path,
         enum material::type type);
};

class folder : public material
{
  protected:
    std::vector<std::unique_ptr<material>> children_;

  public:
    constexpr bool is_folder() const override { return true; }

    constexpr const std::vector<std::unique_ptr<material>> &children() const
    {
        return children_;
    }

    void add_child(std::unique_ptr<material> child);
    material *find_child(const boost::uuids::uuid &uuid);

    folder(const episode *parent_episode, const fs::path &path,
           enum material::type type);
};

class image : public file
{
  public:
    image(const episode *parent, const fs::path &path)
        : file(parent, path, type::file)
    {
    }

    std::expected<std::string, error> tob64() const;

    std::optional<std::string> ext() const;
    std::expected<size_t, error> fsize() const;

    std::expected<fs::path, error> gen_thumbnail(const fs::path &where,
                                                 int maxsz = 256) const;

    std::expected<int, error> width() const;
    std::expected<int, error> height() const;

  protected:
  private:
    mutable int cached_width;
    mutable int cached_height;
};

class keyframe : public file
{
  public:
    enum class type { lo, ls, lss, le, lk, ka, kas, kass, kae, other };

    keyframe(const episode *parent, const fs::path &path, const char cel,
             const type type)
        : file(parent, path, material::type::keyframe), cel_(cel), kftype_(type)
    {
    }

    constexpr char cel() const { return cel_; }
    std::string name() const { return path_.filename().stem().string(); }

  private:
    type kftype_;
    char cel_;
    std::string name_;
};

} // namespace setman

namespace setman::materials
{

//
// Utility functions
//

inline boost::uuids::uuid generate_uuid()
{
    static boost::uuids::random_generator gen;
    return gen();
}

std::pair<std::regex, std::vector<std::string>>
build_regex(const std::string &naming_convention);

std::expected<bool, error> isimg(const fs::path &file);

std::string tob64(const unsigned char *buf, size_t len);
std::string tob64(const std::vector<unsigned char> &data);

std::optional<std::string> file_ext(const fs::path &path);
std::expected<size_t, error> file_size(const fs::path &path);

std::expected<std::vector<unsigned char>, error>
file_tobytes(const fs::path &path);

std::expected<std::string, error> img_tob64(const fs::path &path);

std::expected<std::pair<int, int>, error>
img_dimensions(const fs::path &path); // <width, height>

} // namespace setman::materials
