#pragma once
#include <string>

namespace fluxerpp::models {

class User { public: std::uint64_t id{}; };
class Embed { public: std::string to_dict() const { return "{}"; } };
class Reaction { public: std::string emoji; };
class Attachment {};
class PartialEmoji { public: std::string to_string() const { return ""; } };
class File { public: std::string to_dict() const { return "{}"; } };
class Channel { public: std::uint64_t id{}; };
class Guild { public: std::uint64_t id{}; };

}
