#pragma once

#include "lb/types.h"

namespace lb {
  struct Board;
} // namespace lb

namespace lb::perft {

  auto value(const Board &board, usize depth) -> usize;
  auto run(const Board &board, usize depth) -> void;

} // namespace lb::perft
