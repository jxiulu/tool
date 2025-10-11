#include "fileman.hpp"
#include "framework.hpp"

const char *GEMINI_KEY_CSTR = std::getenv("GEMINI_API_KEY");
const std::string GEMINI_API_KEY = GEMINI_KEY_CSTR ? GEMINI_KEY_CSTR : "";
const std::string PROJECT_ID = "arched-waters-471021-e2";

const char *VISION_KEY_PATH_CSTR = std::getenv("VISION_KEY_PATH");
const std::string VISION_KEY_PATH =
    VISION_KEY_PATH_CSTR ? VISION_KEY_PATH_CSTR : "";

std::string ACCESS_TOKEN = {};

static size_t WriteCallbackForCurl(void *contents, size_t size, size_t nmemb,
                                   void *userp) {
    size_t total = size * nmemb;
    std::string *out = static_cast<std::string *>(userp);
    out->append(static_cast<char *>(contents), total);
    return total;
}

jason LoadGoogleServiceAccount(const std::string &JSONKeyPath) {
    std::ifstream ifs(JSONKeyPath);
    if (!ifs) {
        throw std::runtime_error("[RUNERR] Unable to open key JSON");
    }
    jason getKey;
    ifs >> getKey;
    return getKey;
}

std::string CreateSToken(const jason JSONKey) {
    auto tim = std::chrono::system_clock::now();

    std::string privateKey = JSONKey["private_key"];
    std::string clientEmail = JSONKey["client_email"];

    auto unsToken =
        jwt::create()
            .set_issuer(clientEmail)
            .set_audience("https://oauth2.googleapis.com/token")
            .set_issued_at(tim)
            .set_expires_at(tim + std::chrono::seconds(10000))
            .set_payload_claim(
                "scope",
                jwt::claim(std::string(
                    "https://www.googleapis.com/auth/cloud-platform")));

    auto sToken = unsToken.sign(jwt::algorithm::rs256(
        "", privateKey, "", "")); // Passwords strings empty idc
    return sToken;
}

std::string GetAccessToken(const std::string &SToken) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("[RUNERR] [CURL] Failed to easy init");
        return "";
    }

    std::string post_fields = "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-"
                              "type%3Ajwt-bearer&assertion=" +
                              SToken;
    std::string response_data = {};
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(
        headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackForCurl);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    CURLcode respcode = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (respcode != CURLE_OK) {
        throw std::runtime_error("[RUNERR] [CURL] " +
                                 (std::string)curl_easy_strerror(respcode));
        return "";
    }

    auto jsonResp = jason::parse(response_data);
    return jsonResp["access_token"];
}

std::string Translate(const std::string &Input) {
    if (GEMINI_API_KEY.empty()) {
        std::cerr << "\n[ERR] GEMINI_API_KEY Empty\n";
        return Input;
    }

    std::string translated_response;
    CURL *curl = curl_easy_init();

    if (curl) {
        std::string prompt = Input;
        std::string translation_instructions =
            "Translate the name of each file/folder into English. Respond only "
            "with the translated name.";

        jason payload = {{"system_instruction",
                          {{"parts", {{{"text", translation_instructions}}}}}},
                         {"contents", {{{"parts", {{{"text", prompt}}}}}}},
                         {"generationConfig",
                          {{"thinkingConfig", {{"thinkingBudget", 0}}}}}};
        std::string payload_string = payload.dump();

        struct curl_slist *curl_headers = nullptr;
        curl_headers =
            curl_slist_append(curl_headers, "Content-Type: application/json");
        curl_headers = curl_slist_append(
            curl_headers, ("X-goog-api-key: " + GEMINI_API_KEY).c_str());

        const std::string GEMINI_URL =
            "https://generativelanguage.googleapis.com/v1beta/models/"
            "gemini-2.0-flash:generateContent?";

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
        curl_easy_setopt(curl, CURLOPT_URL, GEMINI_URL.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_string.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackForCurl);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &translated_response);
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

        CURLcode reponse_code = curl_easy_perform(curl);
        if (reponse_code != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: "
                      << curl_easy_strerror(reponse_code) << "\n";
        }

        curl_slist_free_all(curl_headers);
        curl_easy_cleanup(curl);
    }

    try {
        auto json_response = jason::parse(translated_response);
        std::string TranslatedInput =
            json_response["candidates"][0]["content"]["parts"][0]["text"];
        return TranslatedInput;
    } catch (...) {
        std::cerr << "Translator function error parsing json response\n";
        std::cerr << "Raw Response:\n" << translated_response << '\n';
        return Input;
    }
}

