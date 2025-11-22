// material

#pragma once

#include "types.hpp"
#include <boost/uuid/uuid.hpp>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace org {
class episode;
}

namespace materials {
class material;
class file;
class folder;
class cut;

class material {
  private:
    const org::episode *parent_episode_;
    std::string notes_;
    std::string alias_;
    const boost::uuids::uuid uuid_;
    material_type type_;

  protected:
    fs::path path_;
    material(const org::episode *parent_episode, const fs::path &path,
             material_type type);
    material(const org::episode *parent, const fs::path &path,
             material_type type, boost::uuids::uuid uuid);

  public:
    virtual ~material() = default;
    virtual bool is_folder() const = 0;

    const org::episode *parent_episode() const { return parent_episode_; }
    const fs::path &path() const { return path_; }
    const std::string &notes() const { return notes_; }
    const std::string &alias() const { return alias_; }
    const boost::uuids::uuid &uuid() const { return uuid_; }
    material_type type() const { return type_; }

    std::error_code move_to(const fs::path &location);

    void set_notes(const std::string &notes);
    void set_alias(const std::string &alias);
};

class file : public material {
  public:
    bool is_folder() const override { return false; }
    file(const org::episode *parent_episode, const fs::path &path,
         material_type type);
};

class folder : public material {
  private:
    std::vector<std::unique_ptr<material>> children_;

  public:
    bool is_folder() const override { return true; }

    const std::vector<std::unique_ptr<material>> &children() const;
    void add_child(std::unique_ptr<material> child);
    material *find_child(const boost::uuids::uuid &uuid);

    folder(const org::episode *parent_episode, const fs::path &path,
           material_type type);
};

class cut : public folder {
  private:
    cut_stage stage_code_;
    std::string stage_;
    int num_;
    std::optional<int> scene_num_;
    std::vector<progress_entry> progress_history_;

  public:
    cut(const org::episode *parent_episode, const fs::path &path,
        const std::optional<int> &scene_num, const int number,
        const std::string &stage);

    const std::optional<int> &scene() const { return scene_num_; }
    int number() const { return num_; }
    const cut_stage stage_code() const { return stage_code_; }
    const cut_status status() const;

    void mark(const cut_status new_status);
    bool matches_with(const cut &other) const;
    bool conflicts_with(const cut &other) const;
};

} // namespace materials
