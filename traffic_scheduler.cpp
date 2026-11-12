#include "traffic_scheduler.h"
#include <algorithm>
#include <queue>
#include <cmath>

std::string roadToString(RoadDirection road) {
    switch (road) {
        case RoadDirection::NORTH: return "North";
        case RoadDirection::SOUTH: return "South";
        case RoadDirection::EAST:  return "East";
        case RoadDirection::WEST:  return "West";
    }
    return "Unknown";
}

RoadDirection stringToRoad(const std::string& name) {
    if (name == "South" || name == "SOUTH") return RoadDirection::SOUTH;
    if (name == "East"  || name == "EAST")  return RoadDirection::EAST;
    if (name == "West"  || name == "WEST")  return RoadDirection::WEST;
    return RoadDirection::NORTH;
}

// Dynamic priority calculation incorporating vehicle count, waiting time, aging, and emergency
int calculateDynamicPriority(const TrafficTask& task, int currentTime, const TrafficConfig& cfg) {
    int wait = std::max(0, currentTime - task.arrivalTime);
    // Base priority + 2 points per vehicle + aging bonus per unit wait
    int prio = task.priority + (task.waitingVehicles * 2) + (wait * cfg.agingRate);
    if (task.hasEmergency) {
        prio += cfg.emergencyPriorityBoost;
    }
    return prio;
}

// Performance metrics evaluation
void calculateTrafficMetrics(TrafficScheduleResult& result) {
    if (result.completedTasks.empty()) {
        result.avgVehicleWaitingTime = 0.0;
        result.maxWaitingTime = 0;
        result.totalVehiclesServed = 0;
        result.emergencyAvgWaitingTime = 0.0;
        result.deadlineMisses = 0;
        result.trafficThroughput = 0.0;
        result.signalUtilization = 0.0;
        result.totalSimulationTime = 0;
        result.totalGreenTime = 0;
        result.totalClearanceTime = 0;
        return;
    }

    double totalVehicleDelay = 0.0;
    int maxWait = 0;
    int totalVehicles = 0;
    double emergencyTotalWait = 0.0;
    int emergencyCount = 0;
    int deadlineMisses = 0;

    for (const auto& t : result.completedTasks) {
        int wait = t.waitingTime;
        if (wait > maxWait) maxWait = wait;
        totalVehicles += t.waitingVehicles;
        totalVehicleDelay += (static_cast<double>(wait) * t.waitingVehicles);

        if (t.hasEmergency) {
            emergencyTotalWait += wait;
            emergencyCount++;
        }
        if (t.missedDeadline) {
            deadlineMisses++;
        }
    }

    result.totalVehiclesServed = totalVehicles;
    result.maxWaitingTime = maxWait;
    result.avgVehicleWaitingTime = (totalVehicles > 0) ? (totalVehicleDelay / totalVehicles) : 0.0;
    result.emergencyAvgWaitingTime = (emergencyCount > 0) ? (emergencyTotalWait / emergencyCount) : 0.0;
    result.deadlineMisses = deadlineMisses;

    // Timeline durations
    int greenDuration = 0;
    int clearanceDuration = 0;
    int simEnd = 0;

    for (const auto& interval : result.signalTimeline) {
        if (interval.signalState == "GREEN") {
            greenDuration += interval.duration();
        } else if (interval.signalState == "YELLOW" || interval.signalState == "ALL_RED") {
            clearanceDuration += interval.duration();
        }
        if (interval.endTime > simEnd) {
            simEnd = interval.endTime;
        }
    }

    result.totalGreenTime = greenDuration;
    result.totalClearanceTime = clearanceDuration;
    result.totalSimulationTime = simEnd;

    if (result.totalSimulationTime > 0) {
        result.trafficThroughput = static_cast<double>(totalVehicles) / result.totalSimulationTime;
        result.signalUtilization = (static_cast<double>(greenDuration) / result.totalSimulationTime) * 100.0;
    } else {
        result.trafficThroughput = 0.0;
        result.signalUtilization = 0.0;
    }
}

