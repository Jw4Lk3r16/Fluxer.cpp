#pragma once
// fluxerpp/builders/EmbedBuilder.h

#include <string>
#include <vector>
#include <optional>

#include "fluxerpp/models/Embed.h"
#include "fluxerpp/models/EmbedAuthor.h"
#include "fluxerpp/models/EmbedFooter.h"
#include "fluxerpp/models/EmbedField.h"
#include "fluxerpp/models/EmbedThumbnail.h"
#include "fluxerpp/models/EmbedImage.h"

namespace fluxerpp {
namespace builders {

class EmbedBuilder {
private:
    models::Embed embed;

public:
    EmbedBuilder() = default;

    // Title
    EmbedBuilder& set_title(const std::string& title) {
        embed.title = title;
        return *this;
    }

    // Description
    EmbedBuilder& set_description(const std::string& desc) {
        embed.description = desc;
        return *this;
    }

    // URL
    EmbedBuilder& set_url(const std::string& url) {
        embed.url = url;
        return *this;
    }

    // Color
    EmbedBuilder& set_color(int color) {
        embed.color = color;
        return *this;
    }

    // Author
    EmbedBuilder& set_author(const std::string& name,
                             const std::optional<std::string>& icon_url = std::nullopt,
                             const std::optional<std::string>& url = std::nullopt) {
        models::EmbedAuthor a;
        a.name = name;
        a.icon_url = icon_url;
        a.url = url;
        embed.author = a;
        return *this;
    }

    // Footer
    EmbedBuilder& set_footer(const std::string& text,
                             const std::optional<std::string>& icon_url = std::nullopt) {
        models::EmbedFooter f;
        f.text = text;
        f.icon_url = icon_url;
        embed.footer = f;
        return *this;
    }

    // Thumbnail
    EmbedBuilder& set_thumbnail(const std::string& url) {
        models::EmbedThumbnail t;
        t.url = url;
        embed.thumbnail = t;
        return *this;
    }

    // Image
    EmbedBuilder& set_image(const std::string& url) {
        models::EmbedImage img;
        img.url = url;
        embed.image = img;
        return *this;
    }

    // Fields
    EmbedBuilder& add_field(const std::string& name,
                            const std::string& value,
                            bool inline_field = false) {
        models::EmbedField f;
        f.name = name;
        f.value = value;
        f.inline_field = inline_field;

        embed.fields.push_back(f);
        return *this;
    }

    // Build
    models::Embed build() const {
        return embed;
    }
};

} // namespace builders
} // namespace fluxerpp
