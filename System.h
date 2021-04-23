#pragma once

#include "Parser.h"

namespace BShell {
std::string get$cwd();
std::string get$home();
std::string get$username();
std::string get$hostname();
std::string get$pname(pid_t const&);
std::string get$pname(std::shared_ptr<Expression> const&);
std::string get$executable_path(std::string const&);
}
