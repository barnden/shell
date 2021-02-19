#pragma once

#include <vector>

#include "Tokenizer.h"

namespace BShell {
struct Expression {
    Expression();
    Expression(Token);

    std::vector<Expression*> children;
    Token token;
};

class Parser {
public:
    Parser(std::vector<Token>&);
    std::vector<Expression*> asts() const;
private:
    void parse();
    void add_strings(Expression*);
    void parse_pipe();
    void parse_background();
    Token* peek();
    Token* peek_back();

    bool m_err;
    Token *m_cur, *m_prev, *m_next;
    std::vector<Expression*> m_asts;
    std::vector<Token>&m_tokens;
};

std::vector<Expression*> input$parse(std::vector<Token>&);

void ast$delete_children(Expression*);
}
