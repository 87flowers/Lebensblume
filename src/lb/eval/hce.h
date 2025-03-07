#pragma once

#include "lb/types.h"

namespace lb {
  struct Position;
}

namespace lb::eval {

  auto hce(const Position &position) -> i32;

  auto printInfo(const Position &position) -> void;

} // namespace lb::eval
