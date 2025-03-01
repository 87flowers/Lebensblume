#include "lb/search.h"

#include <algorithm>
#include <print>

#include "lb/eval/eval.h"
#include "lb/eval/hce.h"
#include "lb/game.h"
#include "lb/line.h"
#include "lb/movegen.h"
#include "lb/types.h"
#include "lb/util/assert.h"
#include "lb/util/defer.h"

namespace lb::search {

  namespace nodetype {
    struct Root;
    struct Pv;
    struct NonPv;

    struct Root {
      inline static constexpr bool is_root = true;
      inline static constexpr bool is_pv = true;
    };

    struct Pv {
      inline static constexpr bool is_root = false;
      inline static constexpr bool is_pv = true;
    };

    struct NonPv {
      inline static constexpr bool is_root = false;
      inline static constexpr bool is_pv = false;
    };
  } // namespace nodetype

  struct ControlBase {
  protected:
    time::TimePoint start_time;
    u64 nodes = 0;

    explicit ControlBase(time::TimePoint start_time) : start_time(start_time) {}

  public:
    auto elapsed() const -> time::Duration { return time::Clock::now() - start_time; }
    auto nodeCount() const -> u64 { return nodes; }

    auto nodeVisited() -> void { nodes++; }
  };

  struct TimeControl : public ControlBase {
  private:
    bool is_terminated = false;
    time::Duration soft_limit;
    time::Duration hard_limit;

  public:
    TimeControl(time::TimePoint start_time, time::Duration soft_limit, time::Duration hard_limit)
        : ControlBase(start_time), soft_limit(soft_limit), hard_limit(hard_limit) {}

    auto checkSoftTermination([[maybe_unused]] i32 depth) const -> bool { return soft_limit <= elapsed(); }
    auto checkHardTermination() -> void { is_terminated = hard_limit <= elapsed(); }

    auto isTerminated() const -> bool { return is_terminated; }
  };

  struct DepthControl : public ControlBase {
  private:
    i32 target_depth;

  public:
    DepthControl(time::TimePoint start_time, i32 target_depth) : ControlBase(start_time), target_depth(target_depth) {}

    auto checkSoftTermination(i32 depth) const -> bool { return depth >= target_depth; }
    auto checkHardTermination() -> void {}

    auto isTerminated() const -> bool { return false; }
  };

  struct NodeControl : public ControlBase {
  private:
    bool is_terminated = false;
    u64 hard_limit;

  public:
    NodeControl(time::TimePoint start_time, u64 hard_limit) : ControlBase(start_time), hard_limit(hard_limit) {}

    auto checkSoftTermination([[maybe_unused]] i32 depth) const -> bool { return false; }
    auto checkHardTermination() -> void { is_terminated = nodes >= hard_limit; }

    auto isTerminated() const -> bool { return is_terminated; }
  };

  template <typename NodeT, typename ControlT> auto search(Game &game, ControlT &ctrl, Line &pv, i32 alpha, i32 beta, i32 ply, i32 depth) -> i32 {
    const i32 initial_alpha = alpha;

    if (game.position().canDeclareEnteringKingsWin()) {
      pv.write(Move::win());
      return eval::mate(ply);
    }

    if (depth <= 0)
      return eval::hce(game.position());
    if (ply >= max_search_ply)
      return eval::hce(game.position());

    if constexpr (!NodeT::is_root) {
      ctrl.checkHardTermination();
      if (ctrl.isTerminated())
        return 0;
    }

    movegen::MoveList moves;
    movegen::generateMoves(moves, game.position());

    i32 best_score = eval::no_moves;
    usize legal_moves = 0;

    for (const Move m : moves) {
      game.move(m);
      lb_defer { game.unmove(); };
      ctrl.nodeVisited();

      i32 child_score = eval::no_moves;
      Line child_pv{};

      if (const RepetitionType rep_type = game.checkMaybeRepetition(); rep_type != RepetitionType::none) {
        switch (rep_type) {
        case RepetitionType::illegal_perpetual:
          continue; // skip this move
        case RepetitionType::sennichite:
          child_score = 0;
          break;
        default:
          std::unreachable();
        }
      } else {
        if (!NodeT::is_pv || legal_moves > 0)
          child_score = -search<nodetype::NonPv>(game, ctrl, child_pv, -alpha - 1, -alpha, ply + 1, depth - 1);

        if (NodeT::is_pv && (legal_moves == 0 || child_score > alpha))
          child_score = -search<nodetype::Pv>(game, ctrl, child_pv, -beta, -alpha, ply + 1, depth - 1);
      }

      legal_moves++;

      if (ctrl.isTerminated())
        return 0;

      if (child_score > best_score) {
        best_score = child_score;

        if (child_score > alpha) {
          alpha = child_score;

          if constexpr (NodeT::is_pv)
            pv.write(m, std::move(child_pv));

          if (child_score >= beta)
            break;
        }
      }
    }

    if (legal_moves == 0)
      return eval::mated(ply);
    return best_score;
  }

