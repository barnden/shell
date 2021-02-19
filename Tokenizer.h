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
    Eval            // eval string (enclosed in backticks or "$()")
};

struct Token {
    TokenType type;
    std::string content;
};

class Tokenizer {
public:
    Tokenizer(const char*);

    std::vector<Token> tokens() const;
private:
    void tokenize_input();
    void add_token(Token);
    bool add_quote(int, int);
    char enquote() const;
    void add_string_buf();

    const char* m_input;
    std::string m_string_buf;
    int m_quotes[4];
    bool m_gobble, m_force_string;
    std::vector<Token> m_tokens;
};

std::ostream& operator<<(std::ostream&, const Token&);
std::ostream& operator<<(std::ostream&, const TokenType&);

extern std::unordered_set<std::string> g_keywords;
}
