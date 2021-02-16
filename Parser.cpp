#include <string>
#include <iostream>
#include <unordered_set>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "Parser.h"
#include "Shell.h"

namespace BShell {
std::unordered_set<std::string> KEYWORDS = {
    "export", "cd"
};

bool keywords$contains(std::string w) { return KEYWORDS.contains(w); }

void parse$input(const char*inp) {
    auto str = std::string(inp);
    auto str_buf = std::string {};

    char*argv[3];

    // Nothing to parse if empty string.
    if (!str.size()) return;

    if (KEYWORDS.contains(str))
        return handle$keyword(
            static_cast<KEYWORD>(std::distance(KEYWORDS.find(str), KEYWORDS.end())),
            ""
        );

    int quotes[3] = { 0, 0, 0 };
    // single, double, backticks

    for (const auto&c : str) {
        switch (c) {
            case ' ':
                if (keywords$contains(str_buf))
                    return handle$keyword(
                        static_cast<KEYWORD>(std::distance(KEYWORDS.find(str_buf), KEYWORDS.end())),
                        inp + str_buf.size() + 1
                    );
                else
                    argv[0] = const_cast<char*>(str_buf.c_str());
            break;
            case '"':
            quotes[1] += !(quotes[0] % 2 || quotes[2] % 2);
            break;
            case '\'':
            quotes[0] += !(quotes[1] % 2 || quotes[2] % 2);
            break;
            case '`':
            quotes[2] += !(quotes[0] % 2 || quotes[1] % 2);
            break;
        }

        str_buf += c;
    }
}
}
