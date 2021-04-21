#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include "Parser.h"
#include "System.h"
#include "Tokenizer.h"

namespace BShell {
std::string get$cwd() {
    // From getcwd(3) it says get_current_dir_name() will malloc a
    // string large enough to fit the current working dir.
    // While getcwd() requires us to create our own string buffer.

    auto* buf = get_current_dir_name();
    auto cwd = std::string(buf);

    free(buf);

    return cwd;
}

std::string get$home() {
    // Use getenv "HOME" otherwise, fallback to getpwuid(3)
    // and getuid(3) to get the user's home directory.

    auto* buf = getenv("HOME");

    if (buf == nullptr)
        buf = getpwuid(getuid())->pw_dir;

    return std::string { buf };
}

std::string get$username() {
    // getlogin(3)
    // Suggests that we use getenv to get "LOGNAME", otherwise
    // fallback to getlogin(), then getlogin_r()

    auto* buf = getenv("LOGNAME");
    auto uptr = std::unique_ptr<char[]> {};

    if (buf == nullptr && (buf = getlogin()) == nullptr) {
        // From useradd(8), usernames can be at most 32 chars long.
        uptr = std::make_unique<char[]>(32);
        buf = uptr.get();

        if (getlogin_r(buf, 32) != 0) {
            std::cerr << "getlogin_r()\n";
            exit(1);
        }
    }

    return std::string { buf };
}

std::string get$hostname() {
    auto* buf = getenv("HOSTNAME");
    auto uptr = std::unique_ptr<char[]> {};

    if (buf == nullptr) {
        // hostname(7) states that the maximum hostname is 253 chars.
        uptr = std::make_unique<char[]>(253);
        buf = uptr.get();

        if (gethostname(buf, 253) != 0) {
            std::cerr << "gethostname()\n";
            exit(1);
        }
    }

    return std::string { buf };
}

std::string get$pname(pid_t pid) {
    auto proc = std::ifstream("/proc/" + std::to_string(pid) + "/cmdline");

    return std::string { std::istreambuf_iterator { proc.rdbuf() }, {} };
}

std::string get$pname(std::shared_ptr<Expression> const& expr) {
    auto pname = std::string {};

    if (expr->token.type != Executable)
        return pname;

    pname = expr->token.content;

    if (expr->children.size())
        for (auto const& c : expr->children)
            pname += ' ' + c->token.content;

    return pname;
}

std::string get$executable_path(std::string cmd) {
    // Scans the path for executables that match the given string

    auto env_path = std::string(getenv("PATH"));
    auto path = std::string {};

    auto i = size_t {}, j = env_path.find(':');

    for (; j != std::string::npos; j = env_path.find(':', i = j + 1)) {
        auto tmp = env_path.substr(i, j - i) + "/" + cmd;

        if (access(tmp.c_str(), X_OK) != -1) {
            path = tmp;
            break;
        }
    }

    return path;
}
}
