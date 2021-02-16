#pragma once

#include <unordered_set>

namespace BShell {
extern std::unordered_set<std::string> KEYWORDS;

bool keywords$contains(std::string);
void parse$input(const char*);
}
