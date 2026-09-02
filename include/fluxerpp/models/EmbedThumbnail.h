#pragma once
// fluxerpp/models/EmbedThumbnail.h

#include <string>
#include <optional>
#include "fluxerpp/util/Json.h"

namespace fluxerpp {
namespace models {

class EmbedThumbnail {
public:
    std::optional<std::string> url;
    std::optional<std::string> proxy_url;
    std::optional<int> height;
    std::optional<int> width;

    EmbedThumbnail() = default;

    static EmbedThumbnail from_data(const util::Json& data);
    util::Json to_dict() const;
};

} // namespace models
} // namespace fluxerpp
