#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <iomanip>
#include "cpu_scheduler.h"
#include "traffic_scheduler.h"

// Helper struct for test reporting
struct TestCaseResult {
    std::string testName;
    std::string algorithm;
    std::string inputDescription;
    std::string expectedOutput;
    std::string actualOutput;
    bool passed;
};

std::vector<TestCaseResult> testResults;

bool approxEqual(double a, double b, double eps = 0.05) {
    return std::abs(a - b) <= eps;
}

void printReport(const TestCaseResult& tr) {
    std::cout << "\n----------------------------------------------------------------------\n";
    std::cout << "TEST: " << tr.testName << " [" << tr.algorithm << "]\n";
    std::cout << "Input:\n  " << tr.inputDescription << "\n";
    std::cout << "Expected Output:\n  " << tr.expectedOutput << "\n";
    std::cout << "Actual Output:\n  " << tr.actualOutput << "\n";
    std::cout << "Status: " << (tr.passed ? "[PASS] ✓" : "[FAIL] ✗") << "\n";
    std::cout << "----------------------------------------------------------------------\n";
}

// =============================================================
// SECTION 1: CPU SCHEDULING TESTS (1 - 7)
// =============================================================

void test_FCFS_IdleAndDifferentArrivals() {
    std::vector<Process> processes = { Process(1, 2, 4), Process(2, 8, 3) };
    auto res = scheduleFCFS(processes);

    bool ok = (res.processes.size() == 2) &&
              (res.processes[0].id == 1 && res.processes[0].completionTime == 6 && res.processes[0].waitingTime == 0 && res.processes[0].turnaroundTime == 4) &&
              (res.processes[1].id == 2 && res.processes[1].completionTime == 11 && res.processes[1].waitingTime == 0 && res.processes[1].turnaroundTime == 3) &&
              (res.totalTime == 11) && (res.idleTime == 4) && approxEqual(res.cpuUtilization, 63.64);

    assert(ok && "FCFS Idle & Different Arrivals assertion failed");
    TestCaseResult tr{"FCFS with Idle Periods & Different Arrival Times", "FCFS",
                      "P1(AT=2, BT=4), P2(AT=8, BT=3)",
                      "P1: CT=6, WT=0, TAT=4; P2: CT=11, WT=0, TAT=3; TotalTime=11; Idle=4; CPU Util=63.64%",
                      "P1: CT=" + std::to_string(res.processes[0].completionTime) + ", WT=" + std::to_string(res.processes[0].waitingTime) +
                      ", TAT=" + std::to_string(res.processes[0].turnaroundTime) + "; P2: CT=" + std::to_string(res.processes[1].completionTime) +
                      ", WT=" + std::to_string(res.processes[1].waitingTime) + ", TAT=" + std::to_string(res.processes[1].turnaroundTime) +
                      "; TotalTime=" + std::to_string(res.totalTime) + "; Idle=" + std::to_string(res.idleTime) +
                      "; CPU Util=" + std::to_string(res.cpuUtilization) + "%", ok};
    testResults.push_back(tr);
    printReport(tr);
}

void test_FCFS_SameArrivalTimes() {
    std::vector<Process> processes = { Process(1, 0, 5), Process(2, 0, 3), Process(3, 0, 2) };
    auto res = scheduleFCFS(processes);

    bool ok = (res.processes[0].completionTime == 5 && res.processes[0].waitingTime == 0) &&
              (res.processes[1].completionTime == 8 && res.processes[1].waitingTime == 5) &&
              (res.processes[2].completionTime == 10 && res.processes[2].waitingTime == 8) &&
              approxEqual(res.avgWaitingTime, 4.33) && approxEqual(res.avgTurnaroundTime, 7.67) &&
              (res.totalTime == 10) && (res.idleTime == 0);

    assert(ok && "FCFS Same Arrival Times assertion failed");
    TestCaseResult tr{"FCFS Simultaneous Arrivals at t=0", "FCFS",
                      "P1(AT=0, BT=5), P2(AT=0, BT=3), P3(AT=0, BT=2)",
                      "CT: P1=5, P2=8, P3=10; WT: P1=0, P2=5, P3=8; Avg WT=4.33; Avg TAT=7.67",
                      "CT: P1=" + std::to_string(res.processes[0].completionTime) + ", P2=" + std::to_string(res.processes[1].completionTime) +
                      ", P3=" + std::to_string(res.processes[2].completionTime) + "; WT: P1=" + std::to_string(res.processes[0].waitingTime) +
                      ", P2=" + std::to_string(res.processes[1].waitingTime) + ", P3=" + std::to_string(res.processes[2].waitingTime) +
                      "; Avg WT=" + std::to_string(res.avgWaitingTime) + "; Avg TAT=" + std::to_string(res.avgTurnaroundTime), ok};
    testResults.push_back(tr);
    printReport(tr);
}

