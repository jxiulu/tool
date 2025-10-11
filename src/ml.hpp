#include "framework.hpp"
#include <cstdint>

namespace ml {
inline std::string EncodeBase64(const unsigned char *Data, size_t Length) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (Length == 0) {
        return {};
    }

    std::string output;
    output.reserve(((Length + 2) / 3) * 4);

    size_t i = 0;
    const size_t fullchunks = Length / 3 * 3;
    for (; i < fullchunks; i += 3) {
        const uint32_t triple = (static_cast<uint32_t>(Data[i]) << 16) |
                                (static_cast<uint32_t>(Data[i + 1]) << 8) |
                                static_cast<uint32_t>(Data[i + 2]);
        output.push_back(alphabet[(triple >> 18) & 0x3F]);
        output.push_back(alphabet[(triple >> 12) & 0x3F]);
        output.push_back(alphabet[(triple >> 6) & 0x3F]);
        output.push_back(alphabet[triple & 0x3F]);
    }

    const size_t remainder = Length - i;
    if (remainder == 1) {
        const uint32_t triple = static_cast<uint32_t>(Data[i]) << 16;
        output.push_back(alphabet[(triple >> 18) & 0x3F]);
        output.push_back(alphabet[(triple >> 12) & 0x3F]);
        output.push_back('=');
        output.push_back('=');
    } else if (remainder == 2) {
        const uint32_t triple = (static_cast<uint32_t>(Data[i]) << 16) |
                                (static_cast<uint32_t>(Data[i + 1]) << 8);
        output.push_back(alphabet[(triple >> 18) & 0x3F]);
        output.push_back(alphabet[(triple >> 12) & 0x3F]);
        output.push_back(alphabet[(triple >> 6) & 0x3F]);
        output.push_back('=');
    }

    return output;
}

inline std::string EncodeBase64(const std::vector<unsigned char> &Buf) {
    if (Buf.empty()) {
        return {};
    }
    return EncodeBase64(Buf.data(), Buf.size());
}
} // namespace ml

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
    size_t total = size * nmemb;
    std::string *out = static_cast<std::string *>(userp);
    out->append(static_cast<char *>(contents), total);
    return total;
}

class MLClient {
  private:
    fs::path VisionKeyPath;
    std::string AccessToken;
    std::string ProjectId;

    jason LoadGoogleKey(const std::string &JSONKeyPath) {
        std::ifstream ifs(JSONKeyPath);
        if (!ifs) {
            throw std::runtime_error("[RUNERR] Unable to open key JSON");
        }
        jason key;
        ifs >> key;
        return key;
    }

    std::string CreateSignedToken(const jason &Key) {
        if (!Key.contains("type") || !Key["type"].is_string() ||
            Key["type"].get<std::string>() != "service_account") {
            throw std::runtime_error(
                "[RUNERR] JSON key is not a service account credential");
        }
        if (!Key.contains("private_key") || !Key["private_key"].is_string()) {
            throw std::runtime_error(
                "[RUNERR] JSON key missing private_key string");
        }
        if (!Key.contains("client_email") || !Key["client_email"].is_string()) {
            throw std::runtime_error(
                "[RUNERR] JSON key missing client_email string");
        }

        auto tim = std::chrono::system_clock::now();

        std::string privatekey = Key["private_key"].get<std::string>();
        std::string clientemail = Key["client_email"].get<std::string>();

        auto unsignedtoken =
            jwt::create()
                .set_issuer(clientemail)
                .set_audience("https://oauth2.googleapis.com/token")
                .set_issued_at(tim)
                .set_expires_at(tim + std::chrono::minutes(55))
                .set_payload_claim(
                    "scope",
                    jwt::claim(std::string(
                        "https://www.googleapis.com/auth/cloud-platform")));

        auto signedtoken = unsignedtoken.sign(jwt::algorithm::rs256(
            "", privatekey, "", "")); // Passwords strings empty idc
        return signedtoken;
    }

    std::string GetAccessToken(const std::string &SignedToken) {
        CURL *curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("[RUNERR] [CURL] Failed to easy init");
            return "";
        }

        std::string post_fields =
            "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-"
            "type%3Ajwt-bearer&assertion=" +
            SignedToken;
        std::string response_data = {};
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(
            headers, "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL,
                         "https://oauth2.googleapis.com/token");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        CURLcode rescode = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (rescode != CURLE_OK) {
            throw std::runtime_error("[RUNERR] [CURL] " +
                                     (std::string)curl_easy_strerror(rescode));
            return "";
        }

        auto jsonaccesstoken = jason::parse(response_data);
        if (jsonaccesstoken.contains("access_token") &&
            jsonaccesstoken["access_token"].is_string()) {
            return jsonaccesstoken["access_token"].get<std::string>();
        }

        std::string error_message;
        if (jsonaccesstoken.contains("error_description") &&
            jsonaccesstoken["error_description"].is_string()) {
            error_message =
                jsonaccesstoken["error_description"].get<std::string>();
        } else if (jsonaccesstoken.contains("error") &&
                   jsonaccesstoken["error"].is_string()) {
            error_message = jsonaccesstoken["error"].get<std::string>();
        } else {
            error_message = response_data;
        }

