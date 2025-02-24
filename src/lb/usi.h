#pragma once

#include "lb/types.h"

namespace lb {

  struct Game;

  auto usiParseCommand(Game &game, std::string_view cmd) -> void;

} // namespace lb
