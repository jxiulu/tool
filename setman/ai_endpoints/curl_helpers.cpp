#include "curl_helpers.hpp"

namespace setman::ai::curl_helpers
{

size_t write_callback(void *contents, size_t size, size_t nmemb,
                      std::string *userp)
{
    size_t total_size = size * nmemb;
    userp->append(static_cast<char *>(contents), total_size);
    return total_size;
}

http_response post_json(CURL *curl, const std::string &url, const json &payload,
                        struct curl_slist *headers)
{
    http_response result;
    std::string payload_str = payload.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload_str.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        result.error = curl_easy_strerror(res);
        result.http_code = 0;
        return result;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.http_code);
    return result;
}

struct curl_slist *add_json_header(struct curl_slist *headers)
{
    return curl_slist_append(headers, "Content-Type: application/json");
}

struct curl_slist *add_bearer_auth(struct curl_slist *headers,
                                   const std::string &token)
{
    std::string auth_header = "Authorization: Bearer " + token;
    return curl_slist_append(headers, auth_header.c_str());
}

} // namespace setman::ai::curl_helpers
