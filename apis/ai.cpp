// general ai implementation

#include "ai.hpp"
#include <fstream>
#include <string>

namespace apis {

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

} // namespace apis
