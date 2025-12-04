#pragma once

#include "unified_response.hpp"

namespace setman::ai
{
struct google_response;
struct deepl_response;
struct openrouter_response;
} // namespace setman::ai

namespace setman::translation_service
{

unified_response to_unified(const ai::google_response &resp);
unified_response to_unified(const ai::deepl_response &resp);
unified_response to_unified(const ai::openrouter_response &resp);

} // namespace setman::translation_service
