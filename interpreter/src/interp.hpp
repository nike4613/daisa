#pragma once

#include "types.hpp"

#include <functional>

namespace daisa::interpreter {
  
  void interpret(
    Memory& mem,
    u16 startAddr,
    std::function<bool(Memory const&, RegisterPage const&)> pollInterrupt);

}