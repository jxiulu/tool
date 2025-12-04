// materials

#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <expected>
#include <filesystem>
#include <memory>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>
#include "uuid.hpp"

namespace fs = std::filesystem;

namespace setman
{

class Episode;
class Error;

namespace materials
{

class File;
class Folder;

//
// types
//

enum class stage {
    lo,
    ka,
    ls,
    gs,
    other,
    null,
};

enum class material {
    cut_folder,
    cut_cels_folder,
    cut_file,
    keyframe,
    clipstudio,
    pureref,
    notes,
    folder,
    file,
    reference,
    other,
    null,
};

enum class anime_object {
    character,
    background,
    prop,
    reference,
    other,
    null,
};

//
// functions
//

inline boost::uuids::uuid generate_uuid()
{
    return setman::generate_uuid();
}

std::pair<std::regex, std::vector<std::string>>
build_regex(const std::string &naming_convention);

std::expected<bool, Error> is_image(const fs::path &file);

std::string bytes_to_b64(const unsigned char *buf, size_t len);
std::string bytes_to_b64(const std::vector<unsigned char> &data);

std::optional<std::string> file_extension_of(const fs::path &path);
std::expected<size_t, Error> file_size_of(const fs::path &path);

std::expected<std::vector<unsigned char>, Error>
file_to_bytes(const fs::path &path);

std::expected<std::string, Error> file_to_b64(const fs::path &path);

Error check_if_valid(const fs::path &path,
                     bool write_permission_required = false);

int last_integer_sequence_of(const std::string &sequence);

std::expected<std::pair<int, int>, Error>
image_dimensions_of(const fs::path &path); // <width, height>

//
// materials
//

// ( file is generic
// file directory is a folder )

class GenericMaterial
{
  public:
    virtual ~GenericMaterial() = default;
    virtual constexpr bool is_directory() const = 0;

    constexpr material type() const { return type_; }
    constexpr const setman::Episode *episode() const { return episode_; }
    constexpr const fs::path &file() const { return file_; }
    constexpr const std::string &notes() const { return notes_; }
    constexpr const std::string &alias() const { return alias_; }
    constexpr const boost::uuids::uuid &uuid() const { return uuid_; }
    constexpr const std::unordered_set<std::string>& tags() const { return tags_; }

    std::expected<size_t, Error> disk_size() const;
    std::error_code move_to(const fs::path &parent_location);
    constexpr std::string name() const { return file_.filename().string(); }

    bool file_exists() const;
    bool file_readable() const;
    bool file_writable() const;

    void new_notes(const std::string &notes) { notes_ = notes; }
    void new_alias(const std::string &alias) { alias_ = alias; }

  protected:
    std::string notes_;
    std::string alias_;
    const setman::Episode *episode_;
    enum material type_;
    fs::path file_;

    GenericMaterial(const setman::Episode *episode, const fs::path &path,
                    enum material type);
    GenericMaterial(const setman::Episode *episode, const fs::path &path,
                    enum material type, boost::uuids::uuid uuid);

    void refresh_cache() const;
    void invalidate_cache() const { cache_valid_ = false; }

    std::unordered_set<std::string> tags_;

  private:
    const boost::uuids::uuid uuid_;

    mutable bool cache_valid_;
    mutable bool file_exists_;
    mutable bool is_readable_;
    mutable bool is_writable_;
};

class File : public GenericMaterial
{
  public:
    constexpr bool is_directory() const override { return false; }

    std::expected<std::vector<unsigned char>, Error> to_bytes() const;
    std::expected<std::string, Error> to_b64() const;
    std::optional<std::string> extension() const;

    File(const setman::Episode *episode, const fs::path &path,
         enum material type);

    constexpr std::string file_name() const
    {
        return file_.filename().string();
    }
};

class Folder : public GenericMaterial
{
  public:
    constexpr bool is_directory() const override { return true; }

    constexpr const std::vector<std::unique_ptr<GenericMaterial>> &
    children() const
    {
        return children_;
    }

    void add_child(std::unique_ptr<GenericMaterial> child);
    GenericMaterial *find_child(const boost::uuids::uuid &uuid);

    Folder(const setman::Episode *episode, const fs::path &path,
           enum material type);

  protected:
    std::vector<std::unique_ptr<GenericMaterial>> children_;
};

} // namespace materials
} // namespace setman
