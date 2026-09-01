#pragma once
// fluxerpp/models/Reaction.h
#include "fluxerpp/models/PartialEmoji.h"
#include "fluxerpp/util/Json.h"

namespace fluxerpp {

class RestClient;

namespace models {

// Forward-declared, not included: Message.h includes Reaction.h, so
// including Message.h here would be circular. Only a pointer is needed.
class Message;

class Reaction {
public:
    PartialEmoji emoji;
    int count{0};
    bool me{false};

    Reaction() = default;

    static Reaction from_data(const util::Json& data, RestClient* rest, Message* message);

    void bind_message(Message* message) { message_ = message; }
    void bind_rest(RestClient* rest) { rest_ = rest; }

private:
    Message* message_{nullptr};
    RestClient* rest_{nullptr};
};

} // namespace models
} // namespace fluxerpp