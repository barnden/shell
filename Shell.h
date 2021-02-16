#pragma once

#include <vector>

namespace BShell {
extern std::vector<uint16_t> bg_procs;

enum KEYWORD {
    UNKNOWN,
    EXPORT,
    CD
};

std::string get$cwd();
std::string get$home();
std::string get$username();
std::string get$hostname();

void handle$keyword(KEYWORD, const char*);

void set$cwd(std::string);
}
