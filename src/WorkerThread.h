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
    struct WorkerJob {
        using args_type = std::tuple<F, R, Args...>;
        using type = std::tuple<F, std::tuple<Args...>, std::promise<R>>;
        using result_type = std::future<R>;
    };

    template<typename F, typename R, typename ... Args>
    constexpr auto worker_job_from_function_iml(std::function < R(Args...) > && funcWrapper) {
        return WorkerJob<F, R, std::decay_t<Args>...>{};
    }

    template<typename F>
    constexpr auto worker_job_from_function(F &&func) {
        return worker_job_from_function_iml<F>(std::function{func});
    }

    template <typename ... Fs>
    constexpr auto worker_jobs_from_functions(Fs&& ... funcs) {
        return std::make_tuple(worker_job_from_function(funcs)...);
    }

    template<typename ... Jobs>
    class WorkerThread {
    public:
        using JobTypes = std::tuple<typename Jobs::type...>;
        using ResultTypes = std::tuple<typename Jobs::result_type...>;
        using JobsQueueType = std::tuple<std::queue<typename Jobs::type>...>;

    private:
        std::thread m_thread{};
        bool m_working = true;
        std::mutex m_jobs_lock;
        JobsQueueType m_jobs;

        template <typename T>
        static void process_jobs(bool &working, T& jobs, std::mutex& jobs_lock) {
            while (working && !jobs.empty()) {
                jobs_lock.lock();
                if (!jobs.empty()) {
                    auto [func, args, promise] = std::move(jobs.front());
                    jobs.pop();
                    jobs_lock.unlock();
                    promise.set_value(std::apply(func, args));
                } else {
                    jobs_lock.unlock();
                }
            }
        }

        static void do_jobs(bool &working, JobsQueueType &jobs_tuple, std::mutex& jobs_lock) {
            while (working) {
                std::apply([&working, &jobs_lock](auto& ... args){ (process_jobs(working, args, jobs_lock), ...); }, jobs_tuple);

                std::this_thread::sleep_for(std::chrono::steady_clock::duration{1000000});
            }
        }

    public:
        template <std::size_t I, typename F, typename R, typename ... Args>
        auto queue_job(F&& func, Args&&... args) {
            std::lock_guard<std::mutex> guard{m_jobs_lock};
            // Could never figure out how to match tuple element on function parameters, so just manually setting index for now.
            auto& job = std::get<I>(m_jobs).emplace(std::forward<F>(func), std::forward_as_tuple(args...), std::promise<R>{});
            return std::move(std::get<2>(job).get_future());
        }

        template <std::size_t I, typename F, typename ... Args>
        auto queue_job(F&& func, Args&&... args) {
            using R = decltype(std::invoke(std::forward<F>(func), std::forward<Args>(args)...));
            return queue_job<I, F, R, Args...>(std::forward<F>(func), std::forward<Args>(args)...);
        }

        WorkerThread() {
            static_assert((!std::is_same_v<Jobs, void> && ...));
            m_thread = std::thread{do_jobs, std::ref(m_working), std::ref(m_jobs), std::ref(m_jobs_lock)};
        }

        ~WorkerThread() {
            m_working = false; // Warn worker to stop
            assert(m_thread.joinable());
            if (m_thread.joinable())
                m_thread.join(); // Wait for worker thread to be finished before completely freeing resources.
        }
    };

    template <typename ... Ts, std::size_t ... I>
    constexpr auto worker_manager_from_tuple(std::tuple<Ts...> pack, std::index_sequence<I...>) {
        return WorkerThread<typename std::tuple_element<I, std::tuple<Ts...>>::type...>{};
    }

    template <typename ... Ts>
    constexpr auto worker_manager_from_tuple(std::tuple<Ts...> pack) {
        return worker_manager_from_tuple(pack, std::make_index_sequence<sizeof...(Ts)>{});
    }

    template <typename ... Fs>
    constexpr auto worker_manager_from_functions(Fs&& ... args) {
        auto packedFuncs = worker_jobs_from_functions(args...);
        return worker_manager_from_tuple(packedFuncs);
    }
}

#endif // WORKER_THREAD_H
