#pragma once
// fluxerpp/models/EmbedVideo.h
//
// Represents the <video> block inside an embed.
// Fluxer/Discord embed videos support:
// - url (string)
// - proxy_url (string)
// - height (int)
// - width (int)

#include <string>
#include <optional>
#include "fluxerpp/util/Json.h"

namespace fluxerpp {
namespace models {

class EmbedVideo {
public:
    std::optional<std::string> url;
    std::optional<std::string> proxy_url;
    std::optional<int> height;
    std::optional<int> width;

    EmbedVideo() = default;

    static EmbedVideo from_data(const util::Json& data);
    util::Json to_dict() const;
};

} // namespace models
} // namespace fluxerpp