void test_SJF_NonPreemptive() {
    std::vector<Process> processes = { Process(1, 0, 7), Process(2, 2, 4), Process(3, 4, 1), Process(4, 5, 4) };
    auto res = scheduleSJF(processes);

    auto getP = [&](int pid) -> Process {
        for (const auto& p : res.processes) if (p.id == pid) return p;
        return Process();
    };

    auto p1 = getP(1); auto p2 = getP(2); auto p3 = getP(3); auto p4 = getP(4);
    bool ok = (p1.completionTime == 7 && p1.waitingTime == 0) &&
              (p3.completionTime == 8 && p3.waitingTime == 3) &&
              (p2.completionTime == 12 && p2.waitingTime == 6) &&
              (p4.completionTime == 16 && p4.waitingTime == 7) &&
              approxEqual(res.avgWaitingTime, 4.00) && approxEqual(res.avgTurnaroundTime, 8.00);

    assert(ok && "SJF Non-preemptive assertion failed");
    TestCaseResult tr{"SJF Non-Preemptive with Tie Breaking", "SJF",
                      "P1(AT=0, BT=7), P2(AT=2, BT=4), P3(AT=4, BT=1), P4(AT=5, BT=4)",
                      "CT: P1=7, P3=8, P2=12, P4=16; WT: P1=0, P3=3, P2=6, P4=7; Avg WT=4.00; Avg TAT=8.00",
                      "CT: P1=" + std::to_string(p1.completionTime) + ", P3=" + std::to_string(p3.completionTime) +
                      ", P2=" + std::to_string(p2.completionTime) + ", P4=" + std::to_string(p4.completionTime) +
                      "; WT: P1=" + std::to_string(p1.waitingTime) + ", P3=" + std::to_string(p3.waitingTime) +
                      ", P2=" + std::to_string(p2.waitingTime) + ", P4=" + std::to_string(p4.waitingTime) +
                      "; Avg WT=" + std::to_string(res.avgWaitingTime) + "; Avg TAT=" + std::to_string(res.avgTurnaroundTime), ok};
    testResults.push_back(tr);
    printReport(tr);
}

void test_Priority_NonPreemptive() {
    std::vector<Process> processes = { Process(1, 0, 4, 3), Process(2, 1, 3, 1), Process(3, 2, 5, 2) };
    auto res = schedulePriorityNonPreemptive(processes);

    auto getP = [&](int pid) -> Process {
        for (const auto& p : res.processes) if (p.id == pid) return p;
        return Process();
    };

    auto p1 = getP(1); auto p2 = getP(2); auto p3 = getP(3);
    bool ok = (p1.completionTime == 4 && p1.waitingTime == 0) &&
              (p2.completionTime == 7 && p2.waitingTime == 3) &&
              (p3.completionTime == 12 && p3.waitingTime == 5) &&
              approxEqual(res.avgWaitingTime, 2.67) && approxEqual(res.avgTurnaroundTime, 6.67);

    assert(ok && "Priority Non-preemptive assertion failed");
    TestCaseResult tr{"Priority Scheduling Non-Preemptive", "Priority Non-Preemptive",
                      "P1(AT=0, BT=4, Prio=3), P2(AT=1, BT=3, Prio=1), P3(AT=2, BT=5, Prio=2)",
                      "CT: P1=4, P2=7, P3=12; WT: P1=0, P2=3, P3=5; Avg WT=2.67; Avg TAT=6.67",
                      "CT: P1=" + std::to_string(p1.completionTime) + ", P2=" + std::to_string(p2.completionTime) +
                      ", P3=" + std::to_string(p3.completionTime) + "; WT: P1=" + std::to_string(p1.waitingTime) +
                      ", P2=" + std::to_string(p2.waitingTime) + ", P3=" + std::to_string(p3.waitingTime) +
                      "; Avg WT=" + std::to_string(res.avgWaitingTime) + "; Avg TAT=" + std::to_string(res.avgTurnaroundTime), ok};
    testResults.push_back(tr);
    printReport(tr);
}

