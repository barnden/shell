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
    void parse_background();
    void parse_sequential();
    void parse_redirection();
    void parse_current();

    Expression* glue_sticky();

    Token* peek();

    bool m_err;
    Token *m_cur, *m_next;
    std::vector<Expression*> m_asts;
    std::vector<Token>& m_tokens;
};

std::vector<Expression*> input$parse(std::vector<Token>&);

void ast$delete_children(Expression*);
void ast$print(Expression*);
}
