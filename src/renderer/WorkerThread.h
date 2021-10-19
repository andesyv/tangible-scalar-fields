#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <thread>
#include <queue>
#include <tuple>
#include <utility>
#include <future>
#include <concepts>
#include <functional>
#include <mutex>

namespace molumes {
    template<typename F, typename R, typename ... Args> requires std::invocable<F, Args...>
    class WorkerThread {
    private:
        std::thread m_thread{};
        bool m_working = true;
        std::mutex m_jobs_lock;
        using JobT = std::tuple<F, std::tuple<Args...>, std::promise<R>>;
        std::queue<JobT> m_jobs;

        static void do_jobs(bool &working, std::queue<JobT> &jobs, std::mutex& jobs_lock) {
            while (working) {
                if (!jobs.empty()) {
                    JobT job;
                    {
                        std::lock_guard<std::mutex> guard{jobs_lock};
                        job = std::move(jobs.front());
                        jobs.pop();
                    }
                    auto& [func, args, promise] = job;
                    promise.set_value(std::apply(func, args));
                }

                std::this_thread::sleep_for(std::chrono::steady_clock::duration{1000000});
            }
        }

    public:
        std::future<R> queue_job(F func, Args... args) {
            std::lock_guard<std::mutex> guard{m_jobs_lock};
            auto &job = m_jobs.emplace(std::forward<F>(func), std::move(std::forward_as_tuple(args...)), std::promise<R>{});
            return std::get<2>(job).get_future();
        }

        WorkerThread() {
            m_thread = std::thread{do_jobs, std::ref(m_working), std::ref(m_jobs), std::ref(m_jobs_lock)};
        }
    };

    template<typename F, typename R, typename ... Args>
    constexpr auto worker_manager_from_function(std::function < R(Args...) > && funcWrapper) {
        return WorkerThread<F, R, Args...>{};
    }

    template<typename F>
    constexpr auto worker_manager_from_function(F &&func) {
        return worker_manager_from_function<F>(std::function{func});
    }
}

#endif // WORKER_THREAD_H
