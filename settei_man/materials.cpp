// material classes implementation

#include "materials.hpp"
#include <array>
#include <cstddef>
#include <cstring>
#include <expected>
#include <filesystem>
#include <fstream>
#include <map>
#include <system_error>

namespace setman::materials {

//
// material class
//

material::material(const setman::episode *parent_episode, const fs::path &path,
                   material_type type)
    : parent_episode_(parent_episode), path_(path), uuid_(generate_uuid()),
      type_(type) {}

material::material(const setman::episode *parent_episode, const fs::path &path,
                   material_type type, boost::uuids::uuid uuid)
    : parent_episode_(parent_episode), path_(path), uuid_(uuid), type_(type) {}

std::error_code material::move_to(const fs::path &parentfolder) {
    // caller needs to verify the validity of the target path
    std::error_code ec;
    fs::path dest = parentfolder / path().filename();
    fs::rename(path_, parentfolder, ec);
    if (ec)
        return ec;

    path_ = parentfolder / path().filename();

    return std::error_code(); // success
}

//
// file class
//

file::file(const setman::episode *parent_episode, const fs::path &path,
           material_type type)
    : material(parent_episode, path, type) {}

//
// folder class
//

folder::folder(const setman::episode *parent_episode, const fs::path &path,
               material_type type)
    : material(parent_episode, path, type) {}

void folder::add_child(std::unique_ptr<material> child) {
    children_.push_back(std::move(child));
}

material *folder::find_child(const boost::uuids::uuid &uuid) {
    for (auto &child : children_) {
        if (child->uuid() == uuid) {
            return child.get();
        }
    }
    return nullptr;
}

//
// image
//
//

std::expected<std::string, setman::error> image::tob64() const {
    return img_tob64(path_);
}

std::optional<std::string> image::ext() const { return file_ext(path_); }

std::expected<size_t, setman::error> image::fsize() const {
    return setman::materials::file_size(path_);
}

//
// keyframe class
//

//
// keyframe folder class
//

std::vector<keyframe *> kf_folder::keyframes() const {
    std::vector<keyframe *> found = {};
    found.reserve(children_.size());

    for (auto &file : children_) {
        if (auto *keyframe = dynamic_cast<class keyframe *>(file.get())) {
            found.push_back(keyframe);
        }
    }

    found.shrink_to_fit();
    return found;
}

//
// Utility functions
//

std::pair<std::regex, std::vector<std::string>>
build_regex(const std::string &naming_convention) {
    std::string pattern = naming_convention;

    const char *alphanumeric = "([A-Za-z0-9]+)";
    const char *alphanumeric_with_underscores = "([A-Za-z0-9_]+)";
    const char *numeric = "(\\d+)";

    std::map<std::string, std::string> mapping = {
        {"{series}", alphanumeric},
        {"{episode}", numeric},
        {"{scene}", numeric},
        {"{cut}", numeric},
        {"{stage}", alphanumeric_with_underscores}};

    std::vector<std::string> field_order;

    size_t p = 0;
    while (p < pattern.length()) {
        bool found_placeholder = false;

        for (const auto &[placeholder, regex_pattern] : mapping) {
            if (pattern.substr(p, placeholder.length()) == placeholder) {
                std::string field =
                    placeholder.substr(1, placeholder.length() - 2);
                field_order.push_back(field);

                pattern.replace(p, placeholder.length(), regex_pattern);
                p += regex_pattern.length();
                found_placeholder = true;
                break;
            }
        }

        if (!found_placeholder)
            p++;
    }

    return std::pair<std::regex, std::vector<std::string>>(
        std::regex(pattern, std::regex::icase), field_order);
}

std::expected<bool, setman::error> isimg(const fs::path &path) {
    // Check if file exists and is a regular file
    if (!fs::exists(path)) {
        return std::unexpected(setman::error(setman::code::file_doesnt_exist));
    }
    if (!fs::is_regular_file(path)) {
        return std::unexpected(setman::error(setman::code::file_not_valid));
    }

    // Open file and read first 16 bytes for magic number detection
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected(setman::error(setman::code::file_read_failed));
    }

