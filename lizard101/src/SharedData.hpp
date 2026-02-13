#pragma once
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>

struct SharedData {
    long long counter{ 0 };
    std::atomic<size_t> nextJobIndex{ 0 };
    std::atomic<size_t> inputJobIndex{ 0 };
    std::atomic<size_t> networkJobIndex{ 0 };
    std::atomic<size_t> collisionJobIndex{ 0 };

    std::mutex frameMutex;
    std::condition_variable frameCv;
    std::atomic<int> jobsToComplete{ 0 };
    std::atomic<bool> frameReady{ false };
    std::atomic<bool> running{ true };

    std::string networkMessage;
    std::mutex networkMutex;

};