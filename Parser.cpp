#include <string>
#include <iostream>
#include <unistd.h>

#include "Tokenizer.h"
#include "Parser.h"
#include "Shell.h"

#define PARSE_ERR(msg) std::cerr << #msg << '\n';
#define EMPTY_AST\
    [&]() -> std::vector<Expression*> {\
        for (auto&ast : asts) ast$delete_children(ast);\
        return std::vector<Expression*> {};\
    }();

namespace BShell {
Expression::Expression() {}
Expression::Expression(Token token) : token(token) {}

const Token* next$pk(const std::vector<Token>&tokens, const Token*cur) {
    if (&*tokens.end() != ++cur)
        return cur;

    return nullptr;
}

const Token* prev$pk(const std::vector<Token>&tokens, const Token*cur) {
    if (&*tokens.begin() != --cur)
        return cur;
    
    return nullptr;
}

Expression* parse$executable(
    const std::vector<Token>&tokens,
    Token**cur
) {
    auto expr = new Expression(**cur);
    const Token*next = nullptr;

    while ((next = next$pk(tokens, *cur)) != nullptr && next->type == TokenType::STRING) {
        expr->children.push_back(new Expression(*next));
        (*cur)++;
    }

    return expr;
}

Expression* parse$pipe(
    const std::vector<Token>&tokens,
    std::vector<Expression*>&asts,
    Token**cur
) {
    Expression*expr = nullptr;

    if (asts.size() && asts.back()->token.type != TokenType::STRING) {
        auto*next = next$pk(tokens, (*cur)++);

        if (next == nullptr || next->type != TokenType::EXECUTABLE) {
            // We do not currently support a continuation prompt
            PARSE_ERR(Syntax error at unexpected token '|'.);

            return nullptr;
        }

        expr = new Expression(**cur);

        expr->children.push_back(asts.back());
        expr->children.push_back(new Expression(*next));

        asts.pop_back();
    } else PARSE_ERR(Syntax error at unexpected token '|'.);

    return expr;
}

std::vector<Expression*> input$parse(std::vector<Token>&tokens) {
    auto asts = std::vector<Expression*> {};
    if (!tokens.size()) return EMPTY_AST;

    auto*cur = &*tokens.begin();

    for (; cur < &*tokens.end(); cur++) {
        Expression*expr = nullptr;

        switch (cur->type) {
            case EXECUTABLE: asts.push_back(parse$executable(tokens, &cur));
            continue;
            case PIPE: {
                auto pexpr = parse$pipe(tokens, asts, &cur);

                if (!pexpr)
                    return EMPTY_AST;

                asts.push_back(pexpr);
            }
            continue;
        }
    }

    return asts;
}

void ast$delete_children(Expression*expr){
    if (expr->children.size())
        for (auto*child : expr->children)
            ast$delete_children(child);

    delete expr;
}
}