// Helper: Applies clearance interval (Yellow -> All-Red) when switching roads safely
void applyClearance(const std::string& fromRoad, const std::string& toRoad, int& currentTime,
                    std::vector<SignalInterval>& timeline, const TrafficConfig& cfg) {
    if (!fromRoad.empty() && fromRoad != toRoad) {
        if (cfg.yellowDuration > 0) {
            timeline.emplace_back(fromRoad, "YELLOW", currentTime, currentTime + cfg.yellowDuration, 0);
            currentTime += cfg.yellowDuration;
        }
        if (cfg.allRedDuration > 0) {
            timeline.emplace_back("ALL", "ALL_RED", currentTime, currentTime + cfg.allRedDuration, 0);
            currentTime += cfg.allRedDuration;
        }
    }
}

// Helper: Finds an uncompleted emergency task arrived at or before checkTime
int findEmergencyTask(const std::vector<TrafficTask>& tasks, const std::vector<bool>& finished, int checkTime) {
    int bestIdx = -1;
    int earliestArrival = 1e9;
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (!finished[i] && tasks[i].hasEmergency && tasks[i].arrivalTime <= checkTime) {
            if (tasks[i].arrivalTime < earliestArrival) {
                earliestArrival = tasks[i].arrivalTime;
                bestIdx = static_cast<int>(i);
            }
        }
    }
    return bestIdx;
}

// Helper: Checks if an emergency vehicle arrives midway during a planned green phase
int checkMidwayEmergencyPreemption(const std::vector<TrafficTask>& tasks, const std::vector<bool>& finished,
                                   int currentTime, int plannedDuration, int minGreen) {
    int adjustedDuration = plannedDuration;
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (!finished[i] && tasks[i].hasEmergency && tasks[i].arrivalTime > currentTime && tasks[i].arrivalTime < currentTime + plannedDuration) {
            int preemptTime = std::max(currentTime + minGreen, tasks[i].arrivalTime);
            if (preemptTime < currentTime + adjustedDuration) {
                adjustedDuration = preemptTime - currentTime;
            }
        }
    }
    return adjustedDuration;
}

// -------------------------------------------------------------
// 1. Traffic FCFS Scheduling (With Universal Emergency Preemption)
// -------------------------------------------------------------
TrafficScheduleResult scheduleTrafficFCFS(std::vector<TrafficTask> tasks, TrafficConfig cfg) {
    TrafficScheduleResult result;
    result.algorithmName = "Traffic FCFS (with Universal Emergency Preemption)";
    if (tasks.empty()) return result;

    for (auto& t : tasks) {
        t.remainingServiceTime = t.estimatedServiceTime;
        t.firstGreenTime = -1;
        t.vehiclesServed = 0;
        t.missedDeadline = false;
    }

    int n = static_cast<int>(tasks.size());
    std::vector<bool> finished(n, false);
    int completedCount = 0;
    int currentTime = 0;
    std::string lastActiveRoad = "";

    while (completedCount < n) {
        int bestIdx = -1;

        // Universal Emergency Check: If any emergency vehicle is waiting, prioritize immediately!
        bestIdx = findEmergencyTask(tasks, finished, currentTime);

        if (bestIdx == -1) {
            // Standard FCFS: Select arrived task with earliest arrival time
            int earliestArrival = 1e9;
            for (int i = 0; i < n; ++i) {
                if (!finished[i] && tasks[i].arrivalTime <= currentTime) {
                    if (tasks[i].arrivalTime < earliestArrival) {
                        earliestArrival = tasks[i].arrivalTime;
                        bestIdx = i;
                    } else if (tasks[i].arrivalTime == earliestArrival && tasks[i].taskId < tasks[bestIdx].taskId) {
                        bestIdx = i;
                    }
                }
            }
        }

        if (bestIdx == -1) {
            int nextArrival = 1e9;
            for (int i = 0; i < n; ++i) {
                if (!finished[i] && tasks[i].arrivalTime < nextArrival) {
                    nextArrival = tasks[i].arrivalTime;
                }
            }
            if (nextArrival > currentTime) {
                result.signalTimeline.emplace_back("ALL", "ALL_RED", currentTime, nextArrival, 0);
                currentTime = nextArrival;
            }
            lastActiveRoad = "";
            continue;
        }

        TrafficTask& task = tasks[bestIdx];

        // Safe clearance transition if road switches
        applyClearance(lastActiveRoad, task.roadName, currentTime, result.signalTimeline, cfg);

        if (task.firstGreenTime == -1) {
            task.firstGreenTime = currentTime;
        }

        int greenDuration = std::max(cfg.minGreenTime, task.remainingServiceTime);

        // Preemption check: If normal task, check if emergency arrives midway
        if (!task.hasEmergency) {
            greenDuration = checkMidwayEmergencyPreemption(tasks, finished, currentTime, greenDuration, cfg.minGreenTime);
        }

        int vehiclesCleared = std::min(task.waitingVehicles - task.vehiclesServed, greenDuration / cfg.serviceRatePerVehicle);
        if (vehiclesCleared <= 0) vehiclesCleared = 1;

        result.signalTimeline.emplace_back(task.roadName, "GREEN", currentTime, currentTime + greenDuration, vehiclesCleared);
        currentTime += greenDuration;
        task.remainingServiceTime -= greenDuration;
        task.vehiclesServed += vehiclesCleared;

        if (task.remainingServiceTime <= 0 || task.vehiclesServed >= task.waitingVehicles) {
            task.completionTime = currentTime;
            task.waitingTime = task.completionTime - task.arrivalTime - task.estimatedServiceTime;
            if (task.waitingTime < 0) task.waitingTime = 0;
            if (task.completionTime > task.deadline) {
                task.missedDeadline = true;
            }
            finished[bestIdx] = true;
            completedCount++;
        }

        lastActiveRoad = task.roadName;
    }

    result.completedTasks = tasks;
    calculateTrafficMetrics(result);
    return result;
}

