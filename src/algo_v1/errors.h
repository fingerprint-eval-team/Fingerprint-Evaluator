#pragma once
#include "api.h"
#include <stdexcept>
#include <string>

struct AlgorithmError : std::runtime_error {
    int code;
    AlgorithmError(int value, const std::string& message)
        : std::runtime_error(message), code(value) {}
};
