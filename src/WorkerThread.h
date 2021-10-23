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

    template<WorkerJob ... Jobs>
    class WorkerThread {
    public:
        using JobTypes = std::tuple<typename decltype(Jobs)::type...>;
        using ResultTypes = std::tuple<typename decltype(Jobs)::result_type...>;
        using JobsQueueType = std::tuple<std::queue<typename decltype(Jobs)::type>...>;

    private:
        std::thread m_thread{};
        bool m_working = true;
        std::mutex m_jobs_lock;
        JobsQueueType m_jobs;
//        std::queue<JobT> m_jobs;

        static void do_jobs(bool &working, JobsQueueType &jobs_tuple, std::mutex& jobs_lock) {
            const auto process_jobs = [&working, &jobs_lock](auto& jobs){
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
            };
            while (working) {
                std::apply([&working, &jobs_lock](auto& ... args){ std::make_tuple(process_jobs(args)...); }, jobs_tuple);

                std::this_thread::sleep_for(std::chrono::steady_clock::duration{1000000});
            }
        }

    public:
        template <typename F, typename R, typename ... Args, typename JobT = WorkerJob<F, R, Args...>>
        auto queue_job(F func, Args... args) {
            std::lock_guard<std::mutex> guard{m_jobs_lock};
            auto &job = std::get<JobT>(m_jobs).emplace(std::forward<F>(func), std::move(std::forward_as_tuple(args...)), std::promise<R>{});
//            auto &job = m_jobs.emplace(std::forward<F>(func), std::move(std::forward_as_tuple(args...)), std::promise<R>{});
            return std::get<2>(job).get_future();
        }

        template <typename F, typename ... Args>
        auto queue_job(F func, Args... args) {
//            using JobT = decltype(worker_job_from_function(func));
            using R = decltype(func(args...));
            return queue_job<F, R, Args...>(std::forward<F>(func), std::forward<Args>(args)...);
        }

        WorkerThread() {
            m_thread = std::thread{do_jobs, std::ref(m_working), std::ref(m_jobs), std::ref(m_jobs_lock)};
        }

        explicit WorkerThread(std::tuple<decltype(Jobs)...> args) {

        }

        ~WorkerThread() {
            m_working = false; // Warn worker to stop
            if (m_thread.joinable())
                m_thread.join(); // Wait for worker thread to be finished before completely freeing resources.
        }
    };

//    template <WorkerJob ... Jobs>
//    constexpr auto worker_manager_from_functions_impl(std::tuple<declype(Jobs)...>) {
//        return WorkerThread<Jobs...>{};
//    }

//    template <typename ... Ts>
//    auto WorkerThreadHelper(Ts... args) {
//        return WorkerThread{WorkerJob{args}...};
//    }

    template <typename ... Fs>
    constexpr auto worker_manager_from_functions(Fs&& ... args) {
//        const auto packedFuncs = worker_jobs_from_functions(args...);
//        return std::apply([]<typename ... Ts>(Ts&& ... args){ return WorkerThread<Ts::args_type...>{}; }, packedFuncs);
        return WorkerThread{worker_jobs_from_functions(args...)};
    }
}

#endif // WORKER_THREAD_H
