#pragma once

#include <memory>
#include <vector>

#include "Tokenizer.h"

namespace BShell {
struct Expression {
    Expression();
    Expression(Token);

    std::vector<std::shared_ptr<Expression>> children;
    Token token;
};

class Parser {
public:
    Parser(std::vector<Token>&&);
    std::vector<std::shared_ptr<Expression>> asts() const;

private:
    void parse();
    void add_strings(std::shared_ptr<Expression> const&);
    void parse_background();
    void parse_sequential();
    void parse_redirection();
    void parse_current();
    void parse_equal();

    std::shared_ptr<Expression> glue_sticky();

    Token* peek() const;

    bool m_err;
    Token *m_cur, *m_next;

    std::vector<std::shared_ptr<Expression>> m_asts;
    std::vector<Token>& m_tokens;
};

void ast$print(std::shared_ptr<Expression>);
}
