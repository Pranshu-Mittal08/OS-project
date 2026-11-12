#ifndef METRICS_H
#define METRICS_H

#include "cpu_scheduler.h"
#include "traffic_scheduler.h"
#include <vector>
#include <string>

// CPU Algorithm Performance Comparison Record
struct CPUComparisonRow {
    std::string algorithm;
    double avgWaitingTime;
    double avgTurnaroundTime;
    double avgResponseTime;
    double cpuUtilization;
    double throughput;
    int totalTime;
};

// Traffic Algorithm Performance Comparison Record
struct TrafficComparisonRow {
    std::string algorithm;
    int totalVehiclesServed;
    double avgVehicleWaitingTime;
    int maxWaitingTime;
    double emergencyAvgWaitingTime;
    int deadlineMisses;
    double trafficThroughput;
    double signalUtilization;
    int totalSimulationTime;
    int totalClearanceTime;
};

// Module for running benchmarks and comparative evaluations
class MetricsManager {
public:
    // Runs all 5 CPU algorithms on identical process input and generates comparison
    static std::vector<CPUComparisonRow> compareAllCPU(const std::vector<Process>& processes, int timeQuantum = 3);

    // Runs all 4 Traffic algorithms on identical traffic task input and generates comparison
    static std::vector<TrafficComparisonRow> compareAllTraffic(const std::vector<TrafficTask>& tasks, int timeQuantum = 6, TrafficConfig cfg = TrafficConfig());

    // Display formatted comparison tables in terminal
    static void printCPUComparison(const std::vector<CPUComparisonRow>& rows);
    static void printTrafficComparison(const std::vector<TrafficComparisonRow>& rows);

    // Export comparison results to JSON string
    static std::string cpuComparisonToJson(const std::vector<CPUComparisonRow>& rows);
    static std::string trafficComparisonToJson(const std::vector<TrafficComparisonRow>& rows);
};

#endif // METRICS_H
