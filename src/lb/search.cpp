#include "lb/search.h"

#include <algorithm>
#include <print>

#include "lb/eval/eval.h"
#include "lb/eval/hce.h"
#include "lb/game.h"
#include "lb/line.h"
#include "lb/move_picker.h"
#include "lb/search_control.h"
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

  template <typename NodeT, typename ControlT> auto search(Game &game, ControlT &ctrl, Line &pv, i32 alpha, i32 beta, i32 ply, i32 depth) -> i32 {
    const i32 initial_alpha = alpha;

    if (game.position().canDeclareEnteringKingsWin()) {
      pv.write(Move::win());
      return eval::mating(ply);
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

    // Mate distance pruning
    if constexpr (!NodeT::is_root) {
      alpha = std::max(alpha, eval::mated(ply));
      beta = std::min(beta, eval::mating(ply + 1));
      if (alpha >= beta)
        return alpha;
    }

    const auto tte = game.ttLoad(ply);

    if constexpr (!NodeT::is_pv) {
      if (tte.depth >= depth && [&] {
            switch (tte.bound) {
            case tt::Bound::none:
              return false;
            case tt::Bound::lower_bound:
              return tte.score >= beta;
            case tt::Bound::exact:
              return true;
            case tt::Bound::upper_bound:
              return tte.score <= initial_alpha;
            }
            std::unreachable();
          }()) {
        return tte.score;
      }
    }

    MovePicker moves{game, tte.move};

    i32 best_score = eval::no_moves;
    usize legal_moves = 0;
    Move best_move = tte.move;

    for (Move m = moves.next(); m != Move::none(); m = moves.next()) {
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
          best_move = m;

          if constexpr (NodeT::is_pv)
            pv.write(m, std::move(child_pv));

          if (child_score >= beta)
            break;
        }
      }
    }

    if (legal_moves == 0)
      return eval::mated(ply);

    game.ttStore(ply, {
                          .depth = depth,
                          .bound = best_score >= beta            ? tt::Bound::lower_bound
                                   : best_score <= initial_alpha ? tt::Bound::upper_bound
                                                                 : tt::Bound::exact,
                          .score = best_score,
                          .move = best_move,
                      });

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
