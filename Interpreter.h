#pragma once

#include <vector>

#include "Parser.h"
#include "Tokenizer.h"

namespace BShell {
struct Pipe {
    int fd[2];
};

struct Process {
    pid_t pid;
    std::string name;
};

enum KEYWORD { UNKNOWN, EXPORT, CD };

void erase_dead_children();

void handle$argv_strings(std::vector<std::string>&, bool&, Token const&);

template <typename T> void handle$ast(std::shared_ptr<Expression>&&, T&&);
void handle$ast(std::shared_ptr<Expression>&&);

extern std::string g_prev_wd;
extern std::vector<Process> g_processes;

// TODO: Implement $? and $! to get the exit code of processes.
extern int g_exit_fg; // $?
extern int g_exit_bg; // $!
}