std::string DetectText(const std::vector<unsigned char> &InputContent) {
    if (ACCESS_TOKEN.empty()) {
        std::cerr << "\n[ERR] ACCESS_TOKEN empty, Vision auth unavailable\n";
        return "";
    }

    std::string b64_content = base64_encode(
        reinterpret_cast<const unsigned char *>(InputContent.data()),
        InputContent.size());

    jason requestPayload = {
        {"requests",
         jason::array(
             {{{"image", {{"content", b64_content}}},
               {"features", jason::array({{{"type", "DOCUMENT_TEXT_DETECTION"},
                                           {"maxResults", 1},
                                           {"model", "builtin/latest"}}})}}})}};
    std::string payloadString = requestPayload.dump();

    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[ERR] [CURL]  Failed to init";
        return "";
    }

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers,
                                ("x-goog-user-project: " + PROJECT_ID).c_str());
    headers = curl_slist_append(
        headers, ("Authorization: Bearer " + ACCESS_TOKEN).c_str());
    headers = curl_slist_append(
        headers, "Content-Type: application/json; charset=utf-8");

    const std::string REQUEST_URL =
        "https://vision.googleapis.com/v1/images:annotate";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, REQUEST_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadString.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackForCurl);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    CURLcode rescode = curl_easy_perform(curl);
    if (rescode != CURLE_OK) {
        std::cerr << "[ERR] [CURL]  curl_easy_perform() failed: "
                  << curl_easy_strerror(rescode) << "\n";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return "";
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        auto jasonsResponse = jason::parse(response);
        if (!jasonsResponse.contains("responses") ||
            jasonsResponse["responses"].empty()) {
            return "";
        }
        const auto &firstResponse = jasonsResponse["responses"][0];
        if (!firstResponse.contains("fullTextAnnotations") ||
            !firstResponse["fullTextAnnotations"].contains("text")) {
            return "";
        }
        return firstResponse["fullTextAnnotations"]["text"].get<std::string>();
    } catch (...) {
        std::cerr << "\nDetect text error: unable to parse response\n\n   [RAW "
                     "RESPONSE]\n\n"
                  << response << "\n    [END RAW]\n\n";
        return "";
    }
}

bool ReadToBytes(const std::filesystem::path &PathToInput,
                 std::vector<unsigned char> &VectorOut) {
    std::ifstream ifs(PathToInput, std::ios::binary);
    if (!ifs) {
        return false;
    }

    ifs.seekg(0, std::ios::end);
    std::streamsize ssize = ifs.tellg();
    if (ssize < 0) {
        return false;
    }

    ifs.seekg(0, std::ios::beg);
    VectorOut.resize(static_cast<size_t>(ssize));
    if (ssize > 0) {
        ifs.read(reinterpret_cast<char *>(VectorOut.data()), ssize);
    }

    return static_cast<bool>(ifs);
}

void RunPiecemeal(std::filesystem::path TargetDirectory, bool IsTestRun) {
    for (const auto &file :
         std::filesystem::directory_iterator(TargetDirectory)) {
        const auto &path = file.path();
        std::string file_name = path.stem().string();
        std::string file_extension = path.extension().string();

        std::string translated_name = Translate(file_name);
        translated_name.erase(
            std::remove(translated_name.begin(), translated_name.end(), '\n'),
            translated_name.end());

        std::string new_name = translated_name + file_extension;

        if (IsTestRun) {
            std::cout << "[Test Run] Would rename \"" << file_name
                      << file_extension << "\" to \n"
                      << translated_name << file_extension << '\n';
        } else {
            fman::Rename(file.path(), new_name);
        }
    }
}

