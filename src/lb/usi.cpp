#include "lb/usi.h"

#include <cstdio>
#include <optional>
#include <print>

#include "lb/game.h"
#include "lb/perft.h"
#include "lb/util/tokenizer.h"

namespace lb {

  static auto parseInt(std::string_view str) -> std::optional<i64> {
    bool negate = false;
    i64 result = 0;
    usize i = 0;

    if (str.size() == 0)
      return std::nullopt;

    if (str[0] == '-') {
      if (str.size() == 1)
        return std::nullopt;
      negate = true;
      i = 1;
    }

    for (; i < str.size(); i++) {
      if (str[i] < '0' && str[i] > '9')
        return std::nullopt;
      result = result * 10 + (str[i] - '0');
    }

    return negate ? -result : result;
  }

  template <typename... Args> static auto printProtocolError(std::string_view cmd, std::format_string<Args...> fmt, Args &&...args) -> void {
    std::print("error ({}): ", cmd);
    std::print(fmt, std::forward<Args>(args)...);
    std::print("\n");
    std::fflush(stdout);
  }

  static auto printUnrecognizedToken(std::string_view cmd, std::string_view token) -> void {
    printProtocolError(cmd, "unrecognised token `{}`", token);
  }

  static auto printIllegalMove(std::string_view move) -> void { printProtocolError("illegal move", "{}", move); }

  static auto expectToken(std::string_view cmd, Tokenizer &it, std::string_view token) -> bool {
    const std::string_view next = it.next();
    if (next == token)
      return true;
    if (!next.empty())
      printUnrecognizedToken(cmd, next);
    return false;
  }

  static auto usiParseMoves(Game &game, Tokenizer &it) -> void {
    while (true) {
      const std::string_view move_str = it.next();
      if (move_str.empty())
        break;
      const auto m = Move::parse(move_str);
      if (!m)
        return printIllegalMove(move_str);
      game.move(m.value());
    }
  }

  static auto usiParsePosition(Game &game, Tokenizer &it) -> void {
    const std::string_view pos_type = it.next();
    if (pos_type.empty())
      return printProtocolError("position", "no position provided");

    if (pos_type == "startpos") {
      game.setPositionStartpos();
    } else if (pos_type == "sfen") {
      const std::string_view board_str = it.next();
      const std::string_view color_str = it.next();
      const std::string_view hand_str = it.next();
      const std::string_view ply_str = it.next();
      const auto pos = Board::parse(board_str, color_str, hand_str, ply_str);
      if (!pos)
        return printProtocolError("position", "invalid sfen provided: {}", pos.error());
      game.setPosition(pos.value());
    } else {
      return printUnrecognizedToken("position", pos_type);
    }

    if (expectToken("position", it, "moves")) {
      usiParseMoves(game, it);
    }
  }

  static auto usiParsePerft(Game &game, Tokenizer &it) -> void {
    const std::string_view depth_str = it.rest().empty() ? "1" : it.next();
    const auto depth = parseInt(depth_str);
    if (!depth || *depth < 0)
      return printUnrecognizedToken("perft", depth_str);
    perft::run(game.position(), static_cast<usize>(*depth));
  }

  auto usiParseCommand(Game &game, std::string_view line) -> void {
    Tokenizer it{line};
    const std::string_view cmd = it.next();

    if (cmd == "position") {
      usiParsePosition(game, it);
    } else if (cmd == "perft") {
      usiParsePerft(game, it);
    } else if (cmd == "d") {
      game.position().printKifu();
    } else if (cmd == "kifu") {
      game.printKifu();
    } else if (cmd == "quit") {
      std::exit(0);
    } else {
      printProtocolError(cmd, "unknown command");
    }
  }

} // namespace lb
