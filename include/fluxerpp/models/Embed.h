#pragma once
// fluxerpp/models/Embed.h
#include <string>
#include <optional>
#include <vector>
#include "fluxerpp/util/Json.h" // NOTE: previously used util::Json without including this — flagged in review item 1.
#include "fluxerpp/models/EmbedField.h"
#include "fluxerpp/models/EmbedVideo.h"
#include "fluxerpp/models/EmbedAuthor.h"
#include "fluxerpp/models/EmbedFooter.h"
#include "fluxerpp/models/EmbedThumbnail.h"
#include "fluxerpp/models/EmbedImage.h"

namespace fluxerpp {
namespace models {

class Embed {
public:
    std::optional<std::string> title;
    std::optional<std::string> description;
    std::optional<std::string> url;
    std::optional<int> color;
    std::optional<std::string> timestamp;
    std::vector<EmbedField> fields;
    std::optional<EmbedVideo> video;
    std::optional<EmbedAuthor> author;
    std::optional<EmbedFooter> footer;
    std::optional<EmbedThumbnail> thumbnail;
    std::optional<EmbedImage> image;

    Embed() = default;

    static Embed from_data(const util::Json& data);
    util::Json to_dict() const;
};

} // namespace models
} // namespace fluxerpp