// -------------------------------------------------------------
// 2. Traffic Priority Scheduling (Density, Wait, Aging, Emergency)
// -------------------------------------------------------------
TrafficScheduleResult scheduleTrafficPriority(std::vector<TrafficTask> tasks, TrafficConfig cfg) {
    TrafficScheduleResult result;
    result.algorithmName = "Traffic Priority (Density + Wait + Aging + Emergency Preemption)";
    if (tasks.empty()) return result;

    for (auto& t : tasks) {
        t.remainingServiceTime = t.estimatedServiceTime;
        t.firstGreenTime = -1;
        t.vehiclesServed = 0;
        t.missedDeadline = false;
    }

    int n = static_cast<int>(tasks.size());
    std::vector<bool> finished(n, false);
    int completedCount = 0;
    int currentTime = 0;
    std::string lastActiveRoad = "";

    while (completedCount < n) {
        int bestIdx = -1;
        int highestPrio = -1e9;

        // Check for arrived tasks and calculate dynamic priority including aging
        for (int i = 0; i < n; ++i) {
            if (!finished[i] && tasks[i].arrivalTime <= currentTime) {
                int p = calculateDynamicPriority(tasks[i], currentTime, cfg);
                if (p > highestPrio) {
                    highestPrio = p;
                    bestIdx = i;
                } else if (p == highestPrio) {
                    if (tasks[i].arrivalTime < tasks[bestIdx].arrivalTime) {
                        bestIdx = i;
                    } else if (tasks[i].arrivalTime == tasks[bestIdx].arrivalTime && tasks[i].taskId < tasks[bestIdx].taskId) {
                        bestIdx = i;
                    }
                }
            }
        }

        if (bestIdx == -1) {
            int nextArrival = 1e9;
            for (int i = 0; i < n; ++i) {
                if (!finished[i] && tasks[i].arrivalTime < nextArrival) {
                    nextArrival = tasks[i].arrivalTime;
                }
            }
            if (nextArrival > currentTime) {
                result.signalTimeline.emplace_back("ALL", "ALL_RED", currentTime, nextArrival, 0);
                currentTime = nextArrival;
            }
            lastActiveRoad = "";
            continue;
        }

        TrafficTask& task = tasks[bestIdx];

        // Safe clearance transition if road switches
        applyClearance(lastActiveRoad, task.roadName, currentTime, result.signalTimeline, cfg);

        if (task.firstGreenTime == -1) {
            task.firstGreenTime = currentTime;
        }

        int greenDuration = std::max(cfg.minGreenTime, std::min(cfg.maxGreenTime, task.remainingServiceTime));
        
        // Mid-cycle emergency preemption check
        if (!task.hasEmergency) {
            greenDuration = checkMidwayEmergencyPreemption(tasks, finished, currentTime, greenDuration, cfg.minGreenTime);
        }

        int vehiclesCleared = std::min(task.waitingVehicles - task.vehiclesServed, greenDuration / cfg.serviceRatePerVehicle);
        if (vehiclesCleared <= 0) vehiclesCleared = 1;

        result.signalTimeline.emplace_back(task.roadName, "GREEN", currentTime, currentTime + greenDuration, vehiclesCleared);
        currentTime += greenDuration;
        task.remainingServiceTime -= greenDuration;
        task.vehiclesServed += vehiclesCleared;

        if (task.remainingServiceTime <= 0 || task.vehiclesServed >= task.waitingVehicles) {
            task.completionTime = currentTime;
            task.waitingTime = task.completionTime - task.arrivalTime - task.estimatedServiceTime;
            if (task.waitingTime < 0) task.waitingTime = 0;
            if (task.completionTime > task.deadline) {
                task.missedDeadline = true;
            }
            finished[bestIdx] = true;
            completedCount++;
        }

        lastActiveRoad = task.roadName;
    }

    result.completedTasks = tasks;
    calculateTrafficMetrics(result);
    return result;
}

