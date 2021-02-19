#pragma once

#include <vector>

#include "Parser.h"

namespace BShell {
struct Pipe {
    int stdin[2],
         stdout[2],
         stderr[2];
};

struct Process {
    pid_t pid;
    std::string name;
    Pipe pipe;
};

extern std::vector<Process> processes;

// TODO: Implement $? and $! to get the exit code of processes.
extern int fg_exit; // $?
extern int bg_exit; // $!

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
std::string get$pname(Expression*expr);
std::string get$executable_path(std::string);

void set$cwd(std::string);
void edproc();

void handle$ast(Expression*);
}
