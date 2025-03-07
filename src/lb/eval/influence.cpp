#include "lb/eval/influence.h"

#include <array>
#include <tuple>

#include "lb/attacks.h"
#include "lb/common.h"
#include "lb/position.h"
#include "lb/types.h"

namespace lb::eval {

  using BB = Bitboard;
  using BB2 = std::array<Bitboard, 2>;
  using BB3 = std::array<Bitboard, 3>;
  using BB4 = std::array<Bitboard, 4>;

  inline static auto add(BB a, BB b) -> BB2 { return {a ^ b, a & b}; }
  inline static auto add(BB a, BB b, BB c) -> BB2 { return {a ^ b ^ c, (a & b) | (a & c) | (b & c)}; }

  inline static auto add(BB2 a, BB2 b, BB2 c) -> std::tuple<BB3, BB3> {
    const auto [r0, s1] = add(a[0], b[0], c[0]);
    const auto [r1, s2] = add(a[1], b[1], c[1]);
    return {
        {r0, r1, BB{}},
        {BB{}, s1, s2},
    };
  }

  inline static auto add(std::tuple<BB3, BB3> ab, BB c0) -> BB4 {
    const auto [a, b] = ab;
    const auto [r0, c1] = add(a[0], b[0], c0);
    const auto [r1, c2] = add(a[1], b[1], c1);
    const auto [r2, r3] = add(a[2], b[2], c2);
    return {r0, r1, r2, r3};
  }

  inline static auto add(BB4 a, BB b) -> BB4 {
    const auto [r0, c1] = add(a[0], b);
    const auto [r1, c2] = add(a[1], c1);
    const auto [r2, c3] = add(a[2], c2);
    const auto [r3, _] = add(a[2], c3);
    return {r0, r1, r2, r3};
  }

  auto calcInfluence(Color color, const Position &position) -> BB4 {
    const BB pawn = position.getPiece(color, PieceType::pawn);
    const BB lance = position.getPiece(color, PieceType::lance);
    const BB knight = position.getPiece(color, PieceType::knight);
    const BB silver = position.getPiece(color, PieceType::silver);
    const BB gold = position.getPiece(color, PieceType::gold) | position.getPiece(color, PieceType::tokin) | position.getPromoteds(color);
    const BB horse = position.getPiece(color, PieceType::horse);
    const BB dragon = position.getPiece(color, PieceType::dragon);

    const BB step_orthogonal = gold | horse;
    const BB step_diagonal = silver | dragon;
    const BB step_forwards = gold | silver;

    const BB nw = (step_diagonal | step_forwards).shiftRelative(Direction::nw, color);
    const BB n = (pawn | step_orthogonal | step_forwards).shiftRelative(Direction::n, color);
    const BB ne = (step_diagonal | step_forwards).shiftRelative(Direction::ne, color);
    const BB e = step_orthogonal.shiftRelative(Direction::e, color);
    const BB w = step_orthogonal.shiftRelative(Direction::w, color);
    const BB sw = step_diagonal.shiftRelative(Direction::sw, color);
    const BB s = step_orthogonal.shiftRelative(Direction::s, color);
    const BB se = step_diagonal.shiftRelative(Direction::se, color);

    const BB k1 = knight.shiftRelative(Direction::n, color).shiftRelative(Direction::nw, color);
    const BB k2 = knight.shiftRelative(Direction::n, color).shiftRelative(Direction::ne, color);

    const BB blockers = position.getOccupied();
    const BB lance_attacks = attacks::allLances(lance, color, blockers);

    BB4 result = add(add(add(nw, n | lance_attacks, ne), add(e, w, sw), add(s, se, k1)), k2);

    const BB bishop = position.getPiece(color, PieceType::bishop) | horse;
    for (Square sq : bishop)
      result = add(result, attacks::bishop(sq, blockers));

    const BB rook = position.getPiece(color, PieceType::rook) | dragon;
    for (Square sq : rook)
      result = add(result, attacks::rook(sq, blockers));

    return result;
  }

} // namespace lb::eval
