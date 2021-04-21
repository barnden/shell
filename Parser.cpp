#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Parser.h"
#include "Tokenizer.h"

#define PARSER_ERR(msg)           \
    {                             \
        std::cerr << msg << '\n'; \
        m_err = true;             \
    }

namespace BShell {
Expression::Expression() { }
Expression::Expression(Token token)
    : token(token) { }

Parser::Parser(std::vector<Token>&& tokens)
    : m_tokens(tokens)
    , m_cur()
    , m_next()
    , m_asts()
    , m_err() {
    parse();
}

std::vector<std::shared_ptr<Expression>> Parser::asts() const {
    return m_err ? std::vector<std::shared_ptr<Expression>> {} : m_asts;
}

std::shared_ptr<Expression> Parser::glue_sticky() {
    auto expr = std::shared_ptr<Expression> {};

    if (!peek())
        return nullptr;

    if (m_cur->type == StickyRight && peek()->type & (String | StickyLeft)) {
        expr = std::make_shared<Expression>(Token {
            String,
            m_cur->content + peek()->content });

        m_cur++;
    }

    if (m_cur->type == String && peek()->type == StickyLeft) {
        if (expr)
            expr->token.content += peek()->content;
        else
            expr = std::make_shared<Expression>(Token {
                String,
                m_cur->content + peek()->content });

        m_cur++;
    }

    return expr;
}

void Parser::add_strings(std::shared_ptr<Expression> const& expr) {
    while ((m_next = peek()) != nullptr) {
        if (!(m_next->type & (StickyRight | StickyLeft | String | Eval)))
            break;

        m_cur++;

        auto glue = glue_sticky();

        expr->children.push_back(glue ? glue : std::make_shared<Expression>(*m_next));
    }
}

void Parser::parse_background() {
    auto expr = std::shared_ptr<Expression> {};

    if (!m_asts.size() || m_asts.back()->token.type != Executable)
        PARSER_ERR("Syntax error near unexpected token '&'.");

    expr = std::make_shared<Expression>(*m_cur);

    expr.get()->children.push_back(m_asts.back());

    m_asts.pop_back();
    m_asts.push_back(expr);
}

void Parser::parse_current() {
    switch (m_cur->type) {
    case Key:
    case Executable: {
        auto expr = std::make_shared<Expression>(*m_cur);

        add_strings(expr);
        m_asts.push_back(expr);
    } break;
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
    case String:
    case StickyRight:
    case StickyLeft:
        m_asts.push_back(std::make_shared<Expression>(*m_cur));
        break;
    }
}

void Parser::parse_redirection() {
    // Redirections should always be the child of an executable.
    if (m_asts.size() && (m_asts.back()->token.type & (RedirectPipe | Executable))) {
        if (m_next == nullptr || !(m_next->type & (Eval | String | StickyLeft))) {
            // Should probably make a lookup for the token's corresponding char
            PARSER_ERR("Syntax error at unexpected redirection token.");
            return;
        }

        auto expr = std::make_shared<Expression>(*m_cur);
        auto exec = std::shared_ptr<Expression> {};

        if (m_asts.back()->token.type == RedirectPipe)
            exec = m_asts.back()->children.back();
        else
            exec = m_asts.back();

        expr->children.push_back(std::make_shared<Expression>(*++m_cur));
        exec->children.push_back(expr);
    } else {
        PARSER_ERR("Syntax error near unexpected redirection token.");
    }
}

void Parser::parse_sequential() {
    auto type = m_cur->type;

    if (m_asts.size() && m_asts.back()->token.type != String) {
        if (m_next == nullptr || !(m_next->type & (Executable | Key))) {
            // We do not currently support a continuation prompt
            PARSER_ERR("Syntax error at unexpected token '|'.");
            return;
        }

        auto expr = std::shared_ptr<Expression> {};

        // Instead of having a multi-level tree for all the pipes
        // flatten the tree into one layer, where children from
        // left have higher precedence when executing.

        if (m_asts.back()->token.type == type) {
            expr = m_asts.back();
        } else {
            expr = std::make_shared<Expression>(*m_cur);
            expr->children.push_back(m_asts.back());
        }

        m_asts.pop_back();

        m_cur++;
        parse_current();

        expr->children.push_back(m_asts.back());
        m_asts.pop_back();

        m_asts.push_back(expr);
    } else
        PARSER_ERR("Syntax error near unexpected token '|'.");
}

void Parser::parse_equal() {
    if (m_asts.size() && m_asts.back()->token.type & (String | StickyRight | StickyLeft)) {
        if (m_next == nullptr || !(m_next->type & (String | StickyRight | StickyLeft))) {
            PARSER_ERR("Syntax error new unexpected token '='.")
            return;
        }

        auto expr = std::make_shared<Expression>(*m_cur);
        expr->children.push_back(m_asts.back());
        expr->children.push_back(std::make_shared<Expression>(*++m_cur));

        m_asts.pop_back();
        m_asts.push_back(expr);
    } else
        PARSER_ERR("Syntax error near unexpected token '='.")
}

void Parser::parse() {
    if (!m_tokens.size())
        return;

    m_cur = &*m_tokens.begin();

    while (m_cur < &*m_tokens.end()) {
        if (m_err)
            return;

        m_next = peek();

        parse_current();

        m_cur++;
    }
}

Token* Parser::peek() const {
    if (&*m_tokens.end() != m_cur + 1)
        return m_cur + 1;

    return nullptr;
}

void ast$print(std::shared_ptr<Expression> expr, int depth) {
    for (auto i = 0; i < depth; i++)
        std::cout << "    ";

    std::cout << expr->token << '\n';

    if (expr->children.size())
        for (auto i = 0; i < expr->children.size(); i++)
            ast$print(expr->children[i], depth + 1);
}

void ast$print(std::shared_ptr<Expression> expr) {
    ast$print(expr, 0);
}
}
