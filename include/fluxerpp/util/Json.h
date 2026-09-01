#pragma once
// fluxerpp/util/Json.h
//
// Thin alias so the rest of the SDK depends on fluxerpp::util::Json instead
// of nlohmann::json directly — keeps the JSON library swappable later
// without touching every model header.

#include <nlohmann/json.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace fluxerpp::util {
using Json = nlohmann::json;

// Snowflake IDs arrive as JSON strings (they exceed safe JS integer range).
// Shared by every model's from_data() so a malformed ID names which field
// failed instead of surfacing a bare std::stoull exception with no context.
inline std::uint64_t parse_snowflake(const Json& data, const char* field) {
    try {
        return std::stoull(data.at(field).get<std::string>());
    } catch (const std::exception& ex) {
        throw std::runtime_error(std::string("invalid '") + field + "' field: " + ex.what());
    }
}

} // namespace fluxerpp::util