void test_Priority_Preemptive() {
    std::vector<Process> processes = { Process(1, 0, 5, 3), Process(2, 1, 3, 1), Process(3, 2, 4, 2) };
    auto res = schedulePriorityPreemptive(processes);

    auto getP = [&](int pid) -> Process {
        for (const auto& p : res.processes) if (p.id == pid) return p;
        return Process();
    };

    auto p1 = getP(1); auto p2 = getP(2); auto p3 = getP(3);
    bool ok = (p1.completionTime == 12 && p1.waitingTime == 7 && p1.responseTime == 0) &&
              (p2.completionTime == 4 && p2.waitingTime == 0 && p2.responseTime == 0) &&
              (p3.completionTime == 8 && p3.waitingTime == 2 && p3.responseTime == 2) &&
              approxEqual(res.avgWaitingTime, 3.00) && approxEqual(res.avgTurnaroundTime, 7.00);

    assert(ok && "Priority Preemptive assertion failed");
    TestCaseResult tr{"Priority Scheduling Preemptive (Preemption Verification)", "Priority Preemptive",
                      "P1(AT=0, BT=5, Prio=3), P2(AT=1, BT=3, Prio=1), P3(AT=2, BT=4, Prio=2)",
                      "CT: P1=12, P2=4, P3=8; WT: P1=7, P2=0, P3=2; RT: P1=0, P2=0, P3=2; Avg WT=3.00",
                      "CT: P1=" + std::to_string(p1.completionTime) + ", P2=" + std::to_string(p2.completionTime) +
                      ", P3=" + std::to_string(p3.completionTime) + "; WT: P1=" + std::to_string(p1.waitingTime) +
                      ", P2=" + std::to_string(p2.waitingTime) + ", P3=" + std::to_string(p3.waitingTime) +
                      "; RT: P1=" + std::to_string(p1.responseTime) + ", P2=" + std::to_string(p2.responseTime) +
                      ", P3=" + std::to_string(p3.responseTime) + "; Avg WT=" + std::to_string(res.avgWaitingTime), ok};
    testResults.push_back(tr);
    printReport(tr);
}

