#pragma once
// fluxerpp/models/EmbedField.h
//
// Represents a single field inside an embed.
// Fluxer/Discord embed fields support:
// - name (string)
// - value (string)
// - inline (bool)

#include <string>
#include <optional>
#include "fluxerpp/util/Json.h"

namespace fluxerpp {
namespace models {

class EmbedField {
public:
    std::string name;
    std::string value;
    bool inline_field{false};

    EmbedField() = default;

    static EmbedField from_data(const util::Json& data);
    util::Json to_dict() const;
};

} // namespace models
} // namespace fluxerpp
