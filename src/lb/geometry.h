#pragma once

#include "lb/common.h"

namespace lb::geometry {

  auto rayBetween(Square a, Square b) -> Bitboard;
  auto rayInfinite(Square a, Square b) -> Bitboard;

  auto manhattanRing(Square center, usize distance) -> Bitboard;

} // namespace lb::geometry