void test_RoundRobin_QuantumVariations() {
    std::vector<Process> pSet = { Process(1, 0, 5), Process(2, 1, 3), Process(3, 2, 1) };
    auto resQ2 = scheduleRoundRobin(pSet, 2);
    auto getP = [&](const CPUScheduleResult& r, int pid) -> Process {
        for (const auto& p : r.processes) if (p.id == pid) return p;
        return Process();
    };

    auto p1_q2 = getP(resQ2, 1); auto p2_q2 = getP(resQ2, 2); auto p3_q2 = getP(resQ2, 3);
    bool okQ2 = (p1_q2.completionTime == 9 && p1_q2.waitingTime == 4) &&
                (p2_q2.completionTime == 8 && p2_q2.waitingTime == 4) &&
                (p3_q2.completionTime == 5 && p3_q2.waitingTime == 2) && approxEqual(resQ2.avgWaitingTime, 3.33);

    auto resQ4 = scheduleRoundRobin(pSet, 4);
    auto p1_q4 = getP(resQ4, 1); auto p2_q4 = getP(resQ4, 2); auto p3_q4 = getP(resQ4, 3);
    bool okQ4 = (p1_q4.completionTime == 9 && p1_q4.waitingTime == 4) &&
                (p2_q4.completionTime == 7 && p2_q4.waitingTime == 3) &&
                (p3_q4.completionTime == 8 && p3_q4.waitingTime == 5) && approxEqual(resQ4.avgWaitingTime, 4.00);

    assert(okQ2 && okQ4 && "Round Robin variations assertion failed");
    TestCaseResult tr{"Round Robin Quantum Variations (Q=2 vs Q=4)", "Round Robin",
                      "P1(AT=0, BT=5), P2(AT=1, BT=3), P3(AT=2, BT=1)",
                      "Q=2: Avg WT=3.33, CT(P1=9, P2=8, P3=5) | Q=4: Avg WT=4.00, CT(P1=9, P2=7, P3=8)",
                      "Q=2: Avg WT=" + std::to_string(resQ2.avgWaitingTime) + ", CT(P1=" + std::to_string(p1_q2.completionTime) +
                      ", P2=" + std::to_string(p2_q2.completionTime) + ", P3=" + std::to_string(p3_q2.completionTime) +
                      ") | Q=4: Avg WT=" + std::to_string(resQ4.avgWaitingTime) + ", CT(P1=" + std::to_string(p1_q4.completionTime) +
                      ", P2=" + std::to_string(p2_q4.completionTime) + ", P3=" + std::to_string(p3_q4.completionTime) + ")", (okQ2 && okQ4)};
    testResults.push_back(tr);
    printReport(tr);
}

void test_SingleProcess_EdgeCase() {
    std::vector<Process> processes = { Process(1, 4, 6, 1) };
    auto res = scheduleFCFS(processes);

    bool ok = (res.processes.size() == 1) &&
              (res.processes[0].completionTime == 10) &&
              (res.processes[0].waitingTime == 0) &&
              (res.processes[0].turnaroundTime == 6) &&
              (res.processes[0].responseTime == 0) &&
              (res.totalTime == 10) && (res.idleTime == 4) && approxEqual(res.cpuUtilization, 60.0);

    assert(ok && "Single process edge case assertion failed");
    TestCaseResult tr{"Single Process Edge Case with Initial Idle Period", "FCFS / Generic",
                      "P1(AT=4, BT=6, Prio=1)",
                      "P1: CT=10, WT=0, TAT=6, RT=0; TotalTime=10, Idle=4, CPU Util=60.0%",
                      "P1: CT=" + std::to_string(res.processes[0].completionTime) + ", WT=" + std::to_string(res.processes[0].waitingTime) +
                      ", TAT=" + std::to_string(res.processes[0].turnaroundTime) + ", RT=" + std::to_string(res.processes[0].responseTime) +
                      "; TotalTime=" + std::to_string(res.totalTime) + ", Idle=" + std::to_string(res.idleTime) +
                      ", CPU Util=" + std::to_string(res.cpuUtilization) + "%", ok};
    testResults.push_back(tr);
    printReport(tr);
}

// =============================================================
// SECTION 2: TRAFFIC SCHEDULING TESTS (8 - 17)
// =============================================================

// 8. Equal Traffic on All Roads
void test_Traffic_EqualTrafficAllRoads() {
    std::vector<TrafficTask> tasks = {
        TrafficTask(1, "North", 0, 5, 5, 1, 30),
        TrafficTask(2, "South", 0, 5, 5, 1, 30),
        TrafficTask(3, "East",  0, 5, 5, 1, 30),
        TrafficTask(4, "West",  0, 5, 5, 1, 30)
    };

    auto res = scheduleTrafficFCFS(tasks);
    bool ok = (res.totalVehiclesServed == 20) && (res.completedTasks.size() == 4) && (res.deadlineMisses == 0);
    assert(ok && "Equal traffic all roads assertion failed");

    TestCaseResult tr{"Equal Traffic on All 4 Roads", "Traffic FCFS",
                      "4 Roads (North, South, East, West) with 5 vehicles each at t=0",
                      "Total Vehicles Served = 20, All 4 approaches completed, 0 deadline misses",
                      "Total Vehicles Served = " + std::to_string(res.totalVehiclesServed) +
                      ", Completed Tasks = " + std::to_string(res.completedTasks.size()) +
                      ", Deadline Misses = " + std::to_string(res.deadlineMisses), ok};
    testResults.push_back(tr);
    printReport(tr);
}

