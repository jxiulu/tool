// Company
#include "company.hpp"

#include "episode.hpp"
#include "series.hpp"

namespace setman
{

Company::Company(const std::string &name) : name_(name) {}

const std::vector<std::unique_ptr<Series>> &Company::series() const
{
    return series_;
}

void Company::set_path(const fs::path &path) { root_ = path; }

void Company::add_series(const std::string &series_code,
                         const std::string &naming_convention, const int season)
{
    series_.push_back(
        std::make_unique<Series>(this, series_code, naming_convention, season));
}

const class Series *Company::find_series(const std::string &code)
{
    for (auto &entry : series_) {
        if (entry->id() == code)
            return entry.get();
    }
    return nullptr;
}

} // namespace setman
