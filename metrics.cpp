#include "metrics.h"
#include <iostream>
#include <iomanip>
#include <sstream>

std::vector<CPUComparisonRow> MetricsManager::compareAllCPU(const std::vector<Process>& processes, int timeQuantum) {
    std::vector<CPUComparisonRow> rows;
    if (processes.empty()) return rows;

    auto fcfs = scheduleFCFS(processes);
    auto sjf  = scheduleSJF(processes);
    auto pnp  = schedulePriorityNonPreemptive(processes);
    auto pp   = schedulePriorityPreemptive(processes);
    auto rr   = scheduleRoundRobin(processes, timeQuantum);

    auto addRow = [&](const CPUScheduleResult& r) {
        rows.push_back({
            r.algorithmName,
            r.avgWaitingTime,
            r.avgTurnaroundTime,
            r.avgResponseTime,
            r.cpuUtilization,
            r.throughput,
            r.totalTime
        });
    };

    addRow(fcfs);
    addRow(sjf);
    addRow(pnp);
    addRow(pp);
    addRow(rr);

    return rows;
}

std::vector<TrafficComparisonRow> MetricsManager::compareAllTraffic(const std::vector<TrafficTask>& tasks, int timeQuantum, TrafficConfig cfg) {
    std::vector<TrafficComparisonRow> rows;
    if (tasks.empty()) return rows;

    auto fcfs = scheduleTrafficFCFS(tasks, cfg);
    auto prio = scheduleTrafficPriority(tasks, cfg);
    auto rr   = scheduleTrafficRoundRobin(tasks, timeQuantum, cfg);
    auto edf  = scheduleTrafficEDF(tasks, cfg);

    auto addRow = [&](const TrafficScheduleResult& r) {
        rows.push_back({
            r.algorithmName,
            r.totalVehiclesServed,
            r.avgVehicleWaitingTime,
            r.maxWaitingTime,
            r.emergencyAvgWaitingTime,
            r.deadlineMisses,
            r.trafficThroughput,
            r.signalUtilization,
            r.totalSimulationTime,
            r.totalClearanceTime
        });
    };

    addRow(fcfs);
    addRow(prio);
    addRow(rr);
    addRow(edf);

    return rows;
}

void MetricsManager::printCPUComparison(const std::vector<CPUComparisonRow>& rows) {
    std::cout << "\n========================================================================================================\n";
    std::cout << "                             CPU SCHEDULING BENCHMARK COMPARISON TABLE                                  \n";
    std::cout << "========================================================================================================\n";
    std::cout << std::left
              << std::setw(38) << "Algorithm"
              << std::setw(14) << "Avg Wait"
              << std::setw(14) << "Avg TAT"
              << std::setw(14) << "Avg Resp"
              << std::setw(14) << "CPU Util(%)"
              << std::setw(12) << "Throughput"
              << "\n";
    std::cout << "--------------------------------------------------------------------------------------------------------\n";
    std::cout << std::fixed << std::setprecision(2);
    for (const auto& r : rows) {
        std::cout << std::left
                  << std::setw(38) << r.algorithm
                  << std::setw(14) << r.avgWaitingTime
                  << std::setw(14) << r.avgTurnaroundTime
                  << std::setw(14) << r.avgResponseTime
                  << std::setw(14) << r.cpuUtilization
                  << std::setw(12) << r.throughput
                  << "\n";
    }
    std::cout << "========================================================================================================\n";
}

void MetricsManager::printTrafficComparison(const std::vector<TrafficComparisonRow>& rows) {
    std::cout << "\n====================================================================================================================\n";
    std::cout << "                                  TRAFFIC SCHEDULING BENCHMARK COMPARISON TABLE                                     \n";
    std::cout << "====================================================================================================================\n";
    std::cout << std::left
              << std::setw(42) << "Algorithm"
              << std::setw(14) << "Vehicles"
              << std::setw(14) << "Avg Delay"
              << std::setw(12) << "Max Delay"
              << std::setw(14) << "Emerg Delay"
              << std::setw(10) << "Misses"
              << std::setw(12) << "Throughput"
              << std::setw(12) << "Green Util"
              << "\n";
    std::cout << "--------------------------------------------------------------------------------------------------------------------\n";
    std::cout << std::fixed << std::setprecision(2);
    for (const auto& r : rows) {
        std::cout << std::left
                  << std::setw(42) << r.algorithm
                  << std::setw(14) << r.totalVehiclesServed
                  << std::setw(14) << r.avgVehicleWaitingTime
                  << std::setw(12) << r.maxWaitingTime
                  << std::setw(14) << r.emergencyAvgWaitingTime
                  << std::setw(10) << r.deadlineMisses
                  << std::setw(12) << r.trafficThroughput
                  << std::setw(12) << (std::to_string(static_cast<int>(r.signalUtilization)) + "%")
                  << "\n";
    }
    std::cout << "====================================================================================================================\n";
}

std::string MetricsManager::cpuComparisonToJson(const std::vector<CPUComparisonRow>& rows) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "[\n";
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& r = rows[i];
        ss << "  {\n"
           << "    \"algorithm\": \"" << r.algorithm << "\",\n"
           << "    \"avgWaitingTime\": " << r.avgWaitingTime << ",\n"
           << "    \"avgTurnaroundTime\": " << r.avgTurnaroundTime << ",\n"
           << "    \"avgResponseTime\": " << r.avgResponseTime << ",\n"
           << "    \"cpuUtilization\": " << r.cpuUtilization << ",\n"
           << "    \"throughput\": " << r.throughput << ",\n"
           << "    \"totalTime\": " << r.totalTime << "\n"
           << "  }";
        if (i + 1 < rows.size()) ss << ",";
        ss << "\n";
    }
    ss << "]";
    return ss.str();
}

std::string MetricsManager::trafficComparisonToJson(const std::vector<TrafficComparisonRow>& rows) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "[\n";
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& r = rows[i];
        ss << "  {\n"
           << "    \"algorithm\": \"" << r.algorithm << "\",\n"
           << "    \"totalVehiclesServed\": " << r.totalVehiclesServed << ",\n"
           << "    \"avgVehicleWaitingTime\": " << r.avgVehicleWaitingTime << ",\n"
           << "    \"maxWaitingTime\": " << r.maxWaitingTime << ",\n"
           << "    \"emergencyAvgWaitingTime\": " << r.emergencyAvgWaitingTime << ",\n"
           << "    \"deadlineMisses\": " << r.deadlineMisses << ",\n"
           << "    \"trafficThroughput\": " << r.trafficThroughput << ",\n"
           << "    \"signalUtilization\": " << r.signalUtilization << ",\n"
           << "    \"totalSimulationTime\": " << r.totalSimulationTime << ",\n"
           << "    \"totalClearanceTime\": " << r.totalClearanceTime << "\n"
           << "  }";
        if (i + 1 < rows.size()) ss << ",";
        ss << "\n";
    }
    ss << "]";
    return ss.str();
}
