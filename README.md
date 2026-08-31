# Fluxer++
**Fluxer++** is a native **C++ SDK** for building fast, reliable Fluxer bots. It provides a modern REST client, real‑time gateway handling, type‑safe models, and an event‑driven architecture designed for high‑performance applications on the Fluxer platform.

## Project Structure
```
fluxerpp/
│
├── include/
│   ├── FluxerClient.h
│   ├── RestClient.h
│   ├── GatewayClient.h
│   ├── EventDispatcher.h
│   └─── fluxerpp/
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
│   │    
│   │
│   └── util/
│
├── examples/
│   ├── ping_bot.cpp
│   └── echo_bot.cpp
│
└── CMakeLists.txt
```