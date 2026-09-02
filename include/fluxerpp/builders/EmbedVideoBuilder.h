#pragma once
// fluxerpp/builders/EmbedVideoBuilder.h

#include <string>
#include <optional>
#include "fluxerpp/models/EmbedVideo.h"

namespace fluxerpp {
namespace builders {

class EmbedVideoBuilder {
private:
    models::EmbedVideo video;

public:
    EmbedVideoBuilder() = default;

    EmbedVideoBuilder& set_url(const std::string& url) {
        video.url = url;
        return *this;
    }

    EmbedVideoBuilder& set_proxy_url(const std::string& proxy) {
        video.proxy_url = proxy;
        return *this;
    }

    EmbedVideoBuilder& set_dimensions(int width, int height) {
        video.width = width;
        video.height = height;
        return *this;
    }

    models::EmbedVideo build() const {
        return video;
    }
};

} // namespace builders
} // namespace fluxerpp
