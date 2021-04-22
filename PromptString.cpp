#include <string>

#include "Interpreter.h"
#include "PromptString.h"
#include "System.h"

using namespace BShell;

namespace BShell {
std::string PS1 = "[B] \\u@\\h \\w\\$ ";

std::string parse$PS_format_string(char const& c) {
    switch (c) {
    case 'u':
        return get$username();
    case 'h':
        return get$hostname();
    case '$':
        return get$username() == "root" ? "#" : "$";
    case 'w': {
        auto cwd = get$cwd();

        if (cwd == "/")
            return cwd;
        else if (cwd == get$home())
            return "~";

        return cwd.substr(cwd.find_last_of('/') + 1);
    }
    default:
        return "";
    }
}

std::string parse$PS(std::string const& PS) {
    auto parsed = std::string {};
    auto gobble = false;

    for (auto const& c : PS) {
        if (gobble) {
            parsed += parse$PS_format_string(c);
            gobble = false;
        } else if (c == '\\') {
            gobble = true;
        } else {
            parsed += c;
        }
    }

    return parsed;
}

std::string const get$PS1() { return parse$PS(PS1); }
}
