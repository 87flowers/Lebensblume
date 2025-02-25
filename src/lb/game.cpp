#include "lb/game.h"

#include <print>

namespace lb {

  auto Game::reset() -> void { setPositionStartpos(); }

  auto Game::checkRepetition() const -> RepetitionType {
    const auto current_hash = hash();

    usize num_clones = 0;
    usize first_clone_ply = 0;
    usize i = position().activeColor() != position_stack.front().activeColor();
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
    // position sfen 2k6/r8/9/9/9/9/9/9/2K6 b - 1 moves 7i6i 9b6b 6i7i 6b7b 7i6i 7b6b 6i7i 6b7b 7i6i 7b6b 6i7i 6b7b 7i6i 7b6b
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
    if (hash_stack.size() < 6)
      return RepetitionType::none;

    const auto current_hash = hash();

    constexpr usize max_lookback = 16;
    const isize start = static_cast<isize>(hash_stack.size() <= max_lookback ? 0 : hash_stack.size() - max_lookback - 1);
    for (isize i = static_cast<isize>(hash_stack.size() - 5); i >= start; i -= 2) {
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

  auto Game::printKifu() const -> void {
    if (position_stack.front() == Board::startpos) {
      std::print("手合割：平手\n");
    } else {
      position_stack.front().printKifu();
    }
    if (move_stack.size() > 0) {
      std::optional<Square> last_destination = std::nullopt;
      std::print("手数----指手---------消費時間--\n");
      for (usize i = 0; i < move_stack.size(); i++) {
        const Move m = move_stack[i];
        const PieceType ptype = m.drop() ? m.ptype() : position_stack[i].getPlace(m.from()).ptype();
        const std::string_view flag = m.drop() ? "打" : m.promo() ? "成" : "";
        const std::string dest_str = last_destination == m.to() ? "同　" : m.to().toJaString();
        const std::string src_str = m.drop() ? "" : '(' + m.from().toEnString() + ')';
        std::print("{:>3}  {}{}{}{}\n", i + 1, dest_str, ptype.toJaString(), flag, src_str);
        last_destination = m.to();
      }
    }
  }

} // namespace lb
