// File.cpp
#include "fluxerpp/models/File.h"

namespace fluxerpp {
namespace models {

using util::Json;

Json File::to_dict() const {
    Json j = Json::object();
    j["filename"] = filename;
    if (content_type) j["content_type"] = *content_type;
    j["size"] = data.size();
    return j;
}

} // namespace models
} // namespace fluxerpp