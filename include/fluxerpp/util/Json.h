#pragma once
// fluxerpp/util/Json.h
//
// Thin alias so the rest of the SDK depends on fluxerpp::util::Json instead
// of nlohmann::json directly — keeps the JSON library swappable later
// without touching every model header.

#include <nlohmann/json.hpp>

namespace fluxerpp::util {
using Json = nlohmann::json;
}