        throw std::runtime_error(
            "[RUNERR] OAuth access token missing: " + error_message);
    }

    bool SetHeaders() {
        if (Headers) {
            curl_slist_free_all(Headers);
            Headers = nullptr;
        }

        Headers = curl_slist_append(Headers, "Content-Type: application/json");
        if (!Headers) {
            return false;
        }

        std::string auth_header = "Authorization: Bearer " + AccessToken;
        Headers = curl_slist_append(Headers, auth_header.c_str());
        if (!Headers) {
            return false;
        }

        if (!ProjectId.empty()) {
            std::string project_header = "x-goog-user-project: " + ProjectId;
            Headers = curl_slist_append(Headers, project_header.c_str());
            if (!Headers) {
                return false;
            }
        }

        if (curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, Headers) != CURLE_OK) {
            return false;
        }

        return true;
    }

  public:
    CURL *Curl;
    std::string Endpoint;
    struct curl_slist *Headers = nullptr;
    std::string Response;
    bool success = false;

    MLClient(const std::string &PathToJSONKey,
             std::string ProjectIdOverride = {})
        : Curl(curl_easy_init()) {
        VisionKeyPath = PathToJSONKey;
        jason key = LoadGoogleKey(PathToJSONKey);
        if (!ProjectIdOverride.empty()) {
            ProjectId = std::move(ProjectIdOverride);
        } else if (key.contains("project_id") &&
                   key["project_id"].is_string()) {
            ProjectId = key["project_id"].get<std::string>();
        } else if (const char *project_env = std::getenv("GOOGLE_PROJECT_ID")) {
            ProjectId = project_env;
        }

        std::string signedtoken = CreateSignedToken(key);
        AccessToken = GetAccessToken(signedtoken);

        if (!Curl) {
            std::cerr << "curl_easy_init() failed\n";
            success = false;
            return;
        }

        if (AccessToken.empty()) {
            std::cerr << "[ERR] Failed to obtain access token\n";
            success = false;
            return;
        }

        Endpoint = "https://vision.googleapis.com/v1/images:annotate";

        if (!SetHeaders()) {
            std::cerr << "[ERR] Failed to construct header list\n";
            success = false;
            return;
        }

        if (curl_easy_setopt(Curl, CURLOPT_URL, Endpoint.c_str()) != CURLE_OK ||
            curl_easy_setopt(Curl, CURLOPT_HTTP_VERSION,
                             CURL_HTTP_VERSION_1_1) != CURLE_OK ||
            curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, WriteCallback) !=
                CURLE_OK ||
            curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Response) != CURLE_OK) {
            std::cerr << "[ERR] Failed to configure CURL session\n";
            success = false;
            return;
        }

        success = true;
    }

    ~MLClient() noexcept {
        if (Headers)
            curl_slist_free_all(Headers);
        if (Curl)
            curl_easy_cleanup(Curl);
    }

    std::string DetectText(const std::vector<unsigned char> &In,
                           const std::string &OverrideProjectId = {}) {
        std::string encodedcontent = ml::EncodeBase64(In);

        jason payload = {
            {"requests",
             jason::array(
                 {{{"image", {{"content", encodedcontent}}},
                   {"features",
                    jason::array({{{"type", "DOCUMENT_TEXT_DETECTION"},
                                   {"maxResults", 1},
                                   {"model", "builtin/latest"}}})}}})}};
        std::string payloadbuf = payload.dump();
        Response.clear();

        if (!OverrideProjectId.empty() && OverrideProjectId != ProjectId) {
            ProjectId = OverrideProjectId;
            if (!SetHeaders()) {
                std::cerr << "[ERR] Failed to update headers for project id\n";
                return "";
            }
        }

        if (curl_easy_setopt(Curl, CURLOPT_POSTFIELDS, payloadbuf.c_str()) !=
                CURLE_OK ||
            curl_easy_setopt(Curl, CURLOPT_POSTFIELDSIZE_LARGE,
                             static_cast<curl_off_t>(payloadbuf.size())) !=
                CURLE_OK) {
            std::cerr << "[ERR] Failed to set request payload\n";
            return "";
        }

        CURLcode code = curl_easy_perform(Curl);
        if (code != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: "
                      << curl_easy_strerror(code) << '\n';
            return "";
        }

        try {
            auto response_json = jason::parse(Response);
            if (!response_json.contains("responses") ||
                response_json["responses"].empty()) {
                return "";
            }

            const auto &firstresponse = response_json["responses"][0];
            if (!firstresponse.contains("fullTextAnnotations") ||
                !firstresponse["fullTextAnnotations"].contains("text")) {
                return "";
            }

            return firstresponse["fullTextAnnotations"]["text"]
                .get<std::string>();
        } catch (...) {
            std::cerr
                << "DetectText() error. Unable to parse response.\n[RAW]\n"
                << Response << "\n[END RAW]\n";
            return "";
        }
    }
};
