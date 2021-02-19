#pragma once

namespace BShell {
std::string parse$PS_FMTSTR(const char&);
std::string parse$PS(const std::string);

const std::string get$PS1();

void set$PS1(std::string);

extern std::string PS1;
}
