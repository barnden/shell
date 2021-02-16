#include <iostream>
#include <cstring>
#include <string>
#include <unordered_set>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "Shell.h"

namespace BShell {
std::string get$cwd() {
    // From getcwd(3) it says get_current_dir_name() will malloc a
    // string large enough to fit the current working dir.
    // While getcwd() requires us to create our own string buffer.
    auto*buf = get_current_dir_name();
    auto cwd = std::string(buf);

    free(buf);

    return cwd;
}

std::string get$home() {
    // Use getenv "HOME" otherwise, fallback to getpwuid(3)
    // and getuid(3) to get the user's home directory.

    auto*buf = getenv("HOME");

    if (buf == nullptr)
        buf = getpwuid(getuid())->pw_dir;

    return std::string(buf);
}

std::string get$username() {
    // getlogin(3)
    // Suggests that we use getenv to get "LOGNAME", otherwise
    // fallback to getlogin(), then getlogin_r()

    auto*buf = getenv("LOGNAME");
    auto username = std::string {};
    auto del = false;

    if (buf == nullptr && (buf = getlogin()) == nullptr){
        // From useradd(8), usernames can be at most 32 chars long.
        buf = new char [32];

        getlogin_r(buf, 32);

        del = true;
    }

    username = std::string(buf);

    if (del)
        delete[] buf;

    return username;
}

std::string get$hostname() {
    auto buf = getenv("HOSTNAME");
    auto hostname = std::string {};

    if (buf != nullptr) hostname = std::string(buf);
    else {
        // hostname(7) states that the maximum hostname is 253 chars.
        buf = new char [253];
        gethostname(buf, 253);

        hostname = std::string(buf);
        delete[] buf;
    }

    return hostname;
}

void handle$cd(const char*argc) {
    if (!strlen(argc)) {
        if (chdir(get$home().c_str()) == -1)
            std::cerr << "Failed to change directory\n";
    } else if (*argc == '~') {
        auto home = get$home();
        auto*path = new char [strlen(argc) + home.size()];

        strcpy(path, home.c_str());
        strcpy(path + home.size(), argc + 1);

        if (chdir(path) == -1)
            std::cerr << "Failed to change directory\n";

        delete[] path;
    } else if (chdir(argc) == -1)
        std::cerr << "Failed to change directory\n";
}

void handle$keyword(KEYWORD kw, const char*argc) {
    switch (kw) {
        case EXPORT:
        // TODO: Implement EXPORT.
        break;
        case CD: return handle$cd(argc);
        default:

        std::cout
            << "Received unknown keyword ("
            << static_cast<uint32_t>(kw)
            << ") with argc of \""
            << argc
            << "\"\n";
        break;
    }
}
}
