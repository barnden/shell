#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include <string.h>

#include "Shell.h"
#include "Tokenizer.h"

const char* TokenNames[] = {
    "NULL", "STRING", "EQUAL", "EXECUTABLE", "BACKGROUND", "SEQUENTIAL",
    "SEQUENTIAL_CON", "PIPE", "REDIRECT_OUT", "REDIRECT_IN", "KEYWORD", "EVAL"
};

namespace BShell {
std::unordered_set<std::string> g_keywords = {
    "export", "cd", "jobs"
};

Tokenizer::Tokenizer(const char*input) :
    m_input(input),
    m_string_buf(),
    m_quotes(),
    m_gobble(),
    m_force_string()
{
    tokenize_input();
}

std::vector<Token> Tokenizer::tokens() const {
    return m_tokens;
}

void Tokenizer::add_token(Token token) {
    m_tokens.push_back(token);
    m_string_buf = "";
}

bool Tokenizer::add_quote(int index, int mask) {
    m_quotes[index] += !(enquote() & mask);

    if (enquote() & (0xF ^ mask))
        add_string_buf();

    if (m_quotes[index] && !(m_quotes[index] % 2)) {
        add_token(Token { index <= 1 ? String : Eval,
            (m_string_buf.size() > 1) ? m_string_buf.substr(1) : m_string_buf
        });

        return true;
    }

    return false;
}

char Tokenizer::enquote() const {
    auto status = char {};

    [&]<std::size_t ...I>(std::index_sequence<I...>) {
        ((status |= m_quotes[I] % 2 << I), ...);
    }(std::make_index_sequence<4>{});

    return status;
}

void Tokenizer::add_string_buf() {
    if (!m_string_buf.size()) return;

    auto type = TokenType::String;

    if (g_keywords.contains(m_string_buf)) {
        type = TokenType::Key;
        m_force_string = true;
    } else if (!m_force_string) {
        auto path = get$executable_path(m_string_buf);

        if (path.size()) {
            type = TokenType::Executable;
            m_force_string = true;
        }
    }

    add_token(Token { type, m_string_buf });
}

void Tokenizer::tokenize_input() {
    // Nothing to parse if empty string.
    if (!strlen(m_input)) return;

    auto input = std::string(m_input);

    // Iterate through each character in the input
    // We use a one character look ahead to match any multi-character operators
    for (const auto&c : input) {
        if (m_gobble) {
            m_gobble = false;
            continue;
        }

        auto last = !(&*input.end() - &c - 1);
        char*next = !last ? const_cast<char*>(&c + 1) : nullptr;

        switch (c) {
            case ' ':
                if (enquote())
                    break;

                add_string_buf();

                continue;
            case '\'':
                if (add_quote(0, 0xE))
                    continue;
                break;
            case '"':
                if (add_quote(1, 0xD))
                    continue;
                break;
            case '`':
                if (add_quote(2, 0xB))
                    continue;
                break;
            case '$':
                if (next && *next == '(') {
                    m_gobble = true;

                    if (add_quote(3, 0x7))
                        continue;
                }
                break;
            case ')':
                if (enquote() & 8)
                    if (add_quote(3, 0x7))
                        continue;
                break;
            case '>': case '<': case '|':
            case '=': case ';': case '&': {
                if (enquote())
                    break;

                auto type = TokenType {};
                auto override_string = false,
                     pass = false;

                switch (c) {
                    case '>':
                        type = RedirectOut;
                        override_string = true;
                        break;
                    case '<':
                        type = RedirectIn;
                        override_string = true;
                        break;
                    case '|': type = RedirectPipe; break;
                    case '=':
                        if (!m_force_string) {
                            type = Equal;
                            override_string = true;
                        } else pass = true;
                    break;
                    case ';': type = Sequential; break;
                    case '&':
                        if (next && *next == '&') {
                            m_gobble = true;
                            type = SequentialIf;
                        } else type = Background;
                    break;
                }

                if (!pass) {
                    add_string_buf();
                    m_force_string = override_string;

                    add_token(Token { type, "" });
                    continue;
                } else break;
            }
        }

        m_string_buf += c;

        if (last)
            add_string_buf();
    }
}

std::ostream& operator<<(std::ostream&os, const TokenType&type) {
    return (os << TokenNames[static_cast<uint8_t>(type)]);
}

std::ostream& operator<<(std::ostream&os, const Token&token) {
    return (os << "" << token.type << "\t\"" << token.content << '"');
}
}
