#pragma once

#include "Parser.h"

namespace BShell {
    void command$cd(const std::shared_ptr<Expression>&);
    void command$set_env(const std::shared_ptr<Expression>&);
}