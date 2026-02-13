#include "JobSystem.hpp"
#include <cstddef>
#include <iostream>

void worker(SharedData& data, const JobQueue& jobs, JobType type) {
    while (data.running) {
        std::unique_lock<std::mutex> lock(data.frameMutex);
        data.frameCv.wait(lock, [&] { return data.frameReady || !data.running; });
        lock.unlock();

        if (!data.running) break;

        while (true) {
            size_t jobIndex = 0;
            switch (type) {
                case JobType::Input:
                    jobIndex = data.inputJobIndex.fetch_add(1);
                    break;
                case JobType::Network:
                    jobIndex = data.networkJobIndex.fetch_add(1);
                    break;
                case JobType::Collision:
                    jobIndex = data.collisionJobIndex.fetch_add(1);
                    break;
            }
            if (jobIndex >= jobs.size()) break;
            jobs[jobIndex]();
        }

        {
            {
                std::lock_guard<std::mutex> lk(data.frameMutex);
                if (--data.jobsToComplete == 0) {
                    data.frameReady = false;
                    data.frameCv.notify_all();
                }
            }
        }
    }
}