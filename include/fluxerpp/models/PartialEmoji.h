#pragma once
// fluxerpp/models/PartialEmoji.h
#include <string>
#include <optional>
#include <cstdint>

namespace fluxerpp {
namespace models {

// A "partial" emoji reference as used in reactions: either a unicode emoji
// (id absent, name holds the character itself) or a custom guild emoji
// (id present, name is its registered name).
class PartialEmoji {
public:
    std::optional<std::uint64_t> id;
    std::string name;
    bool animated{false};

    PartialEmoji() = default;
    explicit PartialEmoji(std::string name_) : name(std::move(name_)) {}

    // Custom emoji -> "name:id" (the form the REST reaction routes expect
    // in the URL path); unicode emoji -> the unicode character itself.
    std::string to_string() const {
        if (id.has_value()) return name + ":" + std::to_string(*id);
        return name;
    }

    bool operator==(const std::string& other) const { return to_string() == other; }
    bool operator!=(const std::string& other) const { return !(*this == other); }
};

} // namespace models
} // namespace fluxerpp