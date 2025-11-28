// material classes implementation

#include "materials.hpp"
#include <array>
#include <cstring>
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

std::expected<bool, setman::error> is_image(const fs::path &path) {
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

std::string encode_base64(const unsigned char *bytes, size_t len) {
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

std::string encode_base64(const std::vector<unsigned char> &bytes) {
    if (bytes.empty()) {
        return {};
    }
    return encode_base64(bytes.data(), bytes.size());
}

std::expected<std::vector<unsigned char>, setman::error>
read_to_bytes(const fs::path &path) {
    std::vector<unsigned char> buffer;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        return std::unexpected(setman::error(setman::code::file_open_failed));
    }

    ifs.seekg(0, std::ios::end);
    std::streamsize size = ifs.tellg();
    if (size < 0) {
        return std::unexpected(
            setman::error(setman::code::file_size_count_failed));
    }
    ifs.seekg(0, std::ios::beg);

    buffer.resize(static_cast<size_t>(size));
    if (size > 0) {
        if (!ifs.read(reinterpret_cast<char *>(buffer.data()), size)) {
            return std::unexpected(
                setman::error(setman::code::file_read_failed));
        }
    }
    return buffer;
}

} // namespace setman::materials