// 9. Heavy Traffic on One Road vs Other Roads
void test_Traffic_HeavyTrafficOneRoad() {
    std::vector<TrafficTask> tasks = {
        TrafficTask(1, "North", 0, 20, 20, 1, 50),
        TrafficTask(2, "South", 0, 3, 3, 1, 50),
        TrafficTask(3, "East",  0, 2, 2, 1, 50)
    };

    auto res = scheduleTrafficPriority(tasks);
    // North has highest density, so it gets green first
    bool ok = (!res.completedTasks.empty()) && (res.signalTimeline[0].roadName == "North") &&
              (res.totalVehiclesServed == 25);
    assert(ok && "Heavy traffic one road assertion failed");

    TestCaseResult tr{"Heavy Traffic on One Road vs Low Traffic", "Traffic Priority",
                      "North(20 veh), South(3 veh), East(2 veh) at t=0",
                      "North granted green first due to highest density; Total Vehicles = 25",
                      "First Green Granted to: " + res.signalTimeline[0].roadName +
                      "; Total Vehicles Served = " + std::to_string(res.totalVehiclesServed), ok};
    testResults.push_back(tr);
    printReport(tr);
}

// 10. Low Traffic on One Road (Minimum Green Constraint Verification)
void test_Traffic_LowTrafficOneRoad() {
    TrafficConfig cfg;
    cfg.minGreenTime = 4; // Set minimum green to 4 seconds
    std::vector<TrafficTask> tasks = {
        TrafficTask(1, "West", 0, 1, 1, 1, 20) // Only 1 vehicle
    };

    auto res = scheduleTrafficFCFS(tasks, cfg);
    int greenDuration = res.signalTimeline[0].duration();
    bool ok = (greenDuration >= cfg.minGreenTime); // Must satisfy min green duration
    assert(ok && "Minimum green duration constraint failed");

    TestCaseResult tr{"Low Traffic Road Minimum Green Constraint", "Traffic FCFS",
                      "West(1 veh, service=1s) with Min Green Config = 4s",
                      "Green duration must be >= 4s to prevent unsafe rapid signal cycling",
                      "Actual Green Duration = " + std::to_string(greenDuration) + "s", ok};
    testResults.push_back(tr);
    printReport(tr);
}

// 11. Emergency Vehicle Arrival and Priority Preemption
void test_Traffic_EmergencyVehicleArrival() {
    TrafficConfig cfg;
    cfg.minGreenTime = 3;
    cfg.yellowDuration = 2;
    cfg.allRedDuration = 1;

    std::vector<TrafficTask> tasks = {
        TrafficTask(1, "North", 0, 10, 10, 1, 40, false),
        TrafficTask(2, "South", 0, 8, 8, 1, 40, false),
        TrafficTask(3, "East",  2, 2, 3, 1, 40, true) // Emergency arrives at t=2
    };

    auto res = scheduleTrafficPriority(tasks, cfg);

    // North runs for minGreenTime (t=0..3)
    // Emergency on East preempts South! Clearance (t=3..6: Yellow+AllRed)
    // East gets Green at t=6 before South!
    bool eastServicedBeforeSouth = false;
    for (size_t i = 0; i < res.signalTimeline.size(); ++i) {
        if (res.signalTimeline[i].signalState == "GREEN") {
            if (res.signalTimeline[i].roadName == "East") {
                eastServicedBeforeSouth = true;
                break;
            }
            if (res.signalTimeline[i].roadName == "South") {
                break;
            }
        }
    }

    bool ok = eastServicedBeforeSouth && (res.totalVehiclesServed == 20);
    assert(ok && "Emergency vehicle arrival priority assertion failed");

    TestCaseResult tr{"Emergency Vehicle Arrival & Preemption", "Traffic Priority",
                      "North(10 veh, t=0), South(8 veh, t=0), East(Emergency, t=2)",
                      "East (Emergency) preempts normal queue and receives Green before South",
                      "East received Green before South: " + std::string(eastServicedBeforeSouth ? "YES" : "NO") +
                      "; Total Served = " + std::to_string(res.totalVehiclesServed), ok};
    testResults.push_back(tr);
    printReport(tr);
}

