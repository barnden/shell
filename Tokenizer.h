#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace BShell {
enum TokenType : uint16_t {
    NullToken       = 0,        // null token
    String          = 1,        // a string
    Equal           = 1 << 1,   // equal symbol, typically used for setting env variables
    Executable      = 1 << 2,   // executable
    Background      = 1 << 3,   // backgrounded process (&)
    Sequential      = 1 << 4,   // sequential execution (;)
    SequentialIf    = 1 << 5,   // conditional sequential execution (&&)
    RedirectPipe    = 1 << 6,   // pipe stdout to another process stdin (|)
    RedirectOut     = 1 << 7,   // redirect out (>)
    RedirectIn      = 1 << 8,   // redirect in (<)
    Key             = 1 << 9,   // keyword for shell (cd, ?export)
    Eval            = 1 << 10,  // eval string (enclosed in backticks or "$()")
    StickyRight     = 1 << 11,  // sticky string (right), makes a string with next token
    StickyLeft      = 1 << 12,  // sticky string (left), makes a string with previous token
    WhiteSpace      = 1 << 13,  // whitespace
};

struct Token {
    TokenType type;
    std::string content;
};

class Tokenizer {
public:
    Tokenizer(std::string const&, bool = false);

    std::vector<Token> tokens() const;

private:
    void tokenize_input();
    void add_token(Token);
    bool add_quote(int, int, std::string);
    char enquote() const;
    void add_string_buf();

    std::string m_input, m_string_buf;
    int m_quotes[4];
    bool m_gobble, m_force_string, m_make_sticky_l, m_make_sticky_r, m_preserve_whitespace;
    std::vector<Token> m_tokens;
};

std::ostream& operator<<(std::ostream&, Token const&);
std::ostream& operator<<(std::ostream&, TokenType const&);

extern std::unordered_set<std::string> g_keywords;
}
