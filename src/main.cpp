#include <print>

#include "lb/board.h"
#include "lb/perft.h"

auto main() -> int {
  std::print("# Lebensblume {}\n", LB_VERSION);
#if LB_NO_ASSERTS
  std::print("# Assertions disabled\n");
#endif
  lb::perft::run(lb::Board::parse("l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w RGgsn5p 1").value(), 5);
  return 0;
}
