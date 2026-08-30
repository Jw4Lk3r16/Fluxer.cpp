#pragma once
#include <string>

namespace fluxerpp {

class GatewayClient {
public:

    std::string token;

    GatewayClient(const std::string& token);
    void connect();

private:

};

}
