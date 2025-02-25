#include <cstdio>
#include <iostream>
#include <print>
#include <ranges>

#include "lb/game.h"
#include "lb/usi.h"

auto main(int argc, char *argv[]) -> int {
  std::print("# Lebensblume {}\n", LB_VERSION);
  std::fflush(stdout);

  lb::Game game;
  game.reset();

  if (argc > 1) {
    for (lb::usize i : std::views::iota(1, argc)) {
      lb::usiParseCommand(game, argv[i]);
    }
    return 0;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    lb::usiParseCommand(game, line);
  }
  return 0;
}
