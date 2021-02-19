#include <string>
#include <iostream>
#include <vector>

#include "Tokenizer.h"
#include "Parser.h"

#define PARSER_ERR(msg) std::cerr << msg << '\n'; m_err = true;
#define EMPTY_AST\
    [&]() -> std::vector<Expression*> {\
        for (auto&ast : asts) ast$delete_children(ast);\
        return std::vector<Expression*> {};\
    }();
#define ADD_NOT_EMPTY_EARLY_RETURN(func){\
    auto pexpr = func;\
    if (!pexpr) return EMPTY_AST;\
    asts.push_back(pexpr);\
    }

namespace BShell {
Expression::Expression() {}
Expression::Expression(Token token) : token(token) {}

Parser::Parser(std::vector<Token>&tokens) :
    m_tokens(tokens),
    m_prev(),
    m_cur(),
    m_next(),
    m_asts(),
    m_err()
{
    parse();
}

std::vector<Expression*> Parser::asts() const { return m_asts; }

void Parser::add_strings(Expression* expr) {
    while ((m_next = peek()) != nullptr && m_next->type == String) {
        expr->children.push_back(new Expression(*m_next));
        m_cur++;
    }
}

void Parser::parse_background() {
    Expression* expr = nullptr;

    if (m_asts.size() && m_asts.back()->token.type == Executable) {
        expr = new Expression(*m_cur);

        expr->children.push_back(m_asts.back());

        m_asts.pop_back();
        m_asts.push_back(expr);
    } else PARSER_ERR("Syntax error near unexpected token '&'.");
}

void Parser::parse_pipe() {
    Expression* expr = nullptr;

    if (m_asts.size() && m_asts.back()->token.type != String) {
        if (m_next == nullptr || m_next->type != Executable) {
            // We do not currently support a continuation prompt
            PARSER_ERR("Syntax error at unexpected token '|'.");
            return;
        }

        expr = new Expression(*m_cur);

        expr->children.push_back(m_asts.back());
        expr->children.push_back(new Expression(*m_next));

        m_asts.pop_back();
        m_asts.push_back(expr);
    } else PARSER_ERR("Syntax error near unexpected token '|'.");
}

void Parser::parse() {
    if (!m_tokens.size()) return;

    m_cur = &*m_tokens.begin();

    for (; m_cur < &*m_tokens.end(); m_cur++) {
        if (m_err) {
            for (auto& ast : m_asts)
                ast$delete_children(ast);

            m_asts = std::vector<Expression*> {};
            return;
        }

        Expression*expr = nullptr;
        m_next = peek();
        m_prev = peek_back();

        switch (m_cur->type) {
            case Key:
            case Executable: {
                auto expr = new Expression(*m_cur);

                add_strings(expr);
                m_asts.push_back(expr);
                }
                continue;
            case RedirectPipe:
                parse_pipe();
                continue;
            case Background:
                parse_background();
                continue;
        }
    }
}

Token* Parser::peek() {
    return  &*m_tokens.end() != m_cur + 1 ? m_cur + 1 : nullptr;
}

Token* Parser::peek_back() {
    return &*m_tokens.begin() != m_cur - 1 ? m_cur - 1 : nullptr;
}

void ast$delete_children(Expression*expr){
    if (expr->children.size())
        for (auto*child : expr->children)
            ast$delete_children(child);

    delete expr;
}
}
