#include "lb/usi.h"

#include <cstdio>
#include <optional>
#include <print>

#include "lb/bench.h"
#include "lb/common.h"
#include "lb/game.h"
#include "lb/perft.h"
#include "lb/search.h"
#include "lb/util/tokenizer.h"

#define xstr(s) str(s)
#define str(s) #s

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

  static auto usiParseGo(Game &game, Tokenizer &it, time::TimePoint start_time) -> void {
    const auto read_int = [&it] {
      const std::string_view value_str = it.next();
      const auto value = parseInt(value_str);
      if (!value)
        printUnrecognizedToken("go", value_str);
      return value.and_then([](i64 x) -> std::optional<i64> { return std::max<i64>(0, x); });
    };

    std::string_view part = it.next();

    if (part == "wtime" || part == "btime" || part == "winc" || part == "binc" || part == "byoyomi") {
      search::TimeSettings ts{};
      while (!part.empty()) {
        const auto value = read_int();
        if (!value)
          return;

        if (part == "wtime") {
          ts.wtime = time::Milliseconds{*value};
        } else if (part == "btime") {
          ts.btime = time::Milliseconds{*value};
        } else if (part == "binc") {
          ts.binc = time::Milliseconds{*value};
        } else if (part == "winc") {
          ts.winc = time::Milliseconds{*value};
        } else if (part == "byoyomi") {
          ts.byoyomi = time::Milliseconds{*value};
        } else {
          printUnrecognizedToken("go", part);
        }

        part = it.next();
      }
      search::usiTime(game, std::move(ts), std::move(start_time));
    } else if (part == "mate") {
      std::print("checkmate notimplemented\n");
    } else if (part == "infinite") {
      std::print("todo\n");
    } else if (part == "depth") {
      if (const auto value = read_int())
        search::usiDepth(game, static_cast<i32>(std::min(*value, static_cast<i64>(max_search_ply))), std::move(start_time));
    } else if (part == "nodes") {
      if (const auto value = read_int())
        search::usiNode(game, *value, std::move(start_time));
    } else if (part.empty()) {
      search::usiDepth(game, static_cast<i32>(max_search_ply), std::move(start_time));
    } else if (part == "perft") {
      if (const auto depth = read_int(); depth && *depth > 0)
        perft::run(game.position(), static_cast<usize>(*depth));
    } else {
      printUnrecognizedToken("go", part);
    }
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

  static auto usiParseNewGame(Game &game, Tokenizer &it) -> void {
    // Do nothing
  }

  static auto usiParseIsReady(Game &game, Tokenizer &it) -> void {
    game.reset();
    std::print("readyok\n");
  }

  static auto usiParseUsi(Game &game, Tokenizer &it) -> void {
    std::print("id name Lebensblume " LB_VERSION "\n"
               "id author 87 (87flowers.com)\n"
               "usiok\n");
  }

  static auto usiParsePing(Game &game, Tokenizer &it) -> void { std::print("pong\n"); }

  static auto usiParsePerft(Game &game, Tokenizer &it) -> void {
    const std::string_view depth_str = it.rest().empty() ? "1" : it.next();
    const auto depth = parseInt(depth_str);
    if (!depth || *depth < 0)
      return printUnrecognizedToken("perft", depth_str);
    perft::run(game.position(), static_cast<usize>(*depth));
  }

  static auto usiParseUndo(Game &game, Tokenizer &it) -> void {
    const std::string_view count_str = it.rest().empty() ? "1" : it.next();
    const auto count = parseInt(count_str);
    if (!count || *count < 0)
      return printUnrecognizedToken("undo", count_str);
    for (i64 i = 0; i < count; i++)
      game.unmove();
  }

  static auto usiParseDisplay(Game &game, Tokenizer &it) -> void {
    game.position().printKifu();

    std::print("sfen: {}\n", game.position());

    std::print("checkers:");
    if (game.position().getCheckers().empty()) {
      std::print(" -");
    } else {
      for (Square sq : game.position().getCheckers())
        std::print(" {}", sq);
    }
    std::print("\n");
    std::print("pinned:");
    if (game.position().getPinned().empty()) {
      std::print(" -");
    } else {
      for (Square sq : game.position().getPinned())
        std::print(" {}", sq);
    }
    std::print("\n");
  }

  static auto usiParseCompiler(Game &game, Tokenizer &it) -> void {
    // clang-format off
    std::print("compiler build-datetime " __DATE__ " " __TIME__ "\n"
#if defined(__VERSION__)
               "compiler version " __VERSION__ "\n"
#endif
#if defined(__clang__)
               "compiler family clang++ version " xstr(__clang_major__) "." xstr(__clang_minor__) "." xstr(__clang_patchlevel__) "\n"
#elif defined(__GNUC__)
               "compiler family g++ version " xstr(__GNUC__) "." xstr(__GNUC_MINOR__) "." xstr(__GNUC_PATCHLEVEL__) "\n"
#elif defined(_MSC_VER)
               "compiler family msvc version " xstr(_MSC_FULL_VER) " " xstr(_MSC_BUILD) "\n"
#else
               "compiler family unknown\n"
#endif
#if LB_NO_ASSERTS
               "compiler assertions disabled\n"
#else
               "compiler assertions enabled\n"
#endif
               "compilerok\n");
    // clang-format on
  }

  auto usiParseCommand(Game &game, std::string_view line) -> void {
    const time::TimePoint start_time = time::Clock::now();

    Tokenizer it{line};
    const std::string_view cmd = it.next();

    if (cmd == "go") {
      usiParseGo(game, it, std::move(start_time));
    } else if (cmd == "position") {
      usiParsePosition(game, it);
    } else if (cmd == "usinewgame") {
      usiParseNewGame(game, it);
    } else if (cmd == "isready") {
      usiParseIsReady(game, it);
    } else if (cmd == "usi") {
      usiParseUsi(game, it);
    } else if (cmd == "gameover") {
      // ignore
    } else if (cmd == "ping") {
      usiParsePing(game, it);
    } else if (cmd == "bench") {
      bench::run();
    } else if (cmd == "perft") {
      usiParsePerft(game, it);
    } else if (cmd == "moves" || cmd == "move") {
      usiParseMoves(game, it);
    } else if (cmd == "undo") {
      usiParseUndo(game, it);
    } else if (cmd == "d") {
      usiParseDisplay(game, it);
    } else if (cmd == "kifu") {
      game.printKifu();
    } else if (cmd == "compiler") {
      usiParseCompiler(game, it);
    } else if (cmd == "quit") {
      std::exit(0);
    } else {
      printProtocolError(cmd, "unknown command");
    }
  }

} // namespace lb
