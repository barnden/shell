#include <string>
#include <iostream>
#include <vector>

#include "Tokenizer.h"
#include "Parser.h"

#define PARSER_ERR(msg) { std::cerr << msg << '\n'; m_err = true; }

namespace BShell {
Expression::Expression() {}
Expression::Expression(Token token) : token(token) {}

Parser::Parser(std::vector<Token>&tokens) :
    m_tokens(tokens),
    m_cur(),
    m_next(),
    m_asts(),
    m_err()
{
    parse();
}

std::vector<Expression*> Parser::asts() const { return m_asts; }

Expression* Parser::glue_sticky() {
    Expression* expr = nullptr;

    if (m_cur->type == StickyRight &&
        peek() &&
        (
            peek()->type == String ||
            peek()->type == StickyLeft
        )
    ) {
        expr = new Expression(Token {
            String,
            m_cur->content + peek()->content
        });

        m_cur++;
    }

    if (m_cur->type == String && peek() && peek()->type == StickyLeft) {
        if (expr)
            expr->token.content += peek()->content;
        else
            expr = new Expression(Token {
                String,
                m_cur->content + peek()->content
            });

        m_cur++;
    }

    return expr;
}

void Parser::add_strings(Expression* expr) {
    while (
        (m_next = peek()) != nullptr &&
        (
            m_next->type == StickyRight ||
            m_next->type == StickyLeft ||
            m_next->type == String ||
            m_next->type == Eval
        )
    ) {
        m_cur++;

        auto glue = glue_sticky();

        expr->children.push_back(glue ? glue : new Expression(*m_next));
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

void Parser::parse_current() {
    switch (m_cur->type) {
        case Key:
        case Executable: {
            auto expr = new Expression(*m_cur);

            add_strings(expr);
            m_asts.push_back(expr);
            }
            break;
        case RedirectIn:
        case RedirectOut:
            parse_redirection();
            break;
        case RedirectPipe:
        case SequentialIf:
        case Sequential:
            parse_sequential();
            break;
        case Equal:
            parse_equal();
            break;
        case Background:
            parse_background();
            break;
    }
}

void Parser::parse_redirection() {
    // Redirections should always be the child of an executable.
    if (m_asts.size() && (
        m_asts.back()->token.type == RedirectPipe ||
        m_asts.back()->token.type == Executable
    )) {
        if (m_next == nullptr || (m_next->type != Eval && m_next->type != String && m_next->type != StickyLeft)) {
            // Should probably make a lookup for the token's corresponding char
            PARSER_ERR("Syntax error at unexpected redirection token.");
            return;
        }

        auto expr = new Expression(*m_cur);
        Expression* exec = nullptr;

        if (m_asts.back()->token.type == RedirectPipe)
            exec = m_asts.back()->children.back();
        else
            exec = m_asts.back();

        expr->children.push_back(new Expression(*++m_cur));
        exec->children.push_back(expr);
    } else PARSER_ERR("Syntax error near unexpected redirection token.");
}

void Parser::parse_sequential() {
    auto type = m_cur->type;

    if (m_asts.size() && m_asts.back()->token.type != String) {
        if (m_next == nullptr || (m_next->type != Executable && m_next->type != Key)) {
            // We do not currently support a continuation prompt
            PARSER_ERR("Syntax error at unexpected token '|'.");
            return;
        }

        Expression* expr = nullptr;

        // Instead of having a multi-level tree for all the pipes
        // flatten the tree into one layer, where children from
        // left have higher precedence when executing.

        if (m_asts.back()->token.type == type) {
            expr = m_asts.back();
        } else {
            expr = new Expression(*m_cur);
            expr->children.push_back(m_asts.back());
        }

        m_asts.pop_back();

        m_cur++;
        parse_current();

        expr->children.push_back(m_asts.back());
        m_asts.pop_back();

        m_asts.push_back(expr);
    } else PARSER_ERR("Syntax error near unexpected token '|'.");
}

void Parser::parse_equal() {
    if (m_asts.size() && m_asts.back()->token.type == String) {
        auto expr = new Expression(*m_cur);
        expr->children.push_back(m_asts.back());

        m_asts.pop_back();
    } else PARSER_ERR("Syntax error near unexpected token '='.")
}

void Parser::parse() {
    if (!m_tokens.size()) return;

    m_cur = &*m_tokens.begin();

    while (m_cur < &*m_tokens.end()) {
        if (m_err) {
            for (auto& ast : m_asts)
                ast$delete_children(ast);

            m_asts = std::vector<Expression*> {};
            return;
        }

        m_next = peek();

        parse_current();

        m_cur++;
    }
}

Token* Parser::peek() {
    return  &*m_tokens.end() != m_cur + 1 ? m_cur + 1 : nullptr;
}

void ast$delete_children(Expression* expr){
    if (expr->children.size())
        for (auto*child : expr->children)
            ast$delete_children(child);

    delete expr;
}

void ast$print(Expression* expr, int depth) {
    for (auto i = 0; i < depth; i++)
        std::cout << "    ";

    std::cout << expr->token << '\n';

    if (expr->children.size())
        for (auto i = 0; i < expr->children.size(); i++)
            ast$print(expr->children[i], depth + 1);
}

void ast$print(Expression* expr) {
    ast$print(expr, 0);
}
}
