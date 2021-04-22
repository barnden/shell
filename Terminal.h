#pragma once

#include <vector>

#include <termios.h>

namespace BShell {
std::string get$input(std::string const&);
void terminal$control();
void terminal$restore();
void handle$sigint(int);

extern termios g_term;
extern termios g_oterm;
extern std::vector<std::string> g_history;
}