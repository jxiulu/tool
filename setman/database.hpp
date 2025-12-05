// Database
// saving and loading project state from SQLite Database
#pragma once

// sqlite
#include <sqlite3.h>
// setman
#include "error.hpp"
// std
#include <filesystem>
// boost
#include <boost/uuid/uuid.hpp>

namespace setman
{

namespace materials
{
class Cut;
}

class Company;
class Series;
class Episode;
class Conversation;

class Database
{
  public:
    using path = std::filesystem::path;
    Database(const path &location);
    ~Database();

    Error save_company(const Company &company);
    Error save_series(const Series &series);
    Error save_episode(const Episode &episode);
    Error init_schema();

    sqlite3 *handle() const { return database_; }
  private:
    sqlite3 *database_;
};

} // namespace setman
