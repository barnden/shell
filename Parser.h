#pragma once

#include <unordered_set>
#include <vector>

#include "Tokenizer.h"

namespace BShell {
extern std::unordered_set<std::string> KEYWORDS;

struct Expression {
    Expression();
    Expression(Token*);

    std::vector<Expression*> children;
    Token* token;
};

std::vector<Expression*> input$parse(const std::vector<Token>);
bool keywords$contains(std::string);
}
