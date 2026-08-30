// send_message.cpp  (or paste into ping_bot.cpp)
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#pragma comment(lib, "winhttp.lib")

bool send_message_winhttp(const std::string& token, const std::string& channel_id, const std::string& content) {
    nlohmann::json body = { {"content", content} };
    std::string bodyStr = body.dump(); // UTF-8

    HINTERNET hSession = WinHttpOpen(L"PingBot/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, L"api.fluxer.app", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    std::string path = "/channels/" + channel_id + "/messages";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring wpath(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], wlen);

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wpath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    std::string headers = "Content-Type: application/json\r\nAuthorization: Bot " + token + "\r\n";
    int hwlen = MultiByteToWideChar(CP_UTF8, 0, headers.c_str(), -1, nullptr, 0);
    std::wstring wheaders(hwlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, headers.c_str(), -1, &wheaders[0], hwlen);

    BOOL sent = WinHttpSendRequest(
        hRequest,
        wheaders.c_str(),
        (DWORD)(wheaders.size() - 1),
        (LPVOID)bodyStr.c_str(),
        (DWORD)bodyStr.size(),
        (DWORD)bodyStr.size(),
        0
    );

    if (!sent || !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Read status and response body for debugging
    DWORD status = 0; DWORD statusSize = sizeof(status);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);

    std::string response;
    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        std::string chunk(bytesAvailable, '\0');
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, &chunk[0], bytesAvailable, &bytesRead) && bytesRead > 0) {
            chunk.resize(bytesRead);
            response += chunk;
        } else break;
    }

    std::cout << "HTTP status: " << status << "\n";
    std::cout << "Response body: " << response << "\n";

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return (status >= 200 && status < 300);
}
