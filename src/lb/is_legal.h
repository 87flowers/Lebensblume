#pragma once

#include "lb/board.h"
#include "lb/common.h"

namespace lb {

  auto isMoveLegal(const Board &position, Move move) -> bool;

}
