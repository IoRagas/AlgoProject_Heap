#pragma once
#include <chrono>
#include <string>
#include <unordered_map>
#include <iostream>
#include <mutex>
class Timer
{
private:
    using Clock = std::chrono::high_resolution_clock;
    std::unordered_map<std::string, Clock::time_point> startTimes;
    std::mutex mtx;

public:
    // Begin timing a labeled section
    void Start(const std::string& label)
    {
        std::cout << "Timer : " << label << " started...\n";
        std::lock_guard<std::mutex> lock(mtx);
        startTimes[label] = Clock::now();
    }

    // End timing a labeled section and print result
    void Stop(const std::string& label, const std::string& extraInfo = "")
    {
        std::lock_guard<std::mutex> lock(mtx);

        auto it = startTimes.find(label);
        if (it == startTimes.end())
        {
            std::cerr << "[Timer Error] No active timer with label: " << label << "\n";
            return;
        }

        auto end = Clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - it->second).count();

        std::cout << "[TIMER] " << label;
        if (!extraInfo.empty()) std::cout << " (" << extraInfo << ")";
        std::cout << " = " << duration << " ms\n";

        startTimes.erase(it);
    }

    // Returns elapsed time without printing
    double GetElapsed(const std::string& label)
    {
        std::lock_guard<std::mutex> lock(mtx);

        auto it = startTimes.find(label);
        if (it == startTimes.end())
        {
            std::cerr << "[Timer Error] No active timer with label: " << label << "\n";
            return -1.0;
        }

        auto now = Clock::now();
        return std::chrono::duration<double, std::milli>(now - it->second).count();
    }
};
