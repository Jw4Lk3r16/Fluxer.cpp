# Fluxer++
**Fluxer++** is a native **C++ SDK** for building fast, reliable Fluxer bots. It provides a modern REST client, real‑time gateway handling, type‑safe models, and an event‑driven architecture designed for high‑performance applications on the Fluxer platform.

Fluxer++ is designed for developers who want full control, zero overhead, and maximum performance when interacting with the Fluxer API. The SDK is lightweight, modular, and built with modern C++20 standards.

---
## Quick links

### ReadMe
[Project Structure](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#-project-structure "File Tree/Hierarchy")

[Installation](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#-installation "Use Fluxer++")

[Getting Started](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#-getting-started "New?")

[Rest API Usage](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#-rest-api-usage "Not Empty")

[Gateway & Events](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#-gateway--events "Gateway & Events")

[Models](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#-models "Models")

[Packaging](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#future-packaging "Packaging")

[Logging](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#-logging "Logging")

[Problems](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#%EF%B8%8F-error-handling "Uh Oh")

[Contributing](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#%EF%B8%8F-contributing "Helper?")

[Licence](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#-license "Boring")


---

## ✨ Features  
- **High‑performance C++ core** — optimized for speed and low memory overhead  
- **Event‑driven gateway** — register callbacks for ready, message events, reactions, and more  
- **Modern REST client** — simple JSON‑based request/response handling  
- **Type‑safe models** — Message, User, Channel, Guild, Role, Member, etc.  
- **Modular architecture** — clean separation of gateway, REST, events, and models  
- **Easy integration** — drop‑in headers and CMake support  
- **UCRT64‑compatible** — built with MSYS2 UCRT64 toolchain  
- **Examples included** — ping bot, echo bot, and minimal gateway usage  

---

## 📁 Project Structure  
```
fluxerpp/
│
├── include/
│   ├── FluxerClient.h
│   ├── RestClient.h
│   ├── GatewayClient.h
│   ├── EventDispatcher.h
│   └── fluxerpp/
│       ├── models/
│       │   ├─ Message.h
│       │   ├─ User.h
│       │   ├─ Embed.h
│       │   ├─ Reaction.h
│       │   ├─ Attachment.h
│       │   ├─ PartialEmoji.h
│       │   ├─ File.h
│       │   ├─ Channel.h
│       │   ├─ Guild.h
│       │   ├─ Profile.h
│       │   ├─ Role.h
│       │   ├─ Member.h
│       │   ├─ Voice.h
│       │   ├─ Webhook.h
│       │   └─ __init__.h
│       └── util/
│           ├── Json.h
│           └── Logger.h
│
├── src/
│   ├── FluxerClient.cpp
│   ├── RestClient.cpp
│   ├── GatewayClient.cpp
│   ├── EventDispatcher.cpp
│   ├── models/
│   │   ├─ Message.cpp
│   │   ├─ User.cpp
│   │   ├─ Embed.cpp
│   │   ├─ Reaction.cpp
│   │   ├─ Attachment.cpp
│   │   ├─ PartialEmoji.cpp
│   │   ├─ File.cpp
│   │   ├─ Channel.cpp
│   │   ├─ Guild.cpp
│   │   ├─ Profile.cpp
│   │   ├─ Role.cpp
│   │   ├─ Member.cpp
│   │   ├─ Voice.cpp
│   │   ├─ Webhook.cpp
│   │   └─ __init__.cpp
│
├── examples/
│   ├── ping_bot.cpp
│   └── echo_bot.cpp
│
└── CMakeLists.txt
```

---

## 🔧 Installation  
Fluxer++ is currently built using **UCRT64** (MSYS2).  
Precompiled binaries will be available soon.

### Clone the repository  
```
git clone https://github.com/Jw4Lk3r16/Fluxer.cpp
```

### Build with CMake  
```
mkdir build
cd build
cmake ..
cmake --build .
```

---

## 🚀 Getting Started  
### Minimal Bot Example  
```cpp
#include "fluxerpp/FluxerClient.h"

int main() {
    fluxerpp::FluxerConfig cfg;
    cfg.token = "YOUR_TOKEN";

    fluxerpp::FluxerClient client(cfg);

    client.gate.on_ready([&]() {
        std::cout << "Bot is online!\n";
    });

    client.login();
}
```

---

## 🌐 REST API Usage  
Fluxer++ exposes a simple JSON‑based REST interface:

```cpp
client.api().send_message(channel_id, {
    {"content", "Hello from Fluxer++!"}
});
```

Supported operations include:

- Sending messages  
- Editing messages  
- Deleting messages  
- Managing channels  
- Managing guilds  
- Webhooks  
- File uploads  

---

## 🔌 Gateway & Events  
Fluxer++ uses an event dispatcher to handle gateway events:

### Registering events  
```cpp
client.gate.on_message_create([&](const fluxerpp::Message& msg) {
    std::cout << "Received: " << msg.content << "\n";
});
```

Available events include:

- `on_ready`  
- `on_message_create`  
- `on_message_update`  
- `on_message_delete`  
- `on_reaction_add`  
- `on_reaction_remove`  
- `on_member_join`  
- `on_member_leave`  
- `on_typing_start`  
- and more…

---

## 🧱 Models  
Fluxer++ provides strongly‑typed models for all major Fluxer objects:

- **Message** — content, embeds, attachments  
- **User** — username, avatar, flags  
- **Channel** — type, permissions  
- **Guild** — roles, members, settings  
- **Role** — permissions, color  
- **Member** — user + guild metadata  
- **Embed** — title, description, fields  
- **Webhook** — execute, modify, delete  
- **File** — uploadable binary data  

Each model includes:

- JSON parsing  
- JSON serialization  
- Utility helpers  
- Safe accessors  

---
## Future packaging

| Packaging Method | Best For |
| --- | --- |
| Precompiled DLL + headers | Most users |
| Static library | Performance bots |
| CMake package | Serious C++ devs |
| vcpkg port | Windows/Linux/Mac |
| Conan package | Enterprise users |
| MSYS2 package | UCRT64 users |
| Single‑header | Beginners |
| NuGet | Visual Studio users |

---

## 📜 Logging  
Fluxer++ includes a lightweight logger:

```cpp
fluxerpp::Logger::info("Gateway connected");
fluxerpp::Logger::error("Failed to parse JSON");
```

Log levels:

- Info  
- Warning  
- Error  
- Debug  

---

## ⚠️ Error Handling  
REST and gateway operations return structured error objects:

```cpp
auto res = client.api().send_message(...);

if (!res.ok()) {
    std::cerr << res.error().message << "\n";
}
```
---
## ⚠️ Bottles Necks

**This SDK only supports windows** at the moment but in the near future **we __will__ migrate to a cross-platform build**.

It is not fully developed yet expect it to be nearing completion in a week or two.

---

## 🛠️ Contributing  
Contributions are welcome!  
You can help by:

- Improving documentation  
- Adding more examples  
- Implementing additional gateway events  
- Enhancing model coverage  
- Optimizing performance  

Open a pull request or create an issue on GitHub.

---

## 📄 License  
Fluxer++ is licensed under MIT.

[Back to top](https://github.com/Jw4Lk3r16/Fluxer.cpp/blob/main/README.md#fluxer "Back to top")
