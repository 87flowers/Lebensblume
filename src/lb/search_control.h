#pragma once

#include "lb/types.h"

namespace lb::search {

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

} // namespace lb::search
