#include <string>

#include "PromptString.h"
#include "Interpreter.h"

using namespace BShell;

namespace BShell {
std::string PS1 = "[B] \\u@\\h \\w\\$ ";

std::string parse$PS_format_string(const char&c) {
    switch (c) {
        case 'u': return get$username();
        case 'h': return get$hostname();
        case '$': return get$username() == "root" ? "#" : "$";
        case 'w': {
            auto cwd = get$cwd();
            if (cwd == "/") return cwd;
            else if (cwd == get$home()) return "~";

            return cwd.substr(cwd.find_last_of('/') + 1);
        }
        default: return "";
    }
}

std::string parse$PS(const std::string PS) {
    auto parsed = std::string {};
    auto gobble = false;

    for (const auto&c : PS) {
        if (gobble) {
            parsed += parse$PS_format_string(c);
            gobble = false;
        } else if (c == '\\') gobble = true;
        else parsed += c;
    }

    return parsed;
}

const std::string get$PS1() { return parse$PS(PS1); }
}
