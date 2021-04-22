#pragma once

namespace BShell {
std::string parse$PS_FMTSTR(char const&);
std::string parse$PS(std::string const&);

std::string const get$PS1();

void set$PS1(std::string);

extern std::string PS1;
}
