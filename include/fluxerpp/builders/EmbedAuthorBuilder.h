#pragma once
#include "fluxerpp/models/EmbedAuthor.h"

namespace fluxerpp {
namespace builders {

class EmbedAuthorBuilder {
private:
    models::EmbedAuthor a;

public:
    EmbedAuthorBuilder& set_name(const std::string& n) {
        a.name = n; return *this;
    }
    EmbedAuthorBuilder& set_url(const std::string& u) {
        a.url = u; return *this;
    }
    EmbedAuthorBuilder& set_icon(const std::string& icon) {
        a.icon_url = icon; return *this;
    }
    models::EmbedAuthor build() const { return a; }
};

} // namespace builders
} // namespace fluxerpp
