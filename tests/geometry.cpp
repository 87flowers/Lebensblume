#include <print>

#include "lb/common.h"
#include "lb/geometry.h"
#include "lb/types.h"
#include "lb/util/assert.h"

using namespace lb;

auto manhattanRing() -> void {
  {
    const Square center = Square::parse("7g").value();
    lb_assert(geometry::manhattanRing(center, 0).raw == 0x1000000000000000_u128);
    lb_assert(geometry::manhattanRing(center, 1).raw == 0x70281C000000000000_u128);
    lb_assert(geometry::manhattanRing(center, 2).raw == 0x1F08844221F0000000000_u128);
    lb_assert(geometry::manhattanRing(center, 3).raw == 0x8040201008FC0000000_u128);
    lb_assert(geometry::manhattanRing(center, 4).raw == 0x4020100804027F00000_u128);
    lb_assert(geometry::manhattanRing(center, 5).raw == 0x20100804020100BFC00_u128);
    lb_assert(geometry::manhattanRing(center, 6).raw == 0x10080402010080403FF_u128);
    lb_assert(geometry::manhattanRing(center, 7).empty());
    lb_assert(geometry::manhattanRing(center, 8).empty());
    lb_assert(geometry::manhattanRing(center, 9).empty());
  }
  {
    const Square center = Square::parse("1f").value();
    lb_assert(geometry::manhattanRing(center, 0).raw == 0x200000000000_u128);
    lb_assert(geometry::manhattanRing(center, 1).raw == 0xC0403000000000_u128);
    lb_assert(geometry::manhattanRing(center, 2).raw == 0x38100804038000000_u128);
    lb_assert(geometry::manhattanRing(center, 3).raw == 0xF0402010080403C0000_u128);
    lb_assert(geometry::manhattanRing(center, 4).raw == 0x10080402010080403E00_u128);
    lb_assert(geometry::manhattanRing(center, 5).raw == 0x2010080402010080403F_u128);
    lb_assert(geometry::manhattanRing(center, 6).raw == 0x40201008040201008040_u128);
    lb_assert(geometry::manhattanRing(center, 7).raw == 0x80402010080402010080_u128);
    lb_assert(geometry::manhattanRing(center, 8).raw == 0x100804020100804020100_u128);
    lb_assert(geometry::manhattanRing(center, 9).raw == 0);
  }
}

auto main() -> int {
  manhattanRing();
  return 0;
}
