// config imp

#include "config.hpp"
#include <filesystem>
#include <fstream>

namespace setman
{

static std::string without_padding_whitespace(std::string in)
{
    constexpr char whitespace[] = " \t\r\n";
    size_t start = in.find_first_not_of(whitespace);
    size_t end = in.find_last_not_of(whitespace);

    if (start == std::string::npos)
        return "";
    return in.substr(start, end - start + 1);
}

std::expected<Config, Error> load_config(const fs::path &path)
{
    if (!fs::exists(path)) {
        return std::unexpected(
            Error(code::file_doesnt_exist, path.string() + " does not exist."));
    }

    std::ifstream file(path);
    if (!file) {
        return std::unexpected(
            Error(code::file_open_failed, "Failed to open " + path.string()));
    }

    Config cfg;
    std::string line;
    int linepos = 0;

    while (std::getline(file, line)) {
        linepos++;

        line = without_padding_whitespace(line);

        // skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // skip [headers]
        if (line[0] == '[') {
            continue;
        }

        size_t eqpos = line.find('=');
        if (eqpos == std::string::npos) {
            continue; // invalid line
        }

        std::string key = line.substr(0, eqpos);
        std::string value = line.substr(eqpos + 1);

        key = without_padding_whitespace(key);
        value = without_padding_whitespace(value);

        if (!value.empty() &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.length() - 2);
        }

        cfg.set(key, value);
    }

    return cfg;
}

std::optional<fs::path> find_config(const std::string &filename)
{
    fs::path cdcfg = fs::current_path() / filename;
    if (fs::exists(cdcfg))
        return cdcfg;

    const char *home = std::getenv("HOME");
    if (home) {
        fs::path homecfg = fs::path(home) / (".settei_man_" + filename);
        if (fs::exists(homecfg))
            return homecfg;
    }

    return std::nullopt;
}

std::optional<std::string> Config::find(const std::string &key) const
{
    auto it = map_.find(key);
    if (it == map_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::string Config::find_or(const std::string &key,
                            const std::string &def) const
{
    auto it = map_.find(key);
    if (it == map_.end()) {
        return def;
    }
    return it->second;
}

bool Config::has(const std::string &key) const
{
    return map_.find(key) != map_.end();
}

std::vector<std::string> Config::keys() const
{
    std::vector<std::string> result;
    result.reserve(map_.size());

    for (const auto &[key, _] : map_) {
        result.push_back(key);
    }
    return result;
}

void Config::set(std::string key, std::string val) { map_[key] = val; }

} // namespace setman