// 12. Multiple Emergency Vehicles
void test_Traffic_MultipleEmergencyVehicles() {
    std::vector<TrafficTask> tasks = {
        TrafficTask(1, "North", 0, 2, 3, 1, 30, true), // Ambulance
        TrafficTask(2, "South", 0, 2, 3, 1, 30, true), // Fire Truck
        TrafficTask(3, "East",  0, 5, 5, 1, 30, false) // Normal
    };

    auto res = scheduleTrafficPriority(tasks);
    // Both emergencies must be served before East gets green
    bool emergenciesFirst = true;
    int normalGreenIdx = -1;
    for (size_t i = 0; i < res.signalTimeline.size(); ++i) {
        if (res.signalTimeline[i].signalState == "GREEN" && res.signalTimeline[i].roadName == "East") {
            normalGreenIdx = static_cast<int>(i);
            break;
        }
    }

    for (size_t i = 0; i < res.signalTimeline.size(); ++i) {
        if (res.signalTimeline[i].signalState == "GREEN" &&
           (res.signalTimeline[i].roadName == "North" || res.signalTimeline[i].roadName == "South")) {
            if (normalGreenIdx != -1 && static_cast<int>(i) > normalGreenIdx) {
                emergenciesFirst = false;
            }
        }
    }

    bool ok = emergenciesFirst && (res.totalVehiclesServed == 9);
    assert(ok && "Multiple emergency vehicles assertion failed");

    TestCaseResult tr{"Multiple Emergency Vehicles at Intersection", "Traffic Priority",
                      "North(Emergency), South(Emergency), East(Normal)",
                      "Both emergency directions served before any normal direction",
                      "Emergencies cleared first: " + std::string(emergenciesFirst ? "YES" : "NO"), ok};
    testResults.push_back(tr);
    printReport(tr);
}

// 13. Continuous Staggered Traffic Arrivals
void test_Traffic_ContinuousArrivals() {
    std::vector<TrafficTask> tasks = {
        TrafficTask(1, "North", 0,  3, 3, 1, 50),
        TrafficTask(2, "East",  5,  4, 4, 1, 50),
        TrafficTask(3, "South", 12, 3, 3, 1, 50),
        TrafficTask(4, "West",  18, 5, 5, 1, 50)
    };

    auto res = scheduleTrafficFCFS(tasks);
    bool ok = (res.completedTasks.size() == 4) && (res.totalVehiclesServed == 15) &&
              (res.completedTasks.back().completionTime > 20);
    assert(ok && "Continuous arrivals assertion failed");

    TestCaseResult tr{"Continuous Staggered Traffic Arrivals", "Traffic FCFS",
                      "Arrivals at t=0(North), t=5(East), t=12(South), t=18(West)",
                      "All 4 continuous waves serviced smoothly without collision; Total=15 veh",
                      "Total Served = " + std::to_string(res.totalVehiclesServed) +
                      ", Final Completion Time = " + std::to_string(res.totalSimulationTime) + "s", ok};
    testResults.push_back(tr);
    printReport(tr);
}

