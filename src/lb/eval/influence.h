#pragma once

#include <array>

#include "lb/common.h"
#include "lb/position.h"
#include "lb/types.h"

namespace lb::eval {

  auto calcInfluence(Color color, const Position &position) -> std::array<Bitboard, 4>;

} // namespace lb::eval
