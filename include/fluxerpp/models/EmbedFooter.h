#pragma once
// fluxerpp/models/EmbedFooter.h

#include <string>
#include <optional>
#include "fluxerpp/util/Json.h"

namespace fluxerpp {
namespace models {

class EmbedFooter {
public:
    std::optional<std::string> text;
    std::optional<std::string> icon_url;
    std::optional<std::string> proxy_icon_url;

    EmbedFooter() = default;

    static EmbedFooter from_data(const util::Json& data);
    util::Json to_dict() const;
};

} // namespace models
} // namespace fluxerpp
