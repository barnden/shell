#pragma once

#include "Parser.h"

namespace BShell {
void command$cd(std::shared_ptr<Expression> const&);
void command$set_env(std::shared_ptr<Expression> const&);
}