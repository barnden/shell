#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace BShell {
enum TokenType : uint8_t {
    NullToken,      // null token
    String,         // a string
    Equal,          // equal symbol, typically used for setting env variables
    Executable,     // executable
    Background,     // backgrounded process (&)
    Sequential,     // sequential execution (;)
    SequentialIf,   // conditional sequential execution (&&)
    RedirectPipe,   // pipe stdout to another process stdin (|)
    RedirectOut,    // redirect out (>)
    RedirectIn,     // redirect in (<)
    Key,            // keyword for shell (cd, ?export)
    Eval,           // eval string (enclosed in backticks or "$()")
    StickyRight,    // sticky string (right), makes a string with next token
    StickyLeft,     // sticky string (left), makes a string with previous token
    WhiteSpace,     // whitespace
};

struct Token {
    TokenType type;
    std::string content;
};

class Tokenizer {
public:
    Tokenizer(std::string, bool = false);

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

std::ostream& operator<<(std::ostream&, const Token&);
std::ostream& operator<<(std::ostream&, const TokenType&);

extern std::unordered_set<std::string> g_keywords;
}
