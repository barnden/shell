#include <string>
#include <iostream>

#include "Tokenizer.h"
#include "Parser.h"
#include "Shell.h"

namespace BShell {
struct Expression {
    // Assumption: All expressions are binary or unary.
    Expression*left = nullptr, *right = nullptr;
    TokenType op;

    Expression() : op(TokenType::NTOKEN) {};
    Expression(TokenType op) : op(op) {};
    Expression(TokenType op, Expression*left) : op(op), left(left) {};
    Expression(TokenType op, Expression*left, Expression*right) : op(op), left(left), right(right) {};
};

void input$parse(const char*inp) {
    const auto tokens = input$tokenize(inp);

    auto i = 0;

    auto prev = [&]() -> const Token* { return i > 0 ? &tokens[i - 1] : nullptr; };
    auto next = [&]() -> const Token* { return i < tokens.size() - 1 ? &tokens[i + 1] : nullptr; };
    auto go_next = [&]() -> const Token* {
        auto temp = next();
        i++;
        return temp;
    };

    while (i < tokens.size()) {
        const auto&cur = tokens[i];
        Expression*expr;

        switch (cur.type) {
            case PIPE:
            expr = new Expression();
            break;
        }
    }
}
}
