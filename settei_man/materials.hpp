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
class Episode;

//
// Material classes
//

class Material;
class File;
class Folder;

class Material
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

    virtual ~Material() = default;
    virtual constexpr bool is_folder() const = 0;
    constexpr type type() const { return type_; }

    constexpr const Episode *parent_episode() const { return parent_episode_; }

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
    const Episode *parent_episode_;
    enum type type_;
    fs::path path_;

    Material(const Episode *parent_episode, const fs::path &path, enum type type);
    Material(const Episode *parent, const fs::path &path, enum type type,
             boost::uuids::uuid uuid);
};

class File : public Material
{
  public:
    constexpr bool is_folder() const override { return false; }

    File(const Episode *parent_episode, const fs::path &path,
         enum Material::type type);
};

class Folder : public Material
{
  protected:
    std::vector<std::unique_ptr<Material>> children_;

  public:
    constexpr bool is_folder() const override { return true; }

    constexpr const std::vector<std::unique_ptr<Material>> &children() const
    {
        return children_;
    }

    void add_child(std::unique_ptr<Material> child);
    Material *find_child(const boost::uuids::uuid &uuid);

    Folder(const Episode *parent_episode, const fs::path &path,
           enum Material::type type);
};

class Image : public File
{
  public:
    Image(const Episode *parent, const fs::path &path)
        : File(parent, path, type::file)
    {
    }

    std::expected<std::string, Error> tob64() const;

    std::optional<std::string> ext() const;
    std::expected<size_t, Error> fsize() const;

    std::expected<fs::path, Error> gen_thumbnail(const fs::path &where,
                                                 int maxsz = 256) const;

    std::expected<int, Error> width() const;
    std::expected<int, Error> height() const;

  protected:
  private:
    mutable int cached_width;
    mutable int cached_height;
};

class Keyframe : public File
{
  public:
    enum class type { lo, ls, lss, le, lk, ka, kas, kass, kae, other };

    Keyframe(const Episode *parent, const fs::path &path, const char cel,
             const type type)
        : File(parent, path, Material::type::keyframe), cel_(cel), kftype_(type)
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

//
// functions
//

namespace setman::materials
{

inline boost::uuids::uuid generate_uuid()
{
    static boost::uuids::random_generator gen;
    return gen();
}

std::pair<std::regex, std::vector<std::string>>
build_regex(const std::string &naming_convention);

std::expected<bool, Error> isimg(const fs::path &file);

std::string tob64(const unsigned char *buf, size_t len);
std::string tob64(const std::vector<unsigned char> &data);

std::optional<std::string> file_ext(const fs::path &path);
std::expected<size_t, Error> file_size(const fs::path &path);

std::expected<std::vector<unsigned char>, Error>
file_tobytes(const fs::path &path);

std::expected<std::string, Error> img_tob64(const fs::path &path);

std::expected<std::pair<int, int>, Error>
img_dimensions(const fs::path &path); // <width, height>

} // namespace setman::materials
