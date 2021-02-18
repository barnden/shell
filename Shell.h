#pragma once

#include <vector>

namespace BShell {
struct Process {
    pid_t pid;
    uint8_t type;
};
extern std::vector<Process> processes;

enum KEYWORD {
    UNKNOWN,
    EXPORT,
    CD
};

std::string get$cwd();
std::string get$home();
std::string get$username();
std::string get$hostname();
std::string get$executable_path(std::string);

void handle$keyword(KEYWORD, const char*);
void set$cwd(std::string);
}
