#include "simulation.h"
#include <iostream>
#include <algorithm>

SimulationEngine::SimulationEngine(unsigned int seed)
    : randomSeed(seed), rng(seed), currentState(SimState::STOPPED), currentSimulationStep(0) {}

void SimulationEngine::setSeed(unsigned int seed) {
    randomSeed = seed;
    rng.seed(seed);
}

void SimulationEngine::start() {
    currentState = SimState::RUNNING;
    currentSimulationStep = 0;
}

void SimulationEngine::pause() {
    if (currentState == SimState::RUNNING) {
        currentState = SimState::PAUSED;
    }
}

void SimulationEngine::resume() {
    if (currentState == SimState::PAUSED) {
        currentState = SimState::RUNNING;
    }
}

void SimulationEngine::reset() {
    currentState = SimState::STOPPED;
    currentSimulationStep = 0;
    rng.seed(randomSeed); // Restore reproducible state
}

std::string SimulationEngine::getStateString() const {
    switch (currentState) {
        case SimState::STOPPED: return "STOPPED";
        case SimState::RUNNING: return "RUNNING";
        case SimState::PAUSED:  return "PAUSED";
    }
    return "UNKNOWN";
}

// Generates reproducible random CPU processes
std::vector<Process> SimulationEngine::generateRandomProcesses(int count, int maxArrival, int minBurst, int maxBurst, int minPriority, int maxPriority) {
    std::uniform_int_distribution<int> distArrival(0, maxArrival);
    std::uniform_int_distribution<int> distBurst(minBurst, maxBurst);
    std::uniform_int_distribution<int> distPriority(minPriority, maxPriority);

    std::vector<Process> processes;
    processes.reserve(count);

    for (int i = 1; i <= count; ++i) {
        int at = distArrival(rng);
        int bt = distBurst(rng);
        int prio = distPriority(rng);
        processes.emplace_back(i, at, bt, prio);
    }

    std::sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        if (a.arrivalTime != b.arrivalTime) return a.arrivalTime < b.arrivalTime;
        return a.id < b.id;
    });

    return processes;
}

// Generates reproducible random Traffic tasks
std::vector<TrafficTask> SimulationEngine::generateRandomTraffic(int count, int maxArrival, int minVehicles, int maxVehicles, double emergencyProbability) {
    std::vector<std::string> roads = {"North", "South", "East", "West"};
    std::uniform_int_distribution<int> distRoad(0, 3);
    std::uniform_int_distribution<int> distArrival(0, maxArrival);
    std::uniform_int_distribution<int> distVehicles(minVehicles, maxVehicles);
    std::uniform_real_distribution<double> distEmergency(0.0, 1.0);

    std::vector<TrafficTask> tasks;
    tasks.reserve(count);

    for (int i = 1; i <= count; ++i) {
        std::string road = roads[distRoad(rng)];
        int arrival = distArrival(rng);
        int vehicles = distVehicles(rng);
        int serviceTime = vehicles; // 1 unit per vehicle
        bool isEmergency = (distEmergency(rng) < emergencyProbability);
        int basePriority = isEmergency ? 100 : 1;
        // Deadline is arrival + serviceTime + permissible wait allowance
        int deadline = arrival + serviceTime + (isEmergency ? 10 : 25);

        tasks.emplace_back(i, road, arrival, vehicles, serviceTime, basePriority, deadline, isEmergency);
    }

    std::sort(tasks.begin(), tasks.end(), [](const TrafficTask& a, const TrafficTask& b) {
        if (a.arrivalTime != b.arrivalTime) return a.arrivalTime < b.arrivalTime;
        return a.taskId < b.taskId;
    });

    return tasks;
}

// Preset Scenario 1: Distinct Balanced Traffic (Differentiates FCFS, Priority, RR, and EDF)
std::vector<TrafficTask> SimulationEngine::getPresetBalancedIntersection() {
    return {
        TrafficTask(1, "West",  0,  3,  3, 1, 18, false), // Arrives first, tight deadline
        TrafficTask(2, "North", 1, 14, 14, 1, 40, false), // Heavy density (favored by Priority)
        TrafficTask(3, "South", 2,  8,  8, 1, 30, false), // Medium density
        TrafficTask(4, "East",  3,  5,  5, 1, 25, false)  // Arrives last, early deadline (favored by EDF)
    };
}

// Preset Scenario 2: Morning Rush Hour (Heavy North corridor)
std::vector<TrafficTask> SimulationEngine::getPresetMorningRushHour() {
    return {
        TrafficTask(1, "North", 0, 22, 22, 1, 50, false),
        TrafficTask(2, "South", 0, 18, 18, 1, 55, false),
        TrafficTask(3, "East",  3,  4,  4, 1, 40, false),
        TrafficTask(4, "West",  5,  3,  3, 1, 40, false),
        TrafficTask(5, "North", 15, 14, 14, 1, 65, false)
    };
}

// Preset Scenario 3: Emergency Vehicle Preemption
std::vector<TrafficTask> SimulationEngine::getPresetEmergencyResponse() {
    return {
        TrafficTask(1, "North", 0, 12, 12, 1, 45, false),
        TrafficTask(2, "South", 0, 10, 10, 1, 45, false),
        TrafficTask(3, "East",  3,  2,  3, 1, 20, true),  // Ambulance arrives at t=3
        TrafficTask(4, "West",  4,  6,  6, 1, 50, false)
    };
}

// Manual prompt for CPU processes
std::vector<Process> SimulationEngine::promptManualProcesses() {
    std::vector<Process> processes;
    int n = 0;
    std::cout << "\nEnter number of processes (1 to 10): ";
    if (!(std::cin >> n) || n <= 0) n = 3;

    std::cout << "For each process, enter: ArrivalTime BurstTime Priority (Lower = Higher Prio)\n";
    for (int i = 1; i <= n; ++i) {
        int at = 0, bt = 1, prio = 1;
        std::cout << "  Process P" << i << " [AT BT Priority]: ";
        if (std::cin >> at >> bt >> prio) {
            processes.emplace_back(i, at, bt, prio);
        } else {
            processes.emplace_back(i, 0, 4, 1);
        }
    }
    return processes;
}

// Manual prompt for traffic tasks
std::vector<TrafficTask> SimulationEngine::promptManualTraffic() {
    std::vector<TrafficTask> tasks;
    int n = 0;
    std::cout << "\nEnter number of traffic requests (1 to 8): ";
    if (!(std::cin >> n) || n <= 0) n = 3;

    std::cout << "For each traffic request, enter:\n";
    std::cout << "  RoadName(North/South/East/West) ArrivalTime VehicleCount HasEmergency(1=Yes, 0=No) Deadline\n";
    for (int i = 1; i <= n; ++i) {
        std::string road = "North";
        int at = 0, veh = 5, emerg = 0, dline = 30;
        std::cout << "  Task T" << i << " [Road AT Vehicles Emergency(0/1) Deadline]: ";
        if (std::cin >> road >> at >> veh >> emerg >> dline) {
            int service = veh;
            int prio = (emerg == 1) ? 100 : 1;
            tasks.emplace_back(i, road, at, veh, service, prio, dline, (emerg == 1));
        } else {
            tasks.emplace_back(i, "North", 0, 5, 5, 1, 30, false);
        }
    }
    return tasks;
}
