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

enum KEYWORD {
    UNKNOWN,
    EXPORT,
    CD
};

std::string get$cwd();
std::string get$home();
std::string get$username();
std::string get$hostname();
std::string get$pname(pid_t);
std::string get$pname(const std::shared_ptr<Expression>&);
std::string get$executable_path(std::string);

void set$cwd(std::string);
void erase_dead_children();

void handle$argv_strings(std::vector<std::string>&, bool&, Token);

template<typename T>
void handle$ast(const std::shared_ptr<Expression>&, T&&);
void handle$ast(const std::shared_ptr<Expression>&);

extern std::string g_prev_wd;
extern std::vector<Process> g_processes;

// TODO: Implement $? and $! to get the exit code of processes.
extern int g_exit_fg; // $?
extern int g_exit_bg; // $!
}
