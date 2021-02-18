#pragma once

#include <unordered_set>
#include "Tokenizer.h"

namespace BShell {
extern std::unordered_set<std::string> KEYWORDS;

void input$parse(const char*);
bool keywords$contains(std::string);
}
