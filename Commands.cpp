#include <iostream>
#include <memory>

#include <string.h>
#include <unistd.h>

#include "Commands.h"
#include "Interpreter.h"
#include "Parser.h"
#include "System.h"

namespace BShell {
std::string g_prev_wd = "";

void command$cd(std::shared_ptr<Expression> const& expr) {
    auto args = std::vector<std::string> {};
    auto argv = std::vector<char*> {};

    auto sticky = false;

    for (auto const& child : expr->children)
        handle$argv_strings(args, sticky, child->token);

    std::transform(args.begin(), args.end(), std::back_inserter(argv),
                   [](std::string const& str) { return const_cast<char*>(str.c_str()); });

    auto* dir = argv.size() ? argv[0] : nullptr;
    auto cwd = get$cwd();
    auto stat = 0;

    if (!dir) {
        stat = chdir(get$home().c_str());
    } else if (dir[0] == '~') {
        auto home = get$home();
        auto path = std::make_unique<char>(strlen(dir) + home.size());

        strcpy(path.get(), home.c_str());
        strcpy(path.get() + home.size(), dir + 1);

        stat = chdir(path.get());
    } else if (strcmp(dir, "-") == 0) {
        if (!g_prev_wd.size()) {
            std::cerr << "g_prev_wd not set\n";
            return;
        }

        stat = chdir(g_prev_wd.c_str());
    } else {
        stat = chdir(dir);
    }

    // Don't change previous directory on error.
    if (stat < 0) {
        perror("chdir()");
        return;
    }

    g_prev_wd = cwd;
}

void command$set_env(std::shared_ptr<Expression> const& expr) {
    if (expr->children.size() < 2) {
        std::cerr << "Syntax error '='.\n";
        return;
    }

    auto key = expr->children[0]->token.content;
    auto val = expr->children[1]->token.content;

    if (setenv(key.c_str(), val.c_str(), 1) < 0)
        perror("setenv()");
}
}
