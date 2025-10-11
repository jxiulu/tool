#include "framework.hpp"
#include "ml.hpp"

namespace {
bool ReadFileToBytes(const fs::path &path, std::vector<unsigned char> &buffer) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        std::cerr << "[ERR] Unable to open file: " << path << '\n';
        return false;
    }

    ifs.seekg(0, std::ios::end);
    std::streamsize size = ifs.tellg();
    if (size < 0) {
        std::cerr << "[ERR] Failed to determine size for: " << path << '\n';
        return false;
    }
    ifs.seekg(0, std::ios::beg);

    buffer.resize(static_cast<size_t>(size));
    if (size > 0) {
        if (!ifs.read(reinterpret_cast<char *>(buffer.data()), size)) {
            std::cerr << "[ERR] Failed reading file: " << path << '\n';
            return false;
        }
    }
    return true;
}

bool ParseArguments(int argc, const char *argv[], std::string &keyPath,
                    std::string &projectId, fs::path &targetDirectory) {
    targetDirectory = fs::current_path();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--key" || arg == "-k") && i + 1 < argc) {
            keyPath = argv[++i];
        } else if ((arg == "--project" || arg == "-p") && i + 1 < argc) {
            projectId = argv[++i];
        } else if ((arg == "--dir" || arg == "-d") && i + 1 < argc) {
            targetDirectory = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: foldertranslator [options]\n"
                         "  -k, --key <path>       Path to Google Vision JSON "
                         "credentials\n"
                         "  -p, --project <id>     Override Google Cloud "
                         "project id\n"
                         "  -d, --dir <path>       Directory containing "
                         "images (default: cwd)\n";
            return false;
        } else {
            std::cerr << "[WARN] Unknown argument ignored: " << arg << '\n';
        }
    }

    return true;
}
} // namespace

int main(int argc, const char *argv[]) {
    std::string keypath;
    std::string projectid;
    fs::path targetdir;

    if (!ParseArguments(argc, argv, keypath, projectid, targetdir)) {
        return 0;
    }

    if (keypath.empty()) {
        if (const char *envKey = std::getenv("VISION_KEY_PATH")) {
            keypath = envKey;
        }
    }

    if (keypath.empty()) {
        std::cout << "Please enter the path to your Google Vision OCR key: ";
        if (!std::getline(std::cin, keypath)) {
            keypath.clear();
        }
        if (keypath.empty()) {
            std::cerr << "[ERR] Vision key path not provided.\n";
            return 1;
        }
    }

    if (projectid.empty()) {
        if (const char *envProject = std::getenv("GOOGLE_PROJECT_ID")) {
            projectid = envProject;
        }
    }

    std::error_code directory_error;
    fs::path normalized_directory = fs::absolute(targetdir, directory_error);
    if (directory_error) {
        std::cerr << "[ERR] Failed to resolve directory: " << targetdir << " ("
                  << directory_error.message() << ")\n";
        return 1;
    }

    if (!fs::exists(normalized_directory)) {
        std::cerr << "[ERR] Directory does not exist: " << normalized_directory
                  << '\n';
        return 1;
    }

    CURLcode curlInit = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (curlInit != CURLE_OK) {
        std::cerr << "[ERR] curl_global_init failed: "
                  << curl_easy_strerror(curlInit) << '\n';
        return 1;
    }
    struct CurlGlobalGuard {
        ~CurlGlobalGuard() { curl_global_cleanup(); }
    } curlGuard;

    std::unique_ptr<MLClient> client;
    try {
        client = std::make_unique<MLClient>(keypath, projectid);
    } catch (const std::exception &ex) {
        std::cerr << "[ERR] " << ex.what() << '\n';
        return 1;
    }

    if (!client->success) {
        std::cerr << "[ERR] Failed to initialize MLClient\n";
        return 1;
    }

    const std::vector<std::string> validExtensions = {".png", ".jpg", ".jpeg"};
    std::cout << "Scanning directory: " << normalized_directory << '\n';

    bool anything_processed = false;
    for (const auto &entry : fs::directory_iterator(normalized_directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto &path = entry.path();
        std::string extension = path.extension().string();
        std::transform(
            extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (std::find(validExtensions.begin(), validExtensions.end(),
                      extension) == validExtensions.end()) {
            continue;
        }

        std::vector<unsigned char> fileBytes;
        if (!ReadFileToBytes(path, fileBytes)) {
            continue;
        }

        std::cout << "\n[Processing] " << path.filename() << '\n';
        std::string detectedText;
        try {
            detectedText = client->DetectText(fileBytes);
        } catch (const std::exception &ex) {
            std::cerr << "  [ERR] " << ex.what() << '\n';
            continue;
        }
        if (detectedText.empty()) {
            std::cout << "  (no text detected)\n";
        } else {
            std::cout << "  Detected text:\n" << detectedText << '\n';
        }
        anything_processed = true;
    }

    if (!anything_processed) {
        std::cout << "No supported image files found in "
                  << normalized_directory << '\n';
    }

    return 0;
}
