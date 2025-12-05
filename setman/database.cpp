// Database
// implementation
#include "database.hpp"

namespace setman
{

Database::Database(const path &location) : database_(nullptr)
{
    sqlite3_open(location.c_str(), &database_);
}

Database::~Database()
{
    if (database_)
        sqlite3_close(database_);
}

Error Database::init_schema()
{
    const char *schema = R"(
          CREATE TABLE IF NOT EXISTS companies (
              uuid TEXT PRIMARY KEY,
              name TEXT NOT NULL
          );

          CREATE TABLE IF NOT EXISTS series (
              uuid TEXT PRIMARY KEY,
              parent_company_uuid TEXT,
              name TEXT NOT NULL,
              naming_convention TEXT,
              FOREIGN KEY(company_uuid) REFERENCES companies(uuid)
          );

          CREATE TABLE IF NOT EXISTS episodes (
              uuid TEXT PRIMARY KEY,
              parent_series_uuid TEXT,
              number INTEGER,
              location TEXT,
              up_folder TEXT,
              cels_folder TEXT,
              FOREIGN KEY(series_uuid) REFERENCES series(uuid)
          );

          CREATE TABLE IF NOT EXISTS materials (
              uuid TEXT PRIMARY KEY,
              parent_episode_uuid TEXT,
              type TEXT,
              parent_uuid TEXT,
              path TEXT,
              FOREIGN KEY(episode_uuid) REFERENCES episodes(uuid)
          );

          CREATE TABLE IF NOT EXISTS tags (
              material_uuid TEXT,
              tag TEXT,
              FOREIGN KEY(material_uuid) REFERENCES materials(uuid)
          );
      )";

    char *err_msg;
    if (sqlite3_exec(database_, schema, nullptr, nullptr, &err_msg) !=
        SQLITE_OK) {
        std::string error = err_msg;
        sqlite3_free(err_msg);
        return {Code::database_error, error};
    }

    return Code::success;
}

} // namespace setman
