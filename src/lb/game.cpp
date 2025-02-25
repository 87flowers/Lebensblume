#include "lb/game.h"

namespace lb {

  auto Game::checkRepetition() const -> RepetitionType {
    const auto current_hash = hash();

    usize num_clones = 0;
    usize first_clone_ply = 0;
    usize i = position().activeColor() == position_stack.front().activeColor();
    for (; i < hash_stack.size(); i += 2) {
      if (hash_stack[i] == current_hash) {
        num_clones++;
        if (num_clones == 1)
          first_clone_ply = i;
      }
    }

    if (num_clones < 4)
      return RepetitionType::none;

    // Example:
    // position sfen 2k6/r8/9/9/9/9/9/9/2K6 b - 1 moves 7i6i 9b6b 6i7i 6b7b 7i6i 7b6b 7i6i 7b6b 7i6i 7b6b 7i6i 7b6b
    // [sente move] [gote checked sente (clone)] [sente move] [gote checked sente]
    // [sente move] [gote checked sente (clone)] [sente move] [gote checked sente]
    // [sente move] [gote checked sente (clone)] [sente move] [gote checked sente]
    // [sente move] [gote checked sente (clone)]
    // Here:
    // * hash_stack.size() == 14
    // * first_clone_ply == 1
    // * num_clones == 4
    // * position().non_check_clock[0] = 7
    // * distance == 13
    // * non_check_distance == 14

    const usize distance = hash_stack.size() - first_clone_ply;
    const usize non_check_distance = position().nonCheckClock(position().activeColor()) * 2;
    if (non_check_distance > distance)
      return RepetitionType::illegal_perpetual;

    return RepetitionType::sennichite;
  }

  auto Game::checkMaybeRepetition() const -> RepetitionType {
    if (hash_stack.size() <= 4)
      return RepetitionType::none;

    const auto current_hash = hash();

    constexpr usize max_lookback = 16;
    const usize start = hash_stack.size() <= max_lookback ? 0 : hash_stack.size() - max_lookback - 1;
    for (isize i = static_cast<isize>(hash_stack.size() - 4); i >= start; i -= 2) {
      if (hash_stack[i] == current_hash) {
        const usize distance = hash_stack.size() - i;
        const usize non_check_distance = position().nonCheckClock(position().activeColor()) * 2;
        if (non_check_distance > distance)
          return RepetitionType::illegal_perpetual;
        return RepetitionType::sennichite;
      }
    }

    return RepetitionType::none;
  }

} // namespace lb
