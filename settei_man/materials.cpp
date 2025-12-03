// materials

#include "materials.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <expected>
#include <filesystem>
#include <fstream>
#include <map>
#include <system_error>

namespace setman::materials
{

//
//  GenericMaterial
//

GenericMaterial::GenericMaterial(const setman::Episode *parent_episode,
                                 const fs::path &file, enum material type)
    : episode_(parent_episode), file_(file),
      uuid_(materials::generate_uuid()), type_(type)
{
}

GenericMaterial::GenericMaterial(const setman::Episode *parent_episode,
                                 const fs::path &file, enum material type,
                                 boost::uuids::uuid uuid)
    : episode_(parent_episode), file_(file), uuid_(uuid), type_(type)
{
}

void GenericMaterial::refresh_cache() const
{
    std::error_code ec;

    if (!fs::exists(file_, ec)) {
        file_exists_ = false;
        is_readable_ = false;
        is_writable_ = false;
    } else {
        file_exists_ = true;

        std::ifstream read_test(file_);
        is_readable_ = read_test.is_open();
        read_test.close();

        std::ofstream write_test(file_, std::ios::app);
        is_writable_ = write_test.is_open();
        write_test.close();
    }

    cache_valid_ = true;
}

bool GenericMaterial::file_exists() const
{
    if (!cache_valid_) {
        refresh_cache();
    }
    return file_exists_;
}

bool GenericMaterial::file_readable() const
{
    if (!cache_valid_) {
        refresh_cache();
    }
    return is_readable_;
}

bool GenericMaterial::file_writable() const
{
    if (!cache_valid_) {
        refresh_cache();
    }
    return is_writable_;
}
std::error_code GenericMaterial::move_to(const fs::path &parentfolder)
{
    // caller needs to verify the validity of the target path
    std::error_code ec;
    fs::path dest = parentfolder / file().filename();
    fs::rename(file_, dest, ec);
    if (ec)
        return ec;

    file_ = dest;
    invalidate_cache(); // always invalidate after operation

    return std::error_code(); // success
}

std::expected<size_t, Error> GenericMaterial::disk_size() const
{
    return materials::file_size_of(file_);
}

//
// File
//

File::File(const setman::Episode *parent_episode, const fs::path &path,
           material type)
    : GenericMaterial(parent_episode, path, type)
{
}

std::expected<std::vector<unsigned char>, Error> File::to_bytes() const
{
    return materials::file_to_bytes(file_);
}

std::expected<std::string, Error> File::to_b64() const
{
    return materials::file_to_b64(file_);
}

std::optional<std::string> File::extension() const
{
    return materials::file_extension_of(file_);
}

//
// Folder
//

Folder::Folder(const setman::Episode *parent_episode, const fs::path &path,
               material type)
    : GenericMaterial(parent_episode, path, type)
{
}

void Folder::add_child(std::unique_ptr<GenericMaterial> child)
{
    children_.push_back(std::move(child));
}

GenericMaterial *Folder::find_child(const boost::uuids::uuid &uuid)
{
    for (auto &child : children_) {
        if (child->uuid() == uuid) {
            return child.get();
        }
    }
    return nullptr;
}

//
// Image
//

Image::Image(const setman::Episode *parent, const fs::path &path,
             material type)
    : File(parent, path, type)
{
}

void Image::cache_dimensions() const
{
    auto dims = image_dimensions_of(file_);
    if (dims.has_value()) {
        cached_width_ = dims.value().first;
        cached_height_ = dims.value().second;
        dimensions_cached_ = true;
    } else {
        cached_width_ = -1;
        cached_height_ = -1;
        dimensions_cached_ = false;
    }
}

std::expected<int, Error> Image::width() const
{
    if (!dimensions_cached_) {
        cache_dimensions();
    }

    if (dimensions_cached_ && cached_width_ >= 0) {
        return cached_width_;
    }

    return Error(code::file_not_valid, "Could not determine image width");
}

std::expected<int, Error> Image::height() const
{
    if (!dimensions_cached_) {
        cache_dimensions();
    }

    if (dimensions_cached_ && cached_height_ >= 0) {
        return cached_height_;
    }

    return Error(code::file_not_valid, "Could not determine image height");
}

//
// Keyframe
//

Keyframe::Keyframe(const setman::Episode *parent, const fs::path &path,
                   const char cel, const enum stage type)
    : Image(parent, path, material::keyframe), cel_(cel), type_(type)
{
}

std::string Keyframe::identifier() const
{
    std::string identifier = file_name();
    if (identifier.front() == cel_)
        identifier.erase(0, 1);
    return identifier;
}

//
// function
//

std::pair<std::regex, std::vector<std::string>>
build_regex(const std::string &naming_convention)
{
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

std::expected<bool, Error> is_image(const fs::path &path)
{
    // Check if file exists and is a regular file
    if (!fs::exists(path)) {
        return std::unexpected(Error(code::file_doesnt_exist));
    }
    if (!fs::is_regular_file(path)) {
        return std::unexpected(Error(code::file_not_valid));
    }

    // Open file and read first 16 bytes for magic number detection
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected(Error(code::file_read_failed));
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

std::optional<std::string> file_extension_of(const fs::path &path)
{
    std::string ext = path.extension().string();
    if (ext.empty())
        return std::nullopt;

    if (ext[0] == '.')
        ext = ext.substr(1);

    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return ext;
}

std::expected<size_t, Error> file_size_of(const fs::path &path)
{
    if (!fs::exists(path))
        return std::unexpected(code::file_doesnt_exist);

    std::error_code ec;
    size_t size = fs::file_size(path, ec);
    if (ec)
        return std::unexpected(
            Error(code::file_size_count_failed, ec.message()));

    return size;
}

std::string bytes_to_b64(const unsigned char *bytes, size_t len)
{
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

std::string bytes_to_b64(const std::vector<unsigned char> &bytes)
{
    if (bytes.empty()) {
        return {};
    }
    return bytes_to_b64(bytes.data(), bytes.size());
}

std::expected<std::vector<unsigned char>, Error>
file_to_bytes(const fs::path &path)
{
    std::vector<unsigned char> buffer;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        return std::unexpected(code::file_open_failed);
    }

    ifs.seekg(0, std::ios::end);
    std::streamsize size = ifs.tellg();
    if (size < 0) {
        return std::unexpected(code::file_size_count_failed);
    }
    ifs.seekg(0, std::ios::beg);

    buffer.resize(static_cast<size_t>(size));
    if (size > 0) {
        if (!ifs.read(reinterpret_cast<char *>(buffer.data()), size)) {
            return std::unexpected(code::file_read_failed);
        }
    }
    return buffer;
}

std::expected<std::string, Error> file_to_b64(const fs::path &path)
{
    auto check = is_image(path);
    if (!check.has_value())
        return std::unexpected(check.error());
    if (check.value() == false)
        return std::unexpected(
            Error(code::file_not_valid, "File not an image."));

    auto bytes = file_to_bytes(path);
    if (!bytes.has_value())
        return std::unexpected(bytes.error());

    return bytes_to_b64(bytes.value_or(std::vector<unsigned char>({})));
}

std::expected<std::pair<int, int>, Error>
image_dimensions_of(const fs::path &path)
{ // <width, height>
    // Check if it's an image first
    auto check = is_image(path);
    if (!check.has_value())
        return std::unexpected(check.error());
    if (check.value() == false)
        return std::unexpected(
            Error(code::file_not_valid, "File not an image."));

    // Read first bytes to determine dimensions
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return std::unexpected(Error(code::file_open_failed));

    std::array<unsigned char, 24> header{};
    file.read(reinterpret_cast<char *>(header.data()), header.size());
    const std::streamsize bytes_read = file.gcount();

    if (bytes_read < 24)
        return std::unexpected(
            Error(code::file_read_failed,
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
            setman::Error(setman::code::file_not_valid,
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
            Error(code::file_not_valid, "Unknown or unsupported image format"));

    return std::make_pair(w, h);
}

Error check_if_valid(const fs::path &path, bool wp)
{
    std::error_code ec;

    if (!fs::exists(path, ec)) {
        return Error(code::file_doesnt_exist, ec.message());
    }

    std::ifstream read_test(path);
    if (!read_test.is_open()) {
        return Error(code::file_read_failed,
                     "Permission denied reading " + path.string());
    }
    read_test.close();

    if (!wp)
        return Error(code::success, path.string() + " is valid");

    std::ofstream write_test(path);
    if (!write_test.is_open()) {
        return Error(code::file_write_failed,
                     "Permission denied writing to " + path.string());
    }
    write_test.close();

    return Error(code::success, path.string() + " is valid");
}

int last_integer_sequence_of(const std::string &sequence)
{
    std::string found_buffer;
    bool digit_found;

    for (int i = sequence.length() - 1; i >= 0; i--) {
        if (std::isdigit(sequence[i])) {
            found_buffer.push_back(sequence[i]);
            digit_found = true;
        } else if (!digit_found) {
            break;
        }
    }

    std::reverse(found_buffer.begin(), found_buffer.end());
    return std::stoi(found_buffer);
}

} // namespace setman::materials