// -------------------------------------------------------------
// 3. Traffic Round Robin (With Universal Emergency Preemption)
// -------------------------------------------------------------
TrafficScheduleResult scheduleTrafficRoundRobin(std::vector<TrafficTask> tasks, int timeQuantum, TrafficConfig cfg) {
    TrafficScheduleResult result;
    result.algorithmName = "Traffic Round Robin (with Universal Emergency Preemption)";
    result.timeQuantum = timeQuantum;
    if (tasks.empty()) return result;
    if (timeQuantum <= 0) timeQuantum = cfg.minGreenTime;

    for (auto& t : tasks) {
        t.remainingServiceTime = t.estimatedServiceTime;
        t.firstGreenTime = -1;
        t.vehiclesServed = 0;
        t.missedDeadline = false;
    }

    int n = static_cast<int>(tasks.size());
    std::vector<bool> inQueue(n, false);
    std::vector<bool> finished(n, false);
    std::queue<int> readyQueue;
    int completedCount = 0;
    int currentTime = 0;
    std::string lastActiveRoad = "";

    auto enqueueArrived = [&](int timeLimit) {
        std::vector<int> newlyArrived;
        for (int i = 0; i < n; ++i) {
            if (!inQueue[i] && !finished[i] && tasks[i].remainingServiceTime > 0 && tasks[i].arrivalTime <= timeLimit) {
                newlyArrived.push_back(i);
            }
        }
        std::sort(newlyArrived.begin(), newlyArrived.end(), [&](int a, int b) {
            if (tasks[a].arrivalTime != tasks[b].arrivalTime)
                return tasks[a].arrivalTime < tasks[b].arrivalTime;
            return tasks[a].taskId < tasks[b].taskId;
        });
        for (int idx : newlyArrived) {
            readyQueue.push(idx);
            inQueue[idx] = true;
        }
    };

    enqueueArrived(currentTime);

    while (completedCount < n) {
        // Universal Emergency Preemption Check:
        int emergIdx = findEmergencyTask(tasks, finished, currentTime);
        int currentIdx = -1;

        if (emergIdx != -1) {
            currentIdx = emergIdx;
            inQueue[emergIdx] = true;
        } else {
            if (readyQueue.empty()) {
                int nextArrival = 1e9;
                for (int i = 0; i < n; ++i) {
                    if (tasks[i].remainingServiceTime > 0 && !inQueue[i] && !finished[i] && tasks[i].arrivalTime < nextArrival) {
                        nextArrival = tasks[i].arrivalTime;
                    }
                }
                if (nextArrival < 1e9 && nextArrival > currentTime) {
                    result.signalTimeline.emplace_back("ALL", "ALL_RED", currentTime, nextArrival, 0);
                    currentTime = nextArrival;
                    enqueueArrived(currentTime);
                }
                lastActiveRoad = "";
                if (readyQueue.empty()) break;
            }

            currentIdx = readyQueue.front();
            readyQueue.pop();

            // Skip if this task was already serviced (e.g., via emergency preemption)
            if (finished[currentIdx]) {
                continue;
            }
        }

        TrafficTask& task = tasks[currentIdx];

        // Safe clearance transition if road switches
        applyClearance(lastActiveRoad, task.roadName, currentTime, result.signalTimeline, cfg);

        if (task.firstGreenTime == -1) {
            task.firstGreenTime = currentTime;
        }

        int runDuration = task.hasEmergency ? std::max(cfg.minGreenTime, task.remainingServiceTime) : std::min(timeQuantum, task.remainingServiceTime);
        if (runDuration < cfg.minGreenTime && task.remainingServiceTime >= cfg.minGreenTime) {
            runDuration = cfg.minGreenTime;
        }

        // Check if an emergency vehicle arrives midway during normal RR green slice
        if (!task.hasEmergency) {
            runDuration = checkMidwayEmergencyPreemption(tasks, finished, currentTime, runDuration, cfg.minGreenTime);
        }

        int vehiclesCleared = std::min(task.waitingVehicles - task.vehiclesServed, runDuration / cfg.serviceRatePerVehicle);
        if (vehiclesCleared <= 0) vehiclesCleared = 1;

        result.signalTimeline.emplace_back(task.roadName, "GREEN", currentTime, currentTime + runDuration, vehiclesCleared);
        currentTime += runDuration;
        task.remainingServiceTime -= runDuration;
        task.vehiclesServed += vehiclesCleared;

        lastActiveRoad = task.roadName;

        // Enqueue any tasks that arrived during the green phase
        enqueueArrived(currentTime);

        if (task.remainingServiceTime > 0) {
            if (!task.hasEmergency) {
                readyQueue.push(currentIdx);
            }
        } else {
            task.completionTime = currentTime;
            task.waitingTime = task.completionTime - task.arrivalTime - task.estimatedServiceTime;
            if (task.waitingTime < 0) task.waitingTime = 0;
            if (task.completionTime > task.deadline) {
                task.missedDeadline = true;
            }
            finished[currentIdx] = true;
            completedCount++;
        }
    }

    result.completedTasks = tasks;
    calculateTrafficMetrics(result);
    return result;
}

