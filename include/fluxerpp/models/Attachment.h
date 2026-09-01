#pragma once
// fluxerpp/models/Attachment.h
#include <cstdint>
#include <string>
#include <optional>
#include "fluxerpp/util/Json.h"

namespace fluxerpp {
namespace models {

class Attachment {
public:
    std::uint64_t id{};
    std::string filename;
    std::optional<std::string> content_type;
    std::uint64_t size{};
    std::string url;
    std::string proxy_url;
    std::optional<int> width;
    std::optional<int> height;

    Attachment() = default;

    static Attachment from_data(const util::Json& data);
};

} // namespace models
} // namespace fluxerpp