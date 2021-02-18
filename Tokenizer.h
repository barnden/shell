#pragma once

#include <string>
#include <vector>
#include <unordered_set>

#define DEBUG_TOKENIZER

namespace BShell {
extern std::unordered_set<std::string> KEYWORDS;
enum TokenType : uint8_t {
    NTOKEN,         // null token
    STRING,         // a string
    EQUAL,          // equal symbol, typically used for setting env variables
    EXECUTABLE,     // executable
    BACKGROUND,     // modifier for executable, backgrounded process (&)
    SEQUENTIAL,     // modifier for executable, sequential execution (;)
    SEQUENTIAL_CON, // modifier for executable, conditional sequential execution (&&)
    PIPE,           // pipe stdout to another process stdin (|)
    REDIRECT_OUT,   // redirect out (>)
    REDIRECT_IN,    // redirect in (<)
    KEY,            // keyword for shell (cd, ?export)
};

struct Token {
    TokenType type;
    std::string content;
};

std::vector<Token> input$tokenize(const char*);

#ifdef DEBUG_TOKENIZER
std::ostream& operator<<(std::ostream&, const Token&);
std::ostream& operator<<(std::ostream&, const TokenType&);
#endif
}
