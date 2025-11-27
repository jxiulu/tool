// material classes

#pragma once

#include "types.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <expected>
#include <filesystem>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace setman {

class episode;

}

namespace setman::materials {

//
// classes
//

class material;
class file;
class folder;
class cut;

class material {
  private:
    const boost::uuids::uuid uuid_;

  protected:
    std::string notes_;
    std::string alias_;
    const setman::episode *parent_episode_;
    material_type type_;
    fs::path path_;

    material(const setman::episode *parent_episode, const fs::path &path,
             material_type type);
    material(const setman::episode *parent, const fs::path &path,
             material_type type, boost::uuids::uuid uuid);

  public:
    virtual ~material() = default;
    virtual constexpr bool is_folder() const = 0;
    constexpr material_type type() const { return type_; }

    constexpr const setman::episode *parent_episode() const {
        return parent_episode_;
    }

    constexpr const fs::path &path() const { return path_; }
    constexpr const std::string &notes() const { return notes_; }
    constexpr const std::string &alias() const { return alias_; }
    constexpr const boost::uuids::uuid &uuid() const { return uuid_; }

    std::error_code move_to(const fs::path &location);

    void set_notes(const std::string &notes) { notes_ = notes; }
    void set_alias(const std::string &alias) { alias_ = alias; }
};

class file : public material {
  public:
    constexpr bool is_folder() const override { return false; }

    file(const setman::episode *parent_episode, const fs::path &path,
         material_type type);
};

class folder : public material {
  protected:
    std::vector<std::unique_ptr<material>> children_;

  public:
    constexpr bool is_folder() const override { return true; }

    constexpr const std::vector<std::unique_ptr<material>> &children() const {
        return children_;
    }
    void add_child(std::unique_ptr<material> child);
    material *find_child(const boost::uuids::uuid &uuid);

    folder(const setman::episode *parent_episode, const fs::path &path,
           material_type type);
};

enum class keyframe_type {
    lo,
    ls,
    lss,
    le,
    lk,
    ka,
    kas,
    kass,
    kae,
};

class keyframe : public file {
  public:
    keyframe(const episode *parent_episode, const fs::path &path,
             const char cel, const keyframe_type type)
        : file(parent_episode, path, material_type::keyframe), cel_(cel),
          kftype_(type) {}

    constexpr char cel() const { return cel_; }
    std::string name() const { return path_.filename().stem().string(); }

  private:
    keyframe_type kftype_;
    char cel_;
    std::string name_;
};

class kf_folder : public folder {
  public:
    // todo
    // honestly not worth the trouble. the most important information is in the
    // keyframe
    constexpr char cel() const { return cel_; }

    std::vector<keyframe *> keyframes() const;

    int frame_count() const { return keyframes().size(); }

  private:
    char cel_;
};

//
// functions
//

inline boost::uuids::uuid generate_uuid() {
    static boost::uuids::random_generator gen;
    return gen();
}

std::pair<std::regex, std::vector<std::string>>
build_regex(const std::string &naming_convention);

} // namespace setman::materials
