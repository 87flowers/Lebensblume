#include "lb/tt.h"

#include <bit>
#include <cstdlib>
#include <cstring>
#include <emmintrin.h>
#include <optional>

#include "lb/common.h"
#include "lb/types.h"

namespace lb::tt {

  static constexpr inline auto splitHash(usize bucket_count, zhash::Hash hash) -> std::tuple<usize, u8, u64> {
    const u128 mul = static_cast<u128>(hash) * bucket_count;
    const usize index = static_cast<usize>(mul >> 64);
    const u8 ctrl = static_cast<u8>(static_cast<u64>(mul) >> 56);
    const u64 fragment = (static_cast<u64>(mul) >> (56 - Entry::fragment_width)) & Entry::fragment_mask;
    return {index, ctrl, fragment};
  }

  static auto getEntryIndex(const Bucket &bucket, u8 ctrl) -> std::optional<usize> {
    const __m128i ctrls = _mm_load_si128(reinterpret_cast<__m128i const *>(bucket.ctrls.data()));
    const __m128i splat = _mm_set1_epi8(ctrl);
    const int matches = _mm_movemask_epi8(_mm_cmpeq_epi8(ctrls, splat));
    const usize index = std::countr_zero(static_cast<unsigned int>(matches));
    return index < Bucket::entry_count ? std::optional<usize>{index} : std::nullopt;
  }

  auto TT::bucket_alloc(std::size_t bucket_count) -> Bucket * {
    return static_cast<Bucket *>(std::aligned_alloc(4096, bucket_count * sizeof(Bucket)));
  }

  auto TT::bucket_free(Bucket *ptr) -> void { return std::free(ptr); }

  auto TT::clear() -> void { std::memset(buckets.get(), 0, bucket_count * sizeof(Bucket)); }

  auto TT::load(zhash::Hash hash, int ply) const -> LookupResult {
    const auto [bucket_index, ctrl, fragment] = splitHash(bucket_count, hash);
    const Bucket &bucket = buckets.get()[bucket_index];

    if (const auto entry_index = getEntryIndex(bucket, ctrl)) {
      const Entry &entry = bucket.entries[*entry_index];
      if (entry.fragment() == fragment) {
        return entry.toResult(ply);
      }
    }
    return {};
  }

  auto TT::store(zhash::Hash hash, int ply, LookupResult lr) -> void {
    const auto [bucket_index, ctrl, fragment] = splitHash(bucket_count, hash);
    Bucket &bucket = buckets.get()[bucket_index];

    const usize entry_index = getEntryIndex(bucket, ctrl)
                                  .or_else([&] {
                                    const usize res = bucket.ctrls.back();
                                    bucket.ctrls.back() = (res + 1) % Bucket::entry_count;
                                    return std::optional<usize>{res};
                                  })
                                  .value();

    bucket.ctrls[entry_index] = ctrl;
    bucket.entries[entry_index] = Entry{fragment, ply, lr};
  }

} // namespace lb::tt
