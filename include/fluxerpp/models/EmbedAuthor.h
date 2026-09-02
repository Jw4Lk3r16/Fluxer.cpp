#pragma once
// fluxerpp/models/EmbedAuthor.h

#include <string>
#include <optional>
#include "fluxerpp/util/Json.h"

namespace fluxerpp {
namespace models {

class EmbedAuthor {
public:
    std::optional<std::string> name;
    std::optional<std::string> url;
    std::optional<std::string> icon_url;
    std::optional<std::string> proxy_icon_url;

    EmbedAuthor() = default;

    static EmbedAuthor from_data(const util::Json& data);
    util::Json to_dict() const;
};

} // namespace models
} // namespace fluxerpp
