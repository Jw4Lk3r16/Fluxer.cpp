// fluxerpp/models/EmbedField.cpp

#include "fluxerpp/models/EmbedField.h"

namespace fluxerpp {
namespace models {

using util::Json;

EmbedField EmbedField::from_data(const Json& data) {
    EmbedField f;

    if (data.contains("name") && !data["name"].is_null())
        f.name = data["name"].get<std::string>();

    if (data.contains("value") && !data["value"].is_null())
        f.value = data["value"].get<std::string>();

    if (data.contains("inline") && !data["inline"].is_null())
        f.inline_field = data["inline"].get<bool>();

    return f;
}

Json EmbedField::to_dict() const {
    Json j = Json::object();
    j["name"] = name;
    j["value"] = value;
    j["inline"] = inline_field;
    return j;
}

} // namespace models
} // namespace fluxerpp