// 14. Starvation Prevention via Priority Aging
void test_Traffic_StarvationPrevention() {
    TrafficConfig cfg;
    cfg.agingRate = 2; // 2 points per second of waiting

    TrafficTask highDensityTask(1, "North", 0, 15, 15, 0, 60); // prio: 0 + 15*2 = 30
    TrafficTask lowDensityTask(2, "West", 0, 2, 2, 0, 60);     // prio: 0 + 2*2 = 4

    int initialWestPrio = calculateDynamicPriority(lowDensityTask, 0, cfg);
    int initialNorthPrio = calculateDynamicPriority(highDensityTask, 0, cfg);

    // After 20 seconds of waiting without green:
    int agedWestPrio = calculateDynamicPriority(lowDensityTask, 20, cfg); // 4 + 20*2 = 44 > 30

    bool ok = (initialWestPrio < initialNorthPrio) && (agedWestPrio > initialNorthPrio);
    assert(ok && "Starvation prevention priority aging assertion failed");

    TestCaseResult tr{"Starvation Prevention via Dynamic Priority Aging", "Traffic Priority",
                      "North(15 veh, score=30), West(2 veh, score=4), agingRate=2/s",
                      "Initial: West < North; After 20s delay: West priority ages to 44 > North (Prevents Starvation)",
                      "Initial West=" + std::to_string(initialWestPrio) + " vs North=" + std::to_string(initialNorthPrio) +
                      " | Aged West (at t=20)=" + std::to_string(agedWestPrio) + " > North", ok};
    testResults.push_back(tr);
    printReport(tr);
}

// 15. Deadline Misses Calculation (EDF vs Tight Deadlines)
void test_Traffic_DeadlineMisses() {
    std::vector<TrafficTask> tasks = {
        TrafficTask(1, "North", 0, 8, 8, 1, 10), // Deadline at 10
        TrafficTask(2, "South", 0, 8, 8, 1, 12)  // Deadline at 12 (Will miss due to 8s + 3s clearance = 11s start)
    };

    auto res = scheduleTrafficFCFS(tasks);
    bool ok = (res.deadlineMisses == 1) && (res.completedTasks[1].missedDeadline == true);
    assert(ok && "Deadline misses assertion failed");

    TestCaseResult tr{"Deadline Misses Detection under Tight Deadlines", "Traffic EDF / FCFS",
                      "Task 1 (Service=8s, Deadline=10s), Task 2 (Service=8s, Deadline=12s)",
                      "Task 1 meets deadline; Task 2 misses deadline due to clearance delay; Misses = 1",
                      "Deadline Misses Detected = " + std::to_string(res.deadlineMisses) +
                      " (Task 2 completion = " + std::to_string(res.completedTasks[1].completionTime) + "s > 12s)", ok};
    testResults.push_back(tr);
    printReport(tr);
}

// 16. Empty Traffic Queue Edge Case
void test_Traffic_EmptyQueues() {
    std::vector<TrafficTask> emptyList;
    auto res = scheduleTrafficFCFS(emptyList);

    bool ok = (res.completedTasks.empty()) && (res.totalVehiclesServed == 0) &&
              (res.deadlineMisses == 0) && (res.totalSimulationTime == 0);
    assert(ok && "Empty traffic queue assertion failed");

    TestCaseResult tr{"Empty Traffic Queue Edge Case", "Traffic Generic",
                      "Empty task list (0 vehicles)",
                      "Total Vehicles = 0, Completed Tasks = 0, Simulation Time = 0s, No Crash",
                      "Vehicles Served = " + std::to_string(res.totalVehiclesServed) +
                      ", Total Sim Time = " + std::to_string(res.totalSimulationTime), ok};
    testResults.push_back(tr);
    printReport(tr);
}

