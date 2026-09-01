// Reaction.cpp
#include "fluxerpp/models/Reaction.h"

namespace fluxerpp {
namespace models {

using util::Json;
using util::parse_snowflake;

Reaction Reaction::from_data(const Json& data, RestClient* rest, Message* message) {
    Reaction r;
    r.count = data.value("count", 0);
    r.me = data.value("me", false);

    if (data.contains("emoji")) {
        const Json& e = data["emoji"];
        PartialEmoji pe;
        pe.name = e.value("name", "");
        pe.animated = e.value("animated", false);
        if (e.contains("id") && !e["id"].is_null()) {
            pe.id = parse_snowflake(e, "id");
        }
        r.emoji = pe;
    }

    r.bind_rest(rest);
    r.bind_message(message);
    return r;
}

} // namespace models
} // namespace fluxerpp