// -------------------------------------------------------------
// 4. Traffic Earliest Deadline First (EDF) (With Universal Emergency Preemption)
// -------------------------------------------------------------
TrafficScheduleResult scheduleTrafficEDF(std::vector<TrafficTask> tasks, TrafficConfig cfg) {
    TrafficScheduleResult result;
    result.algorithmName = "Traffic Earliest Deadline First (with Universal Emergency Preemption)";
    if (tasks.empty()) return result;

    for (auto& t : tasks) {
        t.remainingServiceTime = t.estimatedServiceTime;
        t.firstGreenTime = -1;
        t.vehiclesServed = 0;
        t.missedDeadline = false;
    }

    int n = static_cast<int>(tasks.size());
    std::vector<bool> finished(n, false);
    int completedCount = 0;
    int currentTime = 0;
    std::string lastActiveRoad = "";

    while (completedCount < n) {
        int bestIdx = -1;
        int earliestDeadline = 1e9;

        for (int i = 0; i < n; ++i) {
            if (!finished[i] && tasks[i].arrivalTime <= currentTime) {
                // Emergency vehicles get highest urgency
                int effectiveDeadline = tasks[i].hasEmergency ? -1000 : tasks[i].deadline;
                if (effectiveDeadline < earliestDeadline) {
                    earliestDeadline = effectiveDeadline;
                    bestIdx = i;
                } else if (effectiveDeadline == earliestDeadline) {
                    if (tasks[i].waitingVehicles > tasks[bestIdx].waitingVehicles) {
                        bestIdx = i;
                    } else if (tasks[i].arrivalTime < tasks[bestIdx].arrivalTime) {
                        bestIdx = i;
                    }
                }
            }
        }

        if (bestIdx == -1) {
            int nextArrival = 1e9;
            for (int i = 0; i < n; ++i) {
                if (!finished[i] && tasks[i].arrivalTime < nextArrival) {
                    nextArrival = tasks[i].arrivalTime;
                }
            }
            if (nextArrival > currentTime) {
                result.signalTimeline.emplace_back("ALL", "ALL_RED", currentTime, nextArrival, 0);
                currentTime = nextArrival;
            }
            lastActiveRoad = "";
            continue;
        }

        TrafficTask& task = tasks[bestIdx];

        // Safe clearance transition if road switches
        applyClearance(lastActiveRoad, task.roadName, currentTime, result.signalTimeline, cfg);

        if (task.firstGreenTime == -1) {
            task.firstGreenTime = currentTime;
        }

        int greenDuration = std::max(cfg.minGreenTime, task.remainingServiceTime);

        // Preemption check: If normal task, check if emergency arrives midway
        if (!task.hasEmergency) {
            greenDuration = checkMidwayEmergencyPreemption(tasks, finished, currentTime, greenDuration, cfg.minGreenTime);
        }

        int vehiclesCleared = std::min(task.waitingVehicles - task.vehiclesServed, greenDuration / cfg.serviceRatePerVehicle);
        if (vehiclesCleared <= 0) vehiclesCleared = 1;

        result.signalTimeline.emplace_back(task.roadName, "GREEN", currentTime, currentTime + greenDuration, vehiclesCleared);
        currentTime += greenDuration;
        task.remainingServiceTime -= greenDuration;
        task.vehiclesServed += vehiclesCleared;

        if (task.remainingServiceTime <= 0 || task.vehiclesServed >= task.waitingVehicles) {
            task.completionTime = currentTime;
            task.waitingTime = task.completionTime - task.arrivalTime - task.estimatedServiceTime;
            if (task.waitingTime < 0) task.waitingTime = 0;
            if (task.completionTime > task.deadline) {
                task.missedDeadline = true;
            }
            finished[bestIdx] = true;
            completedCount++;
        }

        lastActiveRoad = task.roadName;
    }

    result.completedTasks = tasks;
    calculateTrafficMetrics(result);
    return result;
}

