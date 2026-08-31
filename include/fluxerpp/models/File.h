#pragma once
#include <string>

namespace fluxerpp::models {
class File {
public:
    util::Json to_dict() const { return util::Json::object(); }
};
}