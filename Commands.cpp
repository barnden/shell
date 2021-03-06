#include <iostream>
#include <memory>

#include <string.h>
#include <unistd.h>

#include "Commands.h"
#include "Parser.h"
#include "Interpreter.h"

namespace BShell {
void command$cd(const std::shared_ptr<Expression>& expr) {
    auto args = std::vector<std::string> {};
    auto argv = std::vector<char*> {};

    auto sticky = false;

    for (auto& child : expr->children)
        handle$argv_strings(args, sticky, child->token);

    std::transform(
        args.begin(), args.end(),
        std::back_inserter(argv),
        [](const std::string& str) { return const_cast<char*>(str.c_str()); }
    );

    auto* dir = argv.size() ? argv[0] : nullptr;
    auto cwd = get$cwd();
    auto stat = 0;

    if (!dir)
        stat = chdir(get$home().c_str());
    else if (dir[0] == '~') {
        auto home = get$home();
        auto path = std::make_unique<char>(strlen(dir) + home.size());

        strcpy(path.get(), home.c_str());
        strcpy(path.get() + home.size(), dir + 1);

        stat = chdir(path.get());
    } else if (strcmp(dir, "-") == 0) {
        if (g_prev_wd.size())
            stat = chdir(g_prev_wd.c_str());
        else
            std::cerr << "g_prev_wd not set\n";
    } else stat = chdir(dir);

    // Don't change previous directory on error.
    if (stat < 0)
        std::cerr << "Failed to change directory\n";
    else
        g_prev_wd = cwd;
}

void command$set_env(const std::shared_ptr<Expression>& expr) {
    if (expr->children.size() < 2) {
        std::cerr << "Syntax error '='.\n";
        return;
    }

    auto key = expr->children[0]->token.content;
    auto val = expr->children[1]->token.content;

    if (setenv(key.c_str(), val.c_str(), 1) < 0)
        std::cerr << "Failed to set environment variable " << key << ", errno " << errno << '\n';
}
}
