#include "co_context.hpp"

int main(int argc, char *argv[]) {
    co_context::io_context io_context{8};

    io_context.probe();

    // io_context.make_test_thread_pool();

    // io_context.run_test_swap();
    
    io_context.make_thread_pool();

    io_context.run();

    using co_context::task;

    auto t = []() -> task<int> {
        co_return 1;
    }();

    auto aw = t.operator co_await();

    return 0;
}