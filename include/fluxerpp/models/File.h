#pragma once
// fluxerpp/models/File.h
#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include "fluxerpp/util/Json.h" // NOTE: previously used util::Json without including this — flagged in review item 1.

namespace fluxerpp {
namespace models {

// Represents a local file to attach to a message.
//
// LIMITATION: to_dict() currently returns a small JSON description
// (filename/content_type/size), not the file's bytes — Message::send()
// embeds this directly into the JSON request body via build_file_list().
// That means attachments will not actually upload correctly until
// RestClient can build a multipart/form-data body; JSON can't carry
// arbitrary binary payloads. Tracked as a follow-up, not fixed here —
// this pass is only about making the model API compile and have a
// coherent shape, not about multipart REST support.
class File {
public:
    std::string filename;
    std::vector<std::uint8_t> data;
    std::optional<std::string> content_type;

    File() = default;
    File(std::string filename_, std::vector<std::uint8_t> data_)
        : filename(std::move(filename_)), data(std::move(data_)) {}

    util::Json to_dict() const;
};

} // namespace models
} // namespace fluxerpp