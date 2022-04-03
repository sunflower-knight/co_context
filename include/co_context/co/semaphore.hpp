#pragma once

#include "co_context.hpp"
#include <type_traits>
#include "co_context/utility/as_atomic.hpp"

namespace co_context {

class semaphore final {
  private:
    using task_info = detail::task_info;
    using T = config::semaphore_underlying_type;
    static_assert(std::is_integral_v<T>);

    class acquire_awaiter final {
      public:
        explicit acquire_awaiter(semaphore &sem) noexcept : sem(sem) {}

        bool await_ready() noexcept {
            T old_counter =
                sem.counter.fetch_sub(1, std::memory_order_acq_rel); // seq_cst?
            return old_counter > 0;
        }

        void await_suspend(std::coroutine_handle<> current) noexcept;
        void await_resume() const noexcept {}

      private:
        friend class semaphore;
        friend class io_context;
        semaphore &sem;
        acquire_awaiter *next;
        std::coroutine_handle<> handle;
    };

  public:
    constexpr explicit semaphore(T desired) noexcept
        : counter(desired)
        , awaiting(nullptr)
        , to_resume(nullptr)
        , awaken_task(task_info::task_type::semaphore_release) {
        awaken_task.sem = this;
        as_atomic(awaken_task.update).store(0, std::memory_order_relaxed);
    }

    semaphore(const semaphore &) = delete;

    ~semaphore() noexcept;

    bool try_acquire() noexcept;

    acquire_awaiter acquire() noexcept {
        log::d("semaphore %lx acquiring\n", this);
        return acquire_awaiter{*this};
    }

    void release(T update = 1) noexcept;

  private:
    friend class io_context;
    std::coroutine_handle<> try_release() noexcept;

  private:
    std::atomic<acquire_awaiter *> awaiting;
    acquire_awaiter *to_resume;
    std::atomic<T> counter;

    task_info awaken_task;
};

} // namespace co_context