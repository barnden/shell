#pragma once

namespace BShell {
extern std::string PS1;

std::string parse$PS_FMTSTR(const char&);
std::string parse$PS(const std::string);

const std::string get$PS1();

void set$PS1(std::string);
}