    std::array<unsigned char, 16> header{};
    file.read(reinterpret_cast<char *>(header.data()), header.size());
    const std::streamsize bytes_read = file.gcount();

    if (bytes_read < 4) {
        return false; // Not enough bytes to check
    }

    // PNG: 89 50 4E 47 0D 0A 1A 0A
    if (bytes_read >= 8 && header[0] == 0x89 && header[1] == 0x50 &&
        header[2] == 0x4E && header[3] == 0x47 && header[4] == 0x0D &&
        header[5] == 0x0A && header[6] == 0x1A && header[7] == 0x0A) {
        return true;
    }

    // JPEG: FF D8 FF
    if (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF) {
        return true;
    }

    // GIF: GIF87a or GIF89a
    if (bytes_read >= 6 && (std::memcmp(header.data(), "GIF87a", 6) == 0 ||
                            std::memcmp(header.data(), "GIF89a", 6) == 0)) {
        return true;
    }

    // BMP: BM
    if (header[0] == 0x42 && header[1] == 0x4D) {
        return true;
    }

    // TIFF: II (little-endian) or MM (big-endian)
    if (bytes_read >= 4) {
        // Little-endian: 49 49 2A 00
        if (header[0] == 0x49 && header[1] == 0x49 && header[2] == 0x2A &&
            header[3] == 0x00) {
            return true;
        }
        // Big-endian: 4D 4D 00 2A
        if (header[0] == 0x4D && header[1] == 0x4D && header[2] == 0x00 &&
            header[3] == 0x2A) {
            return true;
        }
    }

    // WebP: RIFF....WEBP
    if (bytes_read >= 12 && header[0] == 0x52 && header[1] == 0x49 &&
        header[2] == 0x46 && header[3] == 0x46 && header[8] == 0x57 &&
        header[9] == 0x45 && header[10] == 0x42 && header[11] == 0x50) {
        return true;
    }

    // ICO/CUR: 00 00 01 00 (ICO) or 00 00 02 00 (CUR)
    if (bytes_read >= 4 && header[0] == 0x00 && header[1] == 0x00 &&
        (header[2] == 0x01 || header[2] == 0x02) && header[3] == 0x00) {
        return true;
    }

    return false;
}

std::optional<std::string> file_ext(const fs::path &path) {
    std::string ext = path.extension().string();
    if (ext.empty())
        return std::nullopt;

    if (ext[0] == '.')
        ext = ext.substr(1);

    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return ext;
}

std::expected<size_t, setman::error> file_size(const fs::path &path) {
    if (!fs::exists(path))
        return std::unexpected(setman::code::file_doesnt_exist);

    std::error_code ec;
    size_t size = fs::file_size(path, ec);
    if (ec)
        return std::unexpected(
            setman::error(setman::code::file_size_count_failed, ec.message()));

    return size;
}

std::string tob64(const unsigned char *bytes, size_t len) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (len == 0) {
        return {};
    }

    std::string output;
    output.reserve(((len + 2) / 3) * 4);

    size_t i = 0;
    const size_t fullchunks = len / 3 * 3;

    for (; i < fullchunks; i += 3) {
        const uint32_t triple = (static_cast<uint32_t>(bytes[i]) << 16) |
                                (static_cast<uint32_t>(bytes[i + 1]) << 8) |
                                static_cast<uint32_t>(bytes[i + 2]);
        output.push_back(alphabet[(triple >> 18) & 0x3F]);
        output.push_back(alphabet[(triple >> 12) & 0x3F]);
        output.push_back(alphabet[(triple >> 6) & 0x3F]);
        output.push_back(alphabet[triple & 0x3F]);
    }

    const size_t remainder = len - i;
    if (remainder == 1) {
        const uint32_t triple = static_cast<uint32_t>(bytes[i]) << 16;
        output.push_back(alphabet[(triple >> 18) & 0x3F]);
        output.push_back(alphabet[(triple >> 12) & 0x3F]);
        output.push_back('=');
        output.push_back('=');
    } else if (remainder == 2) {
        const uint32_t triple = (static_cast<uint32_t>(bytes[i]) << 16) |
                                (static_cast<uint32_t>(bytes[i + 1]) << 8);
        output.push_back(alphabet[(triple >> 18) & 0x3F]);
        output.push_back(alphabet[(triple >> 12) & 0x3F]);
        output.push_back(alphabet[(triple >> 6) & 0x3F]);
        output.push_back('=');
    }

    return output;
}

