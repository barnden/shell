#include <iostream>

#include <string.h>
#include <unistd.h>

#include "Parser.h"
#include "Shell.h"

namespace BShell {
void command$cd(Expression* expr) {
    auto argv = std::vector<char*> {};
    auto sticky = false;

    for (auto& child : expr->children)
        handle$argv_strings(argv, sticky, child->token);

    auto* dir = argv.size() ? argv[0] : nullptr;;
    auto cwd = get$cwd();
    auto stat = 0;

    if (!dir)
        stat = chdir(get$home().c_str());
    else if (dir[0] == '~') {
        auto home = get$home();
        auto* path = new char [strlen(dir) + home.size()];

        strcpy(path, home.c_str());
        strcpy(path + home.size(), dir + 1);

        stat = chdir(path);

        delete[] path;
    } else if (strcmp(dir, "-") == 0) {
        if (g_prev_wd.size()) {
            stat = chdir(g_prev_wd.c_str());
            std::cout << g_prev_wd << '\n';
        } else std::cerr << "g_prev_wd not set\n";
    } else stat = chdir(dir);

    // Don't change previous directory on error.
    if (stat < 0)
        std::cerr << "Failed to change directory\n";
    else
        g_prev_wd = cwd;

    for (auto& arg : argv)
        delete[] arg;
}
}