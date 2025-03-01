#pragma once

namespace lb::internal {
  template <typename F> struct DeferHelper {
    F f;
    DeferHelper(F &&f) : f(std::forward<F>(f)) {}
    ~DeferHelper() { f(); }
  };
} // namespace lb::internal

#define LB_CONCAT2(x, y) x##y
#define LB_CONCAT(x, y) LB_CONCAT2(x, y)

#define lb_defer const lb::internal::DeferHelper LB_CONCAT(_lb_internal_defer_guard_, __COUNTER__) = [&]() -> void
