// episode
#include "episode.hpp"

// materials
#include "materials/cut.hpp"
#include "materials/element.hpp"
#include "materials/material.hpp"

// setman
#include "company.hpp"
#include "error.hpp"
#include "series.hpp"
#include "database.hpp"

//sqlite
#include <sqlite3.h>

namespace setman
{

//
// constructors
//

Episode::Episode(const class Series *series, const fs::path &parent_dir)
    : series_(series), location_(parent_dir), uuid_(generate_uuid())
{
}

Episode::Episode(const Series *series, const fs::path &location,
                 const boost::uuids::uuid &uuid)
    : series_(series), location_(location), uuid_(uuid)
{
}

//
// sqlite
//

Error Episode::save(Database &db) const
{
    const char *sql =
        "INSERT OR REPLACE INTO episodes "
        "(uuid, series_uuid, number, location, up_folder, cels_folder) "
        "VALUES (?, ?, ?, ?, ?, ?)";

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db.handle(), sql, -1, &stmt, nullptr);

    std::string uuid_str = boost::uuids::to_string(uuid_);
    std::string series_uuid_str = boost::uuids::to_string(series_->uuid());

    sqlite3_bind_text(stmt, 1, uuid_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, series_uuid_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, number_);
    sqlite3_bind_text(stmt, 4, location_.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, up_folder_.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, cels_folder_.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return {Code::database_error, "Failed to save episode"};
    }

    sqlite3_finalize(stmt);
    return Code::success;
}

//
// cuts
//

void Episode::add_cut(std::unique_ptr<materials::Cut> new_cut)
{
    active_cuts_.push_back(std::move(new_cut));
}

void Episode::reserve_active_cuts(size_t n) { active_cuts_.reserve(n); }

std::vector<materials::Cut *> Episode::find_cut(const int number) const
{
    std::vector<materials::Cut *> matches{};
    for (auto &cut : active_cuts_) {
        if (cut->number() == number)
            matches.push_back(cut.get());
    }
    return matches;
}

materials::Cut *Episode::find_cut(const boost::uuids::uuid &uuid) const
{
    for (auto &cut : active_cuts_) {
        if (cut->uuid() == uuid) {
            return cut.get();
        }
    }

    return nullptr;
}

std::vector<materials::Cut *>
Episode::find_conflicts(const materials::Cut &cut) const
{
    std::vector<materials::Cut *> duplicates;

    for (auto &entry : active_cuts_) {
        if (entry.get() == &cut)
            continue;

        if (cut.conflicts(*entry)) {
            duplicates.push_back(entry.get());
        }
    }

    for (auto &entry : archived_cuts_) {
        if (entry.get() == &cut)
            continue;

        if (cut.conflicts(*entry)) {
            duplicates.push_back(entry.get());
        }
    }

    return duplicates;
}

Error Episode::up_cut(materials::Cut &cut)
{
    if (cut.status() != materials::status::done)
        throw std::logic_error("Precondition violation: check if cut is marked "
                               "done before upping");

    if (!fs::is_directory(up_folder())) {
        return Code::up_folder_doesnt_exist;
    }

    auto ec = cut.move_to(up_folder());
    if (ec)
        return {Code::generic_filesystem_error, ec.message()};

    cut.mark(materials::status::up);
    return Code::success;
}

//
// materials
//

materials::GenericMaterial *
Episode::find_material(const boost::uuids::uuid &mat_uuid)
{
    for (auto &mat : materials_) {
        if (mat->uuid() == mat_uuid) {
            return mat.get();
        }
    }

    return nullptr;
}

const materials::GenericMaterial *
Episode::find_material(const boost::uuids::uuid &mat_uuid) const
{
    return const_cast<const materials::GenericMaterial *>(
        const_cast<Episode *>(this)->find_material(mat_uuid));
}

void Episode::add_material(std::unique_ptr<materials::GenericMaterial> new_mat)
{
    materials_.push_back(std::move(new_mat));
}

void Episode::reserve_materials(size_t n) { materials_.reserve(n); }

//
// tags
//

const std::unordered_set<materials::GenericMaterial *> &
Episode::mentions_tag(const std::string &tag)
{
    return tag_lookup_[tag];
}

void Episode::refresh_tags()
{
    tag_lookup_.clear();
    for (const auto &material : materials()) {
        for (const std::string &tag : material->tags()) {
            tag_lookup_[tag].insert(material.get());
        }
    }
    for (const auto &entry : active()) {
        for (const std::string &tag : entry->tags()) {
            tag_lookup_[tag].insert(entry.get());
        }
    }
    for (const auto &entry : archived()) {
        for (const std::string &tag : entry->tags()) {
            tag_lookup_[tag].insert(entry.get());
        }
    }
}

//
// elements
//

materials::Element *Episode::find_element(const boost::uuids::uuid &uuid)
{
    for (auto &element : elements_) {
        if (element->uuid() == uuid) {
            return element.get();
        }
    }
    return nullptr;
}

const std::unordered_set<materials::GenericMaterial *> *
Episode::mentions_element(const boost::uuids::uuid &uuid)
{
    materials::Element *element = find_element(uuid);
    if (!element)
        return nullptr;

    return &element->mentions();
}

} // namespace setman
