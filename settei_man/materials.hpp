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
}

namespace setman::materials
{

//
// Material classes
//

class Matfile;
class Matfolder;

enum class material_type {
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

class GenericMaterial
{
  public:
    virtual ~GenericMaterial() = default;
    virtual constexpr bool is_folder() const = 0;
    constexpr material_type type() const { return type_; }

    constexpr const setman::Episode *parent_episode() const { return parent_episode_; }

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
    const setman::Episode *parent_episode_;
    enum material_type type_;
    fs::path path_;

    GenericMaterial(const setman::Episode *parent_episode, const fs::path &path,
                    enum material_type type);
    GenericMaterial(const setman::Episode *parent, const fs::path &path,
                    enum material_type type, boost::uuids::uuid uuid);
};

class Matfile : public GenericMaterial
{
  public:
    constexpr bool is_folder() const override { return false; }

    Matfile(const setman::Episode *parent_episode, const fs::path &path,
            enum material_type type);
};

class Matfolder : public GenericMaterial
{
  protected:
    std::vector<std::unique_ptr<GenericMaterial>> children_;

  public:
    constexpr bool is_folder() const override { return true; }

    constexpr const std::vector<std::unique_ptr<GenericMaterial>> &
    children() const
    {
        return children_;
    }

    void add_child(std::unique_ptr<GenericMaterial> child);
    GenericMaterial *find_child(const boost::uuids::uuid &uuid);

    Matfolder(const setman::Episode *parent_episode, const fs::path &path,
              enum material_type type);
};

class Image : public Matfile
{
  public:
    Image(const setman::Episode *parent, const fs::path &path)
        : Matfile(parent, path, material_type::file)
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

enum class keyframe_type { lo, ls, lss, le, lk, ka, kas, kass, kae, other };

class Keyframe : public Matfile
{
  public:
    Keyframe(const setman::Episode *parent, const fs::path &path,
             const std::string &cel, const keyframe_type type)
        : Matfile(parent, path, material_type::keyframe), cel_(cel),
          keyframe_type_(type)
    {
    }

    constexpr const std::string &cel() const { return cel_; }
    constexpr std::string filename() const
    {
        return path_.filename().stem().string();
    }

    constexpr std::string identifier() const
    {
        std::string name = filename();
        if (name.compare(0, cel_.length(), cel_) == 0) {
            name.erase(0, cel_.length());
        }
        return name;
    }

  private:
    keyframe_type keyframe_type_;
    std::string cel_;
    std::string filename_;
};

//
// utility
//

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
