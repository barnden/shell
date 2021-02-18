#include <string>
#include <iostream>
#include <unordered_set>
#include <vector>

#include <string.h>

#include "Shell.h"
#include "Tokenizer.h"

namespace BShell {
#ifdef DEBUG_TOKENIZER
const char* TokenNames[] = {
    "NULL", "STRING", "EQUAL", "EXECUTABLE", "BACKGROUND", "SEQUENTIAL",
    "SEQUENTIAL_CON", "PIPE", "REDIRECT_OUT", "REDIRECT_IN", "KEYWORD"
};

std::ostream& operator<<(std::ostream&os, const TokenType&type) {
    return (os << TokenNames[static_cast<uint8_t>(type)]);
}

std::ostream& operator<<(std::ostream&os, const Token&token) {
    return (os << "" << token.type << "\t\"" << token.content << '"');
}
#endif

std::unordered_set<std::string> KEYWORDS = {
    "export", "cd"
};

std::vector<Token> input$tokenize(const char*inp) {
    // Nothing to parse if empty string.
    if (!strlen(inp)) return std::vector<Token> {};

    auto str = std::string(inp), str_buf = std::string {};
    auto gobble = false, force_string = false;
    auto tokens = std::vector<Token> {};

    int quotes[3] = { 0, 0, 0 }; // count for: single, double, backticks

    // TODO: Maybe move lambda functions outside of input$tokenize(const char*)
    auto add_token = [&tokens, &str_buf](Token token) -> void {
        tokens.push_back(token);
        str_buf = "";
    };

    auto enquote = [&quotes]() -> int {
        // Usage:
        // if enquote != 0 then we know we are in some type of quotation mark
        // using bitmasks we can we can determine which type of quotes we are in, i.e.
        //    SINGLE: 0b110 = 6
        //    DOUBLE: 0b101 = 5
        // BACKTICKS: 0b011 = 3
        return quotes[2] % 2 << 2 |
               quotes[1] % 2 << 1 |
               quotes[0] % 2;
        };

    for (const auto&c : str) {
        if (gobble) {
            gobble = false;
            continue;
        }
    }

    auto add_quote = [&](int index, int mask) -> bool {
        auto ret = false;
        quotes[index] += !(enquote() & mask);

        if (ret = quotes[index] && !(quotes[index] % 2))
            add_token(Token { TokenType::STRING, str_buf.substr(1) } );

        return ret;
    };

    auto handle$str_buf = [&str_buf, &force_string, &add_token]() -> void {
        if (!str_buf.size()) return;

        auto type = TokenType::STRING;

        if (KEYWORDS.contains(str_buf))
            type = TokenType::KEY;
        else if (!force_string) {
            auto path = get$executable_path(str_buf);

            if (path.size()) {
                type = TokenType::EXECUTABLE;
                force_string = true;
            }
        }

        add_token(Token { type, str_buf });
    };

    for (const auto&c : str) {
        if (gobble) {
            gobble = false;
            continue;
        }

        char*next = nullptr;
        auto last = !(&*str.end() - &c - 1);

        if (!last)
            next = const_cast<char*>(&c + 1);

        switch (c) {
            case ' ':
                if (enquote()) break;
                handle$str_buf();
            continue;
            case '\'': if (add_quote(0, 6)) continue;
            break;
            case '"': if (add_quote(1, 5)) continue;
            break;
            case '`': if (add_quote(2, 3)) continue;
            break;
            case '>': case '<': case '|':
            case '=': case ';': case '&':
                if (enquote()) break;
                auto type = TokenType {};

                force_string = false;

                switch (c) {
                    case '>': type = REDIRECT_OUT; break;
                    case '<': type = REDIRECT_IN; break;
                    case '|': type = PIPE; break;
                    case '=':
                        type = EQUAL;
                        force_string = true;

                        handle$str_buf();
                    break;
                    case ';': type = SEQUENTIAL; break;
                    case '&':
                        if (next && *next != '&')
                            type = TokenType::BACKGROUND;
                        else {
                            type = TokenType::SEQUENTIAL_CON;
                            gobble = true;
                        }
                    break;
                }


                add_token(Token { type, str_buf });

            continue;
        }

        str_buf += c;

        if (last)
            handle$str_buf();
    }

    return tokens;
}
}