// -------------------------------------------------------------
// Visual and JSON Reporting Utilities
// -------------------------------------------------------------
void TrafficScheduleResult::printTimeline() const {
    std::cout << "\n========================================================================================\n";
    std::cout << "  TRAFFIC SIGNAL EXECUTION TIMELINE: " << algorithmName << "\n";
    std::cout << "========================================================================================\n";
    std::cout << std::left
              << std::setw(10) << "Time"
              << std::setw(12) << "Road"
              << std::setw(12) << "Signal"
              << std::setw(12) << "Duration"
              << std::setw(16) << "Vehicles Cleared"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    for (const auto& inter : signalTimeline) {
        std::string timeStr = "[" + std::to_string(inter.startTime) + " -> " + std::to_string(inter.endTime) + "]";
        std::cout << std::left
                  << std::setw(10) << timeStr
                  << std::setw(12) << inter.roadName
                  << std::setw(12) << inter.signalState
                  << std::setw(12) << (std::to_string(inter.duration()) + "s")
                  << std::setw(16) << inter.vehiclesPassed
                  << "\n";
    }
    std::cout << "----------------------------------------------------------------------------------------\n";
}

void TrafficScheduleResult::printTaskReport() const {
    std::cout << "\nTask Completion & Delay Report:\n";
    std::cout << std::left
              << std::setw(6)  << "TID"
              << std::setw(10) << "Road"
              << std::setw(10) << "Arrival"
              << std::setw(10) << "Vehicles"
              << std::setw(10) << "Emergency"
              << std::setw(10) << "Deadline"
              << std::setw(12) << "Completion"
              << std::setw(10) << "Delay/WT"
              << std::setw(12) << "Status"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    for (const auto& t : completedTasks) {
        std::cout << std::left
                  << std::setw(6)  << ("T" + std::to_string(t.taskId))
                  << std::setw(10) << t.roadName
                  << std::setw(10) << t.arrivalTime
                  << std::setw(10) << t.waitingVehicles
                  << std::setw(10) << (t.hasEmergency ? "YES [!]" : "No")
                  << std::setw(10) << t.deadline
                  << std::setw(12) << t.completionTime
                  << std::setw(10) << t.waitingTime
                  << std::setw(12) << (t.missedDeadline ? "MISSED" : "MET")
                  << "\n";
    }
    std::cout << "----------------------------------------------------------------------------------------\n";
}