std::string tob64(const std::vector<unsigned char> &bytes) {
    if (bytes.empty()) {
        return {};
    }
    return tob64(bytes.data(), bytes.size());
}

std::expected<std::vector<unsigned char>, setman::error>
file_tobytes(const fs::path &path) {
    std::vector<unsigned char> buffer;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        return std::unexpected(setman::code::file_open_failed);
    }

    ifs.seekg(0, std::ios::end);
    std::streamsize size = ifs.tellg();
    if (size < 0) {
        return std::unexpected(setman::code::file_size_count_failed);
    }
    ifs.seekg(0, std::ios::beg);

    buffer.resize(static_cast<size_t>(size));
    if (size > 0) {
        if (!ifs.read(reinterpret_cast<char *>(buffer.data()), size)) {
            return std::unexpected(setman::code::file_read_failed);
        }
    }
    return buffer;
}

std::expected<std::string, setman::error> img_tob64(const fs::path &path) {
    auto check = isimg(path);
    if (!check.has_value())
        return std::unexpected(check.error());
    if (check.value() == false)
        return std::unexpected(
            setman::error(setman::code::file_not_valid, "File not an image."));

    auto bytes = file_tobytes(path);
    if (!bytes.has_value())
        return std::unexpected(bytes.error());

    return tob64(bytes.value_or(std::vector<unsigned char>({})));
}

std::expected<std::pair<int, int>, setman::error>
img_dimensions(const fs::path &path) { // <width, height>
    // Check if it's an image first
    auto check = isimg(path);
    if (!check.has_value())
        return std::unexpected(check.error());
    if (check.value() == false)
        return std::unexpected(
            setman::error(setman::code::file_not_valid, "File not an image."));

    // Read first bytes to determine dimensions
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return std::unexpected(setman::error(setman::code::file_open_failed));

    std::array<unsigned char, 24> header{};
    file.read(reinterpret_cast<char *>(header.data()), header.size());
    const std::streamsize bytes_read = file.gcount();

    if (bytes_read < 24)
        return std::unexpected(
            setman::error(setman::code::file_read_failed,
                          "Not enough bytes to read image dimensions"));

    int w = -1, h = -1;

    // PNG: width at bytes 16-19, height at 20-23 (big-endian)
    if (header[0] == 0x89 && header[1] == 0x50 && header[2] == 0x4E &&
        header[3] == 0x47) {
        w = (header[16] << 24) | (header[17] << 16) | (header[18] << 8) |
            header[19];
        h = (header[20] << 24) | (header[21] << 16) | (header[22] << 8) |
            header[23];
    }
    // JPEG: need to scan for SOF marker
    else if (header[0] == 0xFF && header[1] == 0xD8) {
        return std::unexpected(
            setman::error(setman::code::file_not_valid,
                          "JPEG dimension reading not yet implemented"));
    }
    // GIF: width at bytes 6-7, height at 8-9 (little-endian)
    else if ((std::memcmp(header.data(), "GIF87a", 6) == 0 ||
              std::memcmp(header.data(), "GIF89a", 6) == 0)) {
        w = header[6] | (header[7] << 8);
        h = header[8] | (header[9] << 8);
    }
    // BMP: width at bytes 18-21, height at 22-25 (little-endian)
    else if (header[0] == 0x42 && header[1] == 0x4D) {
        w = header[18] | (header[19] << 8) | (header[20] << 16) |
            (header[21] << 24);
        h = header[22] | (header[23] << 8) | (header[24] << 16) |
            (header[25] << 24);
    }

    if (w == -1 || h == -1)
        return std::unexpected(
            setman::error(setman::code::file_not_valid,
                          "Unknown or unsupported image format"));

    return std::make_pair(w, h);
}

} // namespace setman::materials
