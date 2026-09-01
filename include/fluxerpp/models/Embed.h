#pragma once
// fluxerpp/models/Embed.h
#include <string>
#include <optional>
#include "fluxerpp/util/Json.h" // NOTE: previously used util::Json without including this — flagged in review item 1.

namespace fluxerpp {
namespace models {

class Embed {
public:
    std::optional<std::string> title;
    std::optional<std::string> description;
    std::optional<std::string> url;
    std::optional<int> color;
    std::optional<std::string> timestamp;

    Embed() = default;

    static Embed from_data(const util::Json& data);
    util::Json to_dict() const;
};

} // namespace models
} // namespace fluxerpp