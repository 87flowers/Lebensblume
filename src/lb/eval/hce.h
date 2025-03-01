#pragma once

#include "lb/types.h"

namespace lb {
  struct Board;
}

namespace lb::eval {

  auto hce(const Board &position) -> i32;

} // namespace lb::eval
