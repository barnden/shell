#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Commands.h"
#include "Interpreter.h"
#include "System.h"
#include "Terminal.h"

namespace BShell {
std::vector<Process> g_processes;
int g_exit_fg = 0, g_exit_bg = 0;

std::string get$eval(Token const& token) {
    // Recursively tokenize and parse eval string until we get something
    auto tokens = Tokenizer(token.content.c_str()).tokens();
    auto asts = Parser(std::move(tokens)).asts();
    auto eval_io = Pipe {};
    auto str = std::string {};

    if (pipe(eval_io.fd) < 0) {
        perror("pipe()");
        exit(1);
    }

    for (auto&& ast : asts) {
        handle$ast(std::move(ast), [&] {
            dup2(eval_io.fd[1], STDOUT_FILENO);
            std::cout.flush();

            close(eval_io.fd[0]);
            close(eval_io.fd[1]);
        });
    }

    write(eval_io.fd[1], NULL, 1);
    close(eval_io.fd[1]);

    auto buf = std::make_unique<char[]>(BUFSIZ);
    auto fmem = fdopen(eval_io.fd[0], "r");

    while (fgets(buf.get(), BUFSIZ, fmem))
        str += std::string { buf.get() };

    close(eval_io.fd[0]);

    return str;
}

void handle$argv_strings(std::vector<std::string>& argv, bool& sticky, Token const& token) {
    auto str = token.content;

    if (token.type == Eval)
        str = get$eval(token);

    if (sticky || token.type & StickyLeft) {
        sticky = false;

        str = argv.back() + str;
        argv.pop_back();
    }

    if (token.type & StickyRight)
        sticky = true;

    // Remove unprintable control characters like SOH, STX, ETX, etc.
    str.erase(std::remove_if(str.begin(), str.end(), [=](int c) { return !std::isprint(c); }),
              str.end());

    argv.push_back(str);
}

void handle$keyword(std::shared_ptr<Expression> const& expr) {
    auto kw = expr->token.content;
    auto index = std::distance(g_keywords.find(kw), g_keywords.end());

    // TODO: Maybe use an enum for more readability
    switch (index) {
    case 1: // export
        break;
    case 2: // cd
        command$cd(expr);
        break;
    case 3: // jobs
        break;
    }
}

void handle$io_redirect(int fd, std::shared_ptr<Expression> const& redir) {
    if (redir == nullptr)
        return;

    auto filename = redir->token.content;

    if (!(redir->token.type & (String | StickyLeft)))
        filename = get$eval(redir->token);

    auto file = open(filename.c_str(), O_CREAT | O_WRONLY, 0644);

    if (file < 0)
        perror("open()");

    if (dup2(file, fd) < 0) {
        perror("dup2()");
        exit(1);
    }

    std::cout.flush();

    close(file);
}

std::vector<std::string> handle$argv(std::shared_ptr<Expression> const& expr) {
    auto argv = std::vector<std::string> { expr->token.content };
    auto sticky = false;

    for (auto const& child : expr->children) {
        switch (child->token.type) {
        case StickyRight:
        case Eval:
        case String:
        case StickyLeft:
            handle$argv_strings(argv, sticky, child->token);
            break;
        case RedirectIn:
            handle$io_redirect(STDIN_FILENO, child->children[0]);
            break;
        case RedirectOut:
            handle$io_redirect(STDOUT_FILENO, child->children[0]);
            break;
        }
    }

    return argv;
}

template <typename T> Process execute(std::shared_ptr<Expression> const& expr, T&& child_hook) {
    auto pid = fork();

    if (pid < 0) {
        perror("fork()");
        exit(1);
    } else if (!pid) {
        auto args = handle$argv(expr);
        auto argv = std::vector<char*> {};

        std::transform(args.begin(), args.end(), std::back_inserter(argv),
                       [](std::string const& str) { return const_cast<char*>(str.c_str()); });

        argv.push_back(NULL);

        child_hook();

        execvp(argv[0], &argv[0]);

        perror("execvp()");
        exit(1);
    }

    return Process { pid, get$pname(expr) };
}

template <typename T> void handle$executable(std::shared_ptr<Expression> const& expr, T&& hook) {
    auto proc = execute(expr, hook);

    waitpid(proc.pid, &g_exit_fg, 0);
}

void handle$background(std::shared_ptr<Expression> const& expr) {
    auto proc = execute(expr->children[0], [=] {});

    std::cout << '[' << g_processes.size() + 1 << "] " << proc.pid << '\n';

    waitpid(proc.pid, &g_exit_bg, WNOHANG);
    g_processes.push_back(Process { proc.pid, proc.name });
}

void handle$sequential(std::shared_ptr<Expression> const& expr) {
    for (auto const& child : expr->children) {
        auto proc = Process {};

        proc = execute(child, [&]() -> void {});

        waitpid(proc.pid, &g_exit_fg, 0);

        if (expr->token.type == SequentialIf && g_exit_fg != 0)
            break;
    }
}

template <typename T> void handle$pipe(std::shared_ptr<Expression> const& expr, T&& last_hook) {
    auto last_io = Pipe {};

    for (auto const& child : expr->children) {
        // TODO: Maybe figure out something other than this callback structure.
        auto proc = Process {};
        auto proc_io = Pipe {};

        if (pipe(proc_io.fd) < 0) {
            perror("pipe()");
            exit(1);
        }

        proc = execute(child, [&]() -> void {
            // This entire lambda function executes within the child process.
            if (child != expr->children.front()) {
                dup2(last_io.fd[0], STDIN_FILENO);

                close(last_io.fd[0]);
                close(last_io.fd[1]);
            }

            if (child != expr->children.back())
                dup2(proc_io.fd[1], STDOUT_FILENO);
            else
                last_hook();

            close(proc_io.fd[0]);
            close(proc_io.fd[1]);
        });

        if (child != expr->children.front()) {
            // Close the file descriptor in the parent process.
            close(last_io.fd[0]);
            close(last_io.fd[1]);
        }

        last_io = proc_io;

        if (waitpid(-1, &g_exit_fg, 0) < 0) {
            perror("waitpid()");
            exit(1);
        }
    }
}

template <typename T> void handle$ast(std::shared_ptr<Expression>&& ast, T&& hook) {
    switch (ast->token.type) {
    case Key:
        return handle$keyword(ast);
    case Executable:
        return handle$executable(ast, hook);
    case Background:
        return handle$background(ast);
    case RedirectPipe:
        return handle$pipe(ast, hook);
    case SequentialIf:
    case Sequential:
        return handle$sequential(ast);
    case Equal:
        return command$set_env(ast);
    default:
        std::cerr << "Bad token type passed to handle$ast\n";
    }
}

void handle$ast(std::shared_ptr<Expression>&& ast) {
#if DEBUG_AST
    std::cout << "--{AST Begin}--\n";
    BShell::ast$print(ast);
    std::cout << "--{AST End}--\n";
#endif

    handle$ast(std::move(ast), [=]() {});
}

void erase_dead_children() {
    g_processes.erase(std::remove_if(g_processes.begin(), g_processes.end(),
                                     [=](Process const& proc) -> bool {
                                         return waitpid(proc.pid, &g_exit_bg, WNOHANG) != 0;
                                     }),
                      g_processes.end());
}
}
