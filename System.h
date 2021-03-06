#pragma once

namespace BShell {
std::string get$cwd();
std::string get$home();
std::string get$username();
std::string get$hostname();
std::string get$pname(pid_t);
std::string get$pname(const std::shared_ptr<Expression>&);
std::string get$executable_path(std::string);
}
