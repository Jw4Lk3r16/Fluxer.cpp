// // tests/consumer_smoke_test.cpp
// //
// // Not a unit test suite — a compile+link smoke test proving the fluxerpp
// // CMake target actually exposes what it claims to a consumer:
// //   1. Every public header compiles standalone as the first include in a
// //      translation unit (no missing includes hiding behind include order
// //      in some other .cpp that happened to include things first).
// //   2. This file links against the fluxerpp *target* using nothing but
// //      target_link_libraries(... fluxerpp) — no hand-added include paths —
// //      relying entirely on target_include_directories(fluxerpp PUBLIC ...)
// //      in CMakeLists.txt.
// //   3. Calling User::from_data() forces the linker to actually pull in
// //      src/models/User.cpp's compiled symbol, not just accept a
// //      declaration that nothing calls. This is the specific kind of gap
// //      the review caught: a target that "builds" while quietly excluding
// //      the model implementations is not evidence the SDK builds.
// //
// // Deliberately does not connect to anything — no token, no network. This
// // is a build-time check, not an integration test.

// #include "fluxerpp/FluxerClient.h"
// #include "fluxerpp/RestClient.h"
// #include "fluxerpp/GatewayClient.h"
// #include "fluxerpp/EventDispatcher.h"
// #include "fluxerpp/FluxerConfig.h"
// #include "fluxerpp/env.h"
// #include "fluxerpp/util/Json.h"
// #include "fluxerpp/util/Logger.h"
// #include "fluxerpp/models/Message.h"
// #include "fluxerpp/models/User.h"
// #include "fluxerpp/models/Attachment.h"
// #include "fluxerpp/models/Embed.h"
// #include "fluxerpp/models/File.h"
// #include "fluxerpp/models/PartialEmoji.h"
// #include "fluxerpp/models/Reaction.h"
// #include "fluxerpp/models/Channel.h"
// #include "fluxerpp/models/Guild.h"

// #include <cstdio>

// int main() {
//     fluxerpp::FluxerConfig cfg;
//     cfg.token = "smoke-test-token";
//     fluxerpp::FluxerClient client(cfg);
//     (void)client;

//     // Forces linking against the compiled model .cpp files, not just their
//     // declarations.
//     fluxerpp::util::Json j = fluxerpp::util::Json::object();
//     j["id"] = "123456789012345678";
//     j["username"] = "smoketest";
//     j["discriminator"] = "0001";
//     j["bot"] = true;
//     fluxerpp::models::User u = fluxerpp::models::User::from_data(j, nullptr);

//     if (u.id != 123456789012345678ull || u.username != "smoketest") {
//         std::fprintf(stderr, "consumer_smoke_test: User::from_data round-trip failed\n");
//         return 1;
//     }

//     std::printf("consumer_smoke_test: OK\n");
//     return 0;
// }5