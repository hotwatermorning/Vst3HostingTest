#pragma once

#include <functional>

namespace hwm { namespace detail {
    
template <typename F>
struct ScopeExit
{
    ScopeExit(F&& exit_behavior)
    : exit_behavior_(std::forward<F>(exit_behavior))
    , initialized_(true)
    {
    }
    
    ScopeExit(ScopeExit<F>&& rhs)
    :   exit_behavior_(std::move(rhs.exit_behavior_))
    {
        rhs.initialized_ = false;
        initialized_ = true;
    }
    
    ScopeExit<F>& operator=(ScopeExit<F>&& rhs)
    {
        exit_behavior_ = std::move(rhs.exit_behavior_);
        rhs.initialized_ = false;
        initialized_ = true;
    }
    
    ~ScopeExit() { if(initialized_) { exit_behavior_(); } }
    
    F exit_behavior_;
    bool initialized_;
};

template <typename F>
ScopeExit<F> MakeScopeExit(F&& f) { return ScopeExit<F>(std::forward<F>(f)); }

}} // hwm::detail

#define HWM_SCOPE_EXIT_CAT(x, y) HWM_SCOPE_EXIT_CAT_I(x, y)
#define HWM_SCOPE_EXIT_CAT_I(x, y) x##y
#define HWM_SCOPE_EXIT(...) HWM_SCOPE_EXIT_CAT(auto hwm_scope_exit_, __LINE__) \
= hwm::detail::MakeScopeExit(__VA_ARGS__)