// 17. Signal Safety Verification (No Conflicts & Mandatory Clearance Intervals)
void test_Traffic_SignalSafety() {
    TrafficConfig cfg;
    cfg.yellowDuration = 2;
    cfg.allRedDuration = 1;
    cfg.minGreenTime = 3;

    std::vector<TrafficTask> tasks = {
        TrafficTask(1, "North", 0, 4, 4, 1, 40),
        TrafficTask(2, "East",  0, 4, 4, 1, 40),
        TrafficTask(3, "South", 0, 4, 4, 1, 40)
    };

    auto res = scheduleTrafficFCFS(tasks, cfg);

    bool safetyPassed = true;
    std::string prevRoad = "";

    for (size_t i = 0; i < res.signalTimeline.size(); ++i) {
        const auto& interval = res.signalTimeline[i];

        // 1. Min green duration check
        if (interval.signalState == "GREEN") {
            if (interval.duration() < cfg.minGreenTime) safetyPassed = false;

            // 2. Road switch clearance check
            if (!prevRoad.empty() && prevRoad != interval.roadName) {
                // Preceding intervals MUST be yellow for prevRoad, then ALL_RED
                if (i < 2) {
                    safetyPassed = false;
                } else {
                    bool yellowOk = (res.signalTimeline[i-2].signalState == "YELLOW" && res.signalTimeline[i-2].roadName == prevRoad);
                    bool allRedOk = (res.signalTimeline[i-1].signalState == "ALL_RED");
                    if (!yellowOk || !allRedOk) safetyPassed = false;
                }
            }
            prevRoad = interval.roadName;
        }
    }

    bool ok = safetyPassed && (res.totalClearanceTime > 0);
    assert(ok && "Signal safety clearance assertion failed");

    TestCaseResult tr{"Traffic Signal Safety & Clearance Verification", "Traffic Safety Module",
                      "3 Conflicting Approaches (North, East, South) switching green signals",
                      "Strict mutual exclusion; every road switch preceded by Yellow (2s) and All-Red (1s)",
                      "Safety Protocol Verified: " + std::string(safetyPassed ? "PASSED (Zero Conflicts)" : "FAILED") +
                      "; Total Clearance Time = " + std::to_string(res.totalClearanceTime) + "s", ok};
    testResults.push_back(tr);
    printReport(tr);
}

// =============================================================
// MAIN ENTRY POINT FOR TEST SUITE
// =============================================================

int main() {
    std::cout << "======================================================================\n";
    std::cout << "        SMART-TRAFFIC COMPLETE AUTOMATED TEST SUITE (17 TESTS)        \n";
    std::cout << "======================================================================\n";

    std::cout << "\n>>> EXECUTING CPU SCHEDULING TESTS (1 to 7) <<<\n";
    test_FCFS_IdleAndDifferentArrivals();
    test_FCFS_SameArrivalTimes();
    test_SJF_NonPreemptive();
    test_Priority_NonPreemptive();
    test_Priority_Preemptive();
    test_RoundRobin_QuantumVariations();
    test_SingleProcess_EdgeCase();

    std::cout << "\n>>> EXECUTING TRAFFIC TASK SCHEDULING TESTS (8 to 17) <<<\n";
    test_Traffic_EqualTrafficAllRoads();
    test_Traffic_HeavyTrafficOneRoad();
    test_Traffic_LowTrafficOneRoad();
    test_Traffic_EmergencyVehicleArrival();
    test_Traffic_MultipleEmergencyVehicles();
    test_Traffic_ContinuousArrivals();
    test_Traffic_StarvationPrevention();
    test_Traffic_DeadlineMisses();
    test_Traffic_EmptyQueues();
    test_Traffic_SignalSafety();

    int passedCount = 0;
    for (const auto& tr : testResults) {
        if (tr.passed) passedCount++;
    }

    std::cout << "\n======================================================================\n";
    std::cout << "TEST SUITE SUMMARY: " << passedCount << " / " << testResults.size() << " TESTS PASSED.\n";
    if (passedCount == static_cast<int>(testResults.size())) {
        std::cout << "ALL 17 UNIT & SYSTEM TESTS PASSED SUCCESSFULLY! ✓\n";
    } else {
        std::cout << "WARNING: Some tests failed.\n";
    }
    std::cout << "======================================================================\n";

    return (passedCount == static_cast<int>(testResults.size())) ? 0 : 1;
}