void TrafficScheduleResult::printMetrics() const {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nTraffic Performance Metrics Summary:\n";
    std::cout << "  - Total Vehicles Served       : " << totalVehiclesServed << "\n";
    std::cout << "  - Average Vehicle Delay (WT)  : " << avgVehicleWaitingTime << " s/veh\n";
    std::cout << "  - Maximum Queue Wait Time     : " << maxWaitingTime << " s\n";
    std::cout << "  - Emergency Avg Waiting Time  : " << emergencyAvgWaitingTime << " s\n";
    std::cout << "  - Number of Deadline Misses   : " << deadlineMisses << "\n";
    std::cout << "  - Traffic Throughput          : " << trafficThroughput << " vehicles/s\n";
    std::cout << "  - Green Signal Utilization    : " << signalUtilization << " %\n";
    std::cout << "  - Total Simulation Time       : " << totalSimulationTime << " s\n";
    std::cout << "  - Total Clearance Time        : " << totalClearanceTime << " s (Yellow + All-Red)\n";
    std::cout << "========================================================================================\n";
}

std::string TrafficScheduleResult::toJson() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{\n";
    ss << "  \"algorithm\": \"" << algorithmName << "\",\n";
    ss << "  \"timeQuantum\": " << timeQuantum << ",\n";
    ss << "  \"totalVehiclesServed\": " << totalVehiclesServed << ",\n";
    ss << "  \"avgVehicleWaitingTime\": " << avgVehicleWaitingTime << ",\n";
    ss << "  \"maxWaitingTime\": " << maxWaitingTime << ",\n";
    ss << "  \"emergencyAvgWaitingTime\": " << emergencyAvgWaitingTime << ",\n";
    ss << "  \"deadlineMisses\": " << deadlineMisses << ",\n";
    ss << "  \"trafficThroughput\": " << trafficThroughput << ",\n";
    ss << "  \"signalUtilization\": " << signalUtilization << ",\n";
    ss << "  \"totalSimulationTime\": " << totalSimulationTime << ",\n";
    ss << "  \"totalClearanceTime\": " << totalClearanceTime << ",\n";

    // Tasks array
    ss << "  \"tasks\": [\n";
    for (size_t i = 0; i < completedTasks.size(); ++i) {
        const auto& t = completedTasks[i];
        ss << "    {\"taskId\": " << t.taskId
           << ", \"roadName\": \"" << t.roadName << "\""
           << ", \"arrivalTime\": " << t.arrivalTime
           << ", \"waitingVehicles\": " << t.waitingVehicles
           << ", \"hasEmergency\": " << (t.hasEmergency ? "true" : "false")
           << ", \"deadline\": " << t.deadline
           << ", \"completionTime\": " << t.completionTime
           << ", \"waitingTime\": " << t.waitingTime
           << ", \"missedDeadline\": " << (t.missedDeadline ? "true" : "false") << "}";
        if (i + 1 < completedTasks.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    // Signal timeline array
    ss << "  \"signalTimeline\": [\n";
    for (size_t i = 0; i < signalTimeline.size(); ++i) {
        const auto& st = signalTimeline[i];
        ss << "    {\"roadName\": \"" << st.roadName << "\""
           << ", \"signalState\": \"" << st.signalState << "\""
           << ", \"startTime\": " << st.startTime
           << ", \"endTime\": " << st.endTime
           << ", \"duration\": " << st.duration()
           << ", \"vehiclesPassed\": " << st.vehiclesPassed << "}";
        if (i + 1 < signalTimeline.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}";
    return ss.str();
}
