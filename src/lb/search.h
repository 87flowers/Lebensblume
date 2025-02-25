#pragma once

#include "lb/types.h"

namespace lb {
  struct Game;
}

namespace lb::search {

  struct TimeSettings {
    time::Milliseconds wtime{};
    time::Milliseconds btime{};
    time::Milliseconds winc{};
    time::Milliseconds binc{};
    time::Milliseconds byoyomi{};
  };

  auto usiTime(Game &game, TimeSettings ts) -> void;
  auto usiDepth(Game &game, i64 depth) -> void;
  auto usiNode(Game &game, i64 nodes) -> void;

} // namespace lb::search
