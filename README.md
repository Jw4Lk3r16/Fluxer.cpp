# Fluxer++
**Fluxer++** is a native **C++ SDK** for building fast, reliable Fluxer bots. It provides a modern REST client, real‑time gateway handling, type‑safe models, and an event‑driven architecture designed for high‑performance applications on the Fluxer platform.

## Project Structure
```
fluxerpp/
│
├── include/fluxerpp/
│   ├── FluxerClient.h
│   ├── RestClient.h
│   ├── GatewayClient.h
│   ├── EventDispatcher.h
│   ├── models/
│   │   ├── Message.h
│   │   ├── Channel.h
│   │   ├── User.h
│   │   └── Guild.h
│   └── util/
│       ├── Json.h
│       └── Logger.h
│
├── src/
│   ├── FluxerClient.cpp
│   ├── RestClient.cpp
│   ├── GatewayClient.cpp
│   ├── EventDispatcher.cpp
│   ├── models/
│   └── util/
│
├── examples/
│   ├── ping_bot.cpp
│   └── echo_bot.cpp
│
└── CMakeLists.txt
```