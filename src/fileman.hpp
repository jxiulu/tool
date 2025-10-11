#include "framework.hpp"

namespace fman {

bool Rename(const fs::path &PathToFile, const std::string &NewName) {
    fs::path newpath = PathToFile.parent_path() / NewName;
    try {
        std::filesystem::rename(PathToFile, newpath);
        std::cout << "Renamed: " << PathToFile << " >> " << newpath << '\n';
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Rename failed: " << e.what() << '\n';
        return false;
    }
}
} // namespace fman
