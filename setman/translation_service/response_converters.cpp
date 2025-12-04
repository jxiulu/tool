#include "response_converters.hpp"
#include "ai_endpoints/deepl.hpp"
#include "ai_endpoints/google.hpp"
#include "ai_endpoints/openrouter.hpp"

namespace setman::translation_service
{

unified_response to_unified(const ai::google_response &resp)
{
    return {.valid = resp.valid,
            .error = resp.error,
            .content = resp.content,
            .raw_json = resp.raw_json};
}

unified_response to_unified(const ai::deepl_response &resp)
{
    return {.valid = resp.valid,
            .error = resp.error,
            .content = resp.content,
            .raw_json = resp.raw_json};
}

unified_response to_unified(const ai::openrouter_response &resp)
{
    return {.valid = resp.valid,
            .error = resp.error,
            .content = resp.content,
            .raw_json = resp.raw_json};
}

} // namespace setman::translation_service
