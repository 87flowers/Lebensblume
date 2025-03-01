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

  auto usiTime(Game &game, TimeSettings ts, time::TimePoint start_time) -> void;
  auto usiDepth(Game &game, i32 depth, time::TimePoint start_time) -> void;
  auto usiNode(Game &game, u64 nodes, time::TimePoint start_time) -> void;

  auto bench(Game &game, i32 depth, time::TimePoint start_time) -> u64;

} // namespace lb::search
