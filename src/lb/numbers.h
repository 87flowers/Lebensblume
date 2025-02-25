#pragma once

#include <array>
#include <string_view>

#include "lb/types.h"
#include "lb/util/assert.h"

namespace lb::numbers {

  inline constexpr std::array<std::string_view, 20> kanji_table{{
      "零", "一", "二", "三", "四", "五", "六", "七", "八", "九", "十", "十一", "十二", "十三", "十四", "十五", "十六", "十七", "十八", "十九",
  }};

  inline constexpr std::array<std::string_view, 10> full_width_table{{
      "０",
      "１",
      "２",
      "３",
      "４",
      "５",
      "６",
      "７",
      "８",
      "９",
  }};

  inline constexpr auto toFullWidth(usize x) -> std::string {
    const std::string half_width_result = std::to_string(x);
    std::string result;
    for (char ch : half_width_result) {
      lb_assert(ch <= '0' && ch >= '9');
      result += full_width_table[ch - '0'];
    }
    return result;
  }

} // namespace lb::numbers