bool CreateTestFiles(const std::filesystem::path &TestFilesDirectory,
                     int NumberOfTestFiles, bool UsedMixedLanguage) {
    const std::vector<std::string> randomFileNames =
        UsedMixedLanguage
            ? std::vector<std::string>{"El_sol_brilla",
                                       "L'eau est_froide",
                                       "Die Katze schläft",
                                       "Le chat est 5",
                                       "123-おはよう、元気???",
                                       "La vie en rose",
                                       "C'est_la_vie",
                                       "Ein Bier, bitte!",
                                       "Dos cervezas por_favor",
                                       "Comment_allez-vous?",
                                       "Que_hora_es?",
                                       "Wie geht's?",
                                       "Alles_gute!",
                                       "Buona giornata!",
                                       "Ciao a tutti!",
                                       "Grazie 1000",
                                       "Arrivederci, Roma",
                                       "Où est la gare?",
                                       "S'il vous plaît",
                                       "あれ_は_誰？"}
            : std::vector<std::string>{
                  "こんにちは_世界",   "おはよう、元気？", "お疲れ様_です！",
                  "日本語が_好き",     "ご飯_食べ_た",     "ありがとう、ね",
                  "はい、そうです",    "いいえ、違います", "ごめんなさい",
                  "すみません_100",    "これ_は_何？",     "あれ_は_誰？",
                  "また_会いましょう", "頑張って！",       "おやすみ_なさい",
                  "楽しかった_ね",     "トイレは_どこ？",  "いくら_ですか",
                  "道に迷った_5",      "もう_一度"};

    if (static_cast<size_t>(NumberOfTestFiles > randomFileNames.size())) {
        std::cout << "i only got about " << randomFileNames.size()
                  << "random names man";
        NumberOfTestFiles = randomFileNames.size();
    }

    std::cout << "[Creating test files]\n";
    for (int i = 0; i < NumberOfTestFiles; i++) {
        std::filesystem::path test_file_path =
            TestFilesDirectory / randomFileNames[i];
        std::ofstream ofs_temp(test_file_path);
        if (!ofs_temp) {
            std::cerr << "  Error creating test file: " << test_file_path
                      << '\n';
            return false;
        }
        std::cout << "  Created file \""
                  << std::filesystem::absolute(test_file_path) << "\"\n";
    }
    return true;
}

void TranslateDemo(std::filesystem::path &TargetDirectory) {
    bool test_run = false;
    std::cout << "Target directory: "
              << std::filesystem::absolute(TargetDirectory).lexically_normal()
              << '\n';

    CreateTestFiles(TargetDirectory, 20, true);
    RunPiecemeal(TargetDirectory, test_run);
}

int run(int argc, char **argv) {
    std::filesystem::path target_directory = ".";
    if (PROJECT_ID.empty()) {
        std::cerr << "\n[ERR] PROJECT_ID Empty\n";
    }

    std::cout << "KEYPATHおねがいします：";
    std::string json_key_path;
    if (!std::getline(std::cin, json_key_path)) {
        json_key_path.clear();
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-d" && i + 1 < argc) {
            target_directory = argv[++i];
        } else {
            std::cerr << "Unknown argument " << arg << '\n';
        }
    }

    std::cout << "Target directory: "
              << std::filesystem::absolute(target_directory).lexically_normal()
              << '\n';

    jason gServAcc;
    if (!json_key_path.empty()) {
        gServAcc = LoadGoogleServiceAccount(json_key_path);
    } else if (!VISION_KEY_PATH.empty()) {
        gServAcc = LoadGoogleServiceAccount(VISION_KEY_PATH);
    } else {
        std::cerr << "[ERR] No Vision service account key path provided\n";
        return 1;
    }
    std::string sToken = CreateSToken(gServAcc);
    std::string accToken = GetAccessToken(sToken);
    ACCESS_TOKEN = accToken;
    if (ACCESS_TOKEN.empty()) {
        std::cerr << "[ERR] Failed to obtain access token\n";
        return 1;
    }

    const std::vector<std::string> validExtensions = {".png", ".jpg", ".jpeg"};
    for (const auto &file :
         std::filesystem::directory_iterator(target_directory)) {
        if (!file.is_regular_file())
            continue;
        const auto &filePath = file.path();
        std::string targetExtension = filePath.extension().string();
        std::transform(targetExtension.begin(), targetExtension.end(),
                       targetExtension.begin(), ::tolower);
        if (std::find(validExtensions.begin(), validExtensions.end(),
                      targetExtension) == validExtensions.end()) {
            continue;
        }
        std::vector<unsigned char> imageBytes{};
        if (!ReadToBytes(filePath, imageBytes)) {
            std::cerr << "[ERR] Failed to read image: " << filePath << '\n';
            continue;
        }
        std::string detectedText = DetectText(imageBytes);
        if (detectedText.empty()) {
            std::cerr << "[WARN] No text detected in " << filePath << '\n';
            continue;
        }
        std::cout << "[ Image: " << filePath << " ] Text: " << detectedText
                  << '\n';
    }

    return 0;
}
