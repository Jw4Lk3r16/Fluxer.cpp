#pragma once
// fluxerpp/builders/EmbedFooterBuilder.h

#include "fluxerpp/models/EmbedFooter.h"

namespace fluxerpp {
namespace builders {

class EmbedFooterBuilder {
private:
    models::EmbedFooter f;

public:
    EmbedFooterBuilder& set_text(const std::string& t) {
        f.text = t;
        return *this;
    }

    EmbedFooterBuilder& set_icon(const std::string& icon) {
        f.icon_url = icon;
        return *this;
    }

    models::EmbedFooter build() const {
        return f;
    }
};

} // namespace builders
} // namespace fluxerpp
