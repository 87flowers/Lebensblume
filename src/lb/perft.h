#pragma once

#include "lb/types.h"

namespace lb {
  struct Position;
} // namespace lb

namespace lb::perft {

  auto value(const Position &position, usize depth) -> usize;
  auto run(const Position &position, usize depth) -> void;

} // namespace lb::perft