  template <typename ControlT> auto go(Game &game, ControlT &ctrl) -> void {
    Line last_pv{};
    i32 last_score;
    i32 last_depth;

    for (i32 depth = 1; depth < max_search_ply; depth++) {
      Line pv{};
      const i32 score = search<nodetype::Root>(game, ctrl, pv, eval::min_score, eval::max_score, 0, depth);

      if (ctrl.isTerminated())
        break;

      last_score = score;
      last_pv = pv;
      last_depth = depth;

      if (ctrl.checkSoftTermination(depth))
        break;

      const f64 nps = static_cast<f64>(ctrl.nodeCount()) / time::cast<time::FloatSeconds>(ctrl.elapsed()).count();
      std::print("info depth {} score cp {} time {} nodes {} nps {} pv {}\n", depth, score, time::cast<time::Milliseconds>(ctrl.elapsed()).count(),
                 ctrl.nodeCount(), static_cast<u64>(nps), pv);
    }

    const f64 nps = static_cast<f64>(ctrl.nodeCount()) / time::cast<time::FloatSeconds>(ctrl.elapsed()).count();
    std::print("info depth {} score cp {} time {} nodes {} nps {} pv {}\n", last_depth, last_score,
               time::cast<time::Milliseconds>(ctrl.elapsed()).count(), ctrl.nodeCount(), static_cast<u64>(nps), last_pv);
    std::print("bestmove {}\n", last_pv.pv[0]);
  }

  auto usiTime(Game &game, TimeSettings ts, time::TimePoint start_time) -> void {
    constexpr time::Milliseconds margin{100};

    const time::Milliseconds remaining_time = game.position().activeColor() == Color::sente ? ts.btime : ts.wtime;
    const time::Milliseconds increment = game.position().activeColor() == Color::sente ? ts.binc : ts.winc;

    const time::Milliseconds safe_remaining = std::max(remaining_time - margin, time::Milliseconds{0});
    const time::Milliseconds safe_byoyomi = std::max(ts.byoyomi - margin, time::Milliseconds{0});

    const time::Milliseconds soft_limit = safe_remaining / 40 + safe_byoyomi;
    const time::Milliseconds hard_limit = safe_remaining / 3 + safe_byoyomi;

    TimeControl ctrl{start_time, time::cast<time::Duration>(soft_limit), time::cast<time::Duration>(hard_limit)};
    go(game, ctrl);
  }

  auto usiDepth(Game &game, i32 depth, time::TimePoint start_time) -> void {
    DepthControl ctrl{start_time, depth};
    go(game, ctrl);
  }

  auto usiNode(Game &game, u64 nodes, time::TimePoint start_time) -> void {
    NodeControl ctrl{start_time, nodes};
    go(game, ctrl);
  }

  auto bench(Game &game, i32 depth, time::TimePoint start_time) -> u64 {
    DepthControl ctrl{start_time, depth};
    go(game, ctrl);
    return ctrl.nodeCount();
  }

} // namespace lb::search
