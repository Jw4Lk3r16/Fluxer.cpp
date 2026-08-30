// main.cpp
#include "fluxerpp/GatewayClient.h"
#include <windows.h>
#include <atomic>
#include <thread>
#include <iostream>
#include <chrono>

static std::atomic<bool> g_running{true};

BOOL WINAPI ConsoleHandler(DWORD ctrlType) {
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        g_running.store(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return TRUE;
    default:
        return FALSE;
    }
}

int main(int argc, char** argv) {
    std::string token;
    if (argc >= 2) token = argv[1];
    else {
        char* env = nullptr;
        size_t len = 0;
        errno_t err = _dupenv_s(&env, &len, "FLUXER_TOKEN");
        if (err == 0 && env) { token = env; free(env); }
    }

    if (token.empty()) {
        std::cerr << "Usage: ping_bot <token> or set FLUXER_TOKEN\n";
        return 1;
    }

    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    fluxerpp::GatewayClient client(token, g_running);

    std::thread t([&client]() {
        client.connect();
    });

    std::cout << "[Main] Bot started. Press Ctrl+C or close the console to stop.\n";

    while (g_running.load()) std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "[Main] Shutdown requested. Waiting for gateway to exit...\n";
    if (t.joinable()) t.join();
    std::cout << "[Main] Exiting.\n";
    return 0;
}
