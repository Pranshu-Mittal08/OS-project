#include "cpu_scheduler.h"
#include <algorithm>
#include <queue>
#include <iomanip>

// Merges adjacent Gantt intervals belonging to the same process or consecutive idle slices
std::vector<GanttSegment> compressGanttChart(const std::vector<GanttSegment>& rawGantt) {
    std::vector<GanttSegment> compressed;
    if (rawGantt.empty()) return compressed;

    for (const auto& seg : rawGantt) {
        if (seg.startTime == seg.endTime) continue; // Skip zero-duration slices
        if (!compressed.empty() && compressed.back().processId == seg.processId && compressed.back().endTime == seg.startTime) {
            compressed.back().endTime = seg.endTime;
        } else {
            compressed.push_back(seg);
        }
    }
    return compressed;
}

// Computes summary performance metrics for a completed schedule
void calculateMetrics(CPUScheduleResult& result) {
    if (result.processes.empty()) {
        result.avgWaitingTime = 0.0;
        result.avgTurnaroundTime = 0.0;
        result.avgResponseTime = 0.0;
        result.cpuUtilization = 0.0;
        result.throughput = 0.0;
        result.totalTime = 0;
        result.idleTime = 0;
        return;
    }

    double totalWT = 0.0;
    double totalTAT = 0.0;
    double totalRT = 0.0;
    int totalBusyTime = 0;
    int maxCompletion = 0;

    for (const auto& p : result.processes) {
        totalWT += p.waitingTime;
        totalTAT += p.turnaroundTime;
        totalRT += p.responseTime;
        totalBusyTime += p.burstTime;
        if (p.completionTime > maxCompletion) {
            maxCompletion = p.completionTime;
        }
    }

    int n = static_cast<int>(result.processes.size());
    result.avgWaitingTime = totalWT / n;
    result.avgTurnaroundTime = totalTAT / n;
    result.avgResponseTime = totalRT / n;
    result.totalTime = maxCompletion;

    // Calculate idle time from gantt segments
    int idle = 0;
    for (const auto& seg : result.ganttChart) {
        if (seg.processId == -1) {
            idle += (seg.endTime - seg.startTime);
        }
    }
    result.idleTime = idle;

    if (result.totalTime > 0) {
        result.cpuUtilization = (static_cast<double>(totalBusyTime) / result.totalTime) * 100.0;
        result.throughput = static_cast<double>(n) / result.totalTime;
    } else {
        result.cpuUtilization = 100.0;
        result.throughput = 0.0;
    }
}

// -------------------------------------------------------------
// 1. First Come First Serve (FCFS) - Non-preemptive
// -------------------------------------------------------------
CPUScheduleResult scheduleFCFS(std::vector<Process> processes) {
    CPUScheduleResult result;
    result.algorithmName = "FCFS (First Come First Serve)";
    if (processes.empty()) return result;

    // Sort by arrival time; tie-breaker: process ID
    std::sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        if (a.arrivalTime != b.arrivalTime) return a.arrivalTime < b.arrivalTime;
        return a.id < b.id;
    });

    std::vector<GanttSegment> rawGantt;
    int currentTime = 0;

    for (auto& p : processes) {
        if (currentTime < p.arrivalTime) {
            // CPU is idle
            rawGantt.emplace_back(-1, currentTime, p.arrivalTime);
            currentTime = p.arrivalTime;
        }

        p.firstStartTime = currentTime;
        p.responseTime = p.firstStartTime - p.arrivalTime;

        rawGantt.emplace_back(p.id, currentTime, currentTime + p.burstTime);
        currentTime += p.burstTime;

        p.completionTime = currentTime;
        p.turnaroundTime = p.completionTime - p.arrivalTime;
        p.waitingTime = p.turnaroundTime - p.burstTime;
        p.remainingBurstTime = 0;
    }

    result.processes = processes;
    result.ganttChart = compressGanttChart(rawGantt);
    calculateMetrics(result);
    return result;
}

// -------------------------------------------------------------
// 2. Shortest Job First (SJF) - Non-preemptive
// -------------------------------------------------------------
CPUScheduleResult scheduleSJF(std::vector<Process> processes) {
    CPUScheduleResult result;
    result.algorithmName = "SJF (Shortest Job First - Non-Preemptive)";
    if (processes.empty()) return result;

    int n = static_cast<int>(processes.size());
    std::vector<bool> isCompleted(n, false);
    std::vector<GanttSegment> rawGantt;
    int completedCount = 0;
    int currentTime = 0;

    while (completedCount < n) {
        int bestIdx = -1;
        int minBurst = 1e9;

        for (int i = 0; i < n; ++i) {
            if (!isCompleted[i] && processes[i].arrivalTime <= currentTime) {
                if (processes[i].burstTime < minBurst) {
                    minBurst = processes[i].burstTime;
                    bestIdx = i;
                } else if (processes[i].burstTime == minBurst) {
                    // Tie-breaker: earlier arrival time, then smaller ID
                    if (processes[i].arrivalTime < processes[bestIdx].arrivalTime) {
                        bestIdx = i;
                    } else if (processes[i].arrivalTime == processes[bestIdx].arrivalTime && processes[i].id < processes[bestIdx].id) {
                        bestIdx = i;
                    }
                }
            }
        }

        if (bestIdx == -1) {
            // No process has arrived yet; CPU is idle until next arrival
            int nextArrival = 1e9;
            for (int i = 0; i < n; ++i) {
                if (!isCompleted[i] && processes[i].arrivalTime < nextArrival) {
                    nextArrival = processes[i].arrivalTime;
                }
            }
            rawGantt.emplace_back(-1, currentTime, nextArrival);
            currentTime = nextArrival;
            continue;
        }

        // Execute selected process non-preemptively
        Process& p = processes[bestIdx];
        p.firstStartTime = currentTime;
        p.responseTime = p.firstStartTime - p.arrivalTime;

        rawGantt.emplace_back(p.id, currentTime, currentTime + p.burstTime);
        currentTime += p.burstTime;

        p.completionTime = currentTime;
        p.turnaroundTime = p.completionTime - p.arrivalTime;
        p.waitingTime = p.turnaroundTime - p.burstTime;
        p.remainingBurstTime = 0;

        isCompleted[bestIdx] = true;
        completedCount++;
    }

    result.processes = processes;
    result.ganttChart = compressGanttChart(rawGantt);
    calculateMetrics(result);
    return result;
}

// -------------------------------------------------------------
// 3. Priority Scheduling - Non-preemptive (Lower number = Higher priority)
// -------------------------------------------------------------
CPUScheduleResult schedulePriorityNonPreemptive(std::vector<Process> processes) {
    CPUScheduleResult result;
    result.algorithmName = "Priority Scheduling (Non-Preemptive)";
    if (processes.empty()) return result;

    int n = static_cast<int>(processes.size());
    std::vector<bool> isCompleted(n, false);
    std::vector<GanttSegment> rawGantt;
    int completedCount = 0;
    int currentTime = 0;

    while (completedCount < n) {
        int bestIdx = -1;
        int highestPriority = 1e9;

        for (int i = 0; i < n; ++i) {
            if (!isCompleted[i] && processes[i].arrivalTime <= currentTime) {
                if (processes[i].priority < highestPriority) {
                    highestPriority = processes[i].priority;
                    bestIdx = i;
                } else if (processes[i].priority == highestPriority) {
                    // Tie-breaker: earlier arrival time, then smaller ID
                    if (processes[i].arrivalTime < processes[bestIdx].arrivalTime) {
                        bestIdx = i;
                    } else if (processes[i].arrivalTime == processes[bestIdx].arrivalTime && processes[i].id < processes[bestIdx].id) {
                        bestIdx = i;
                    }
                }
            }
        }

        if (bestIdx == -1) {
            int nextArrival = 1e9;
            for (int i = 0; i < n; ++i) {
                if (!isCompleted[i] && processes[i].arrivalTime < nextArrival) {
                    nextArrival = processes[i].arrivalTime;
                }
            }
            rawGantt.emplace_back(-1, currentTime, nextArrival);
            currentTime = nextArrival;
            continue;
        }

        Process& p = processes[bestIdx];
        p.firstStartTime = currentTime;
        p.responseTime = p.firstStartTime - p.arrivalTime;

        rawGantt.emplace_back(p.id, currentTime, currentTime + p.burstTime);
        currentTime += p.burstTime;

        p.completionTime = currentTime;
        p.turnaroundTime = p.completionTime - p.arrivalTime;
        p.waitingTime = p.turnaroundTime - p.burstTime;
        p.remainingBurstTime = 0;

        isCompleted[bestIdx] = true;
        completedCount++;
    }

    result.processes = processes;
    result.ganttChart = compressGanttChart(rawGantt);
    calculateMetrics(result);
    return result;
}

// -------------------------------------------------------------
// 4. Priority Scheduling - Preemptive (Lower number = Higher priority)
// -------------------------------------------------------------
CPUScheduleResult schedulePriorityPreemptive(std::vector<Process> processes) {
    CPUScheduleResult result;
    result.algorithmName = "Priority Scheduling (Preemptive)";
    if (processes.empty()) return result;

    int n = static_cast<int>(processes.size());
    for (auto& p : processes) {
        p.remainingBurstTime = p.burstTime;
        p.firstStartTime = -1;
    }

    std::vector<GanttSegment> rawGantt;
    int completedCount = 0;
    int currentTime = 0;

    while (completedCount < n) {
        int bestIdx = -1;
        int highestPriority = 1e9;

        for (int i = 0; i < n; ++i) {
            if (processes[i].remainingBurstTime > 0 && processes[i].arrivalTime <= currentTime) {
                if (processes[i].priority < highestPriority) {
                    highestPriority = processes[i].priority;
                    bestIdx = i;
                } else if (processes[i].priority == highestPriority) {
                    // Tie-breaker: earlier arrival, then smaller ID
                    if (processes[i].arrivalTime < processes[bestIdx].arrivalTime) {
                        bestIdx = i;
                    } else if (processes[i].arrivalTime == processes[bestIdx].arrivalTime && processes[i].id < processes[bestIdx].id) {
                        bestIdx = i;
                    }
                }
            }
        }

        if (bestIdx == -1) {
            int nextArrival = 1e9;
            for (int i = 0; i < n; ++i) {
                if (processes[i].remainingBurstTime > 0 && processes[i].arrivalTime < nextArrival) {
                    nextArrival = processes[i].arrivalTime;
                }
            }
            rawGantt.emplace_back(-1, currentTime, nextArrival);
            currentTime = nextArrival;
            continue;
        }

        // Execute for 1 time unit
        Process& p = processes[bestIdx];
        if (p.firstStartTime == -1) {
            p.firstStartTime = currentTime;
            p.responseTime = p.firstStartTime - p.arrivalTime;
        }

        rawGantt.emplace_back(p.id, currentTime, currentTime + 1);
        p.remainingBurstTime--;
        currentTime++;

        if (p.remainingBurstTime == 0) {
            p.completionTime = currentTime;
            p.turnaroundTime = p.completionTime - p.arrivalTime;
            p.waitingTime = p.turnaroundTime - p.burstTime;
            completedCount++;
        }
    }

    result.processes = processes;
    result.ganttChart = compressGanttChart(rawGantt);
    calculateMetrics(result);
    return result;
}

// -------------------------------------------------------------
// 5. Round Robin (RR) - Preemptive with configurable time quantum
// -------------------------------------------------------------
CPUScheduleResult scheduleRoundRobin(std::vector<Process> processes, int timeQuantum) {
    CPUScheduleResult result;
    result.algorithmName = "Round Robin (RR)";
    result.timeQuantum = timeQuantum;
    if (processes.empty()) return result;
    if (timeQuantum <= 0) timeQuantum = 1; // Safeguard

    int n = static_cast<int>(processes.size());
    for (auto& p : processes) {
        p.remainingBurstTime = p.burstTime;
        p.firstStartTime = -1;
    }

    // Keep track of which processes have been pushed to queue
    std::vector<bool> inQueue(n, false);
    std::queue<int> readyQueue;
    std::vector<GanttSegment> rawGantt;

    int currentTime = 0;
    int completedCount = 0;

    // Helper to check and enqueue newly arrived processes up to time t
    auto enqueueArrived = [&](int timeLimit) {
        // Enqueue processes sorted by arrival time, then ID
        std::vector<int> newlyArrived;
        for (int i = 0; i < n; ++i) {
            if (!inQueue[i] && processes[i].remainingBurstTime > 0 && processes[i].arrivalTime <= timeLimit) {
                newlyArrived.push_back(i);
            }
        }
        std::sort(newlyArrived.begin(), newlyArrived.end(), [&](int a, int b) {
            if (processes[a].arrivalTime != processes[b].arrivalTime)
                return processes[a].arrivalTime < processes[b].arrivalTime;
            return processes[a].id < processes[b].id;
        });
        for (int idx : newlyArrived) {
            readyQueue.push(idx);
            inQueue[idx] = true;
        }
    };

    // Initial check at time 0
    enqueueArrived(currentTime);

    while (completedCount < n) {
        if (readyQueue.empty()) {
            // Find next arriving uncompleted process
            int nextArrival = 1e9;
            for (int i = 0; i < n; ++i) {
                if (processes[i].remainingBurstTime > 0 && !inQueue[i] && processes[i].arrivalTime < nextArrival) {
                    nextArrival = processes[i].arrivalTime;
                }
            }
            if (nextArrival < 1e9) {
                rawGantt.emplace_back(-1, currentTime, nextArrival);
                currentTime = nextArrival;
                enqueueArrived(currentTime);
            }
            if (readyQueue.empty()) {
                break;
            }
        }

        int currentIdx = readyQueue.front();
        readyQueue.pop();
        Process& p = processes[currentIdx];

        if (p.firstStartTime == -1) {
            p.firstStartTime = currentTime;
            p.responseTime = p.firstStartTime - p.arrivalTime;
        }

        int runTime = std::min(timeQuantum, p.remainingBurstTime);
        rawGantt.emplace_back(p.id, currentTime, currentTime + runTime);

        int prevTime = currentTime;
        currentTime += runTime;
        p.remainingBurstTime -= runTime;

        // Any new processes arriving while this process was running must enter ready queue
        enqueueArrived(currentTime);

        if (p.remainingBurstTime > 0) {
            // Re-enqueue current process after newly arrived ones
            readyQueue.push(currentIdx);
        } else {
            p.completionTime = currentTime;
            p.turnaroundTime = p.completionTime - p.arrivalTime;
            p.waitingTime = p.turnaroundTime - p.burstTime;
            completedCount++;
        }
    }

    result.processes = processes;
    result.ganttChart = compressGanttChart(rawGantt);
    calculateMetrics(result);
    return result;
}

// -------------------------------------------------------------
// Visual and JSON Output Utilities
// -------------------------------------------------------------
void CPUScheduleResult::printTable() const {
    std::cout << "\n========================================================================================\n";
    std::cout << "  " << algorithmName;
    if (timeQuantum > 0) std::cout << " (Time Quantum = " << timeQuantum << ")";
    std::cout << "\n========================================================================================\n";
    std::cout << std::left 
              << std::setw(6)  << "PID"
              << std::setw(10) << "Arrival"
              << std::setw(8)  << "Burst"
              << std::setw(10) << "Priority"
              << std::setw(12) << "Completion"
              << std::setw(12) << "Turnaround"
              << std::setw(10) << "Waiting"
              << std::setw(10) << "Response"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    for (const auto& p : processes) {
        std::cout << std::left 
                  << std::setw(6)  << ("P" + std::to_string(p.id))
                  << std::setw(10) << p.arrivalTime
                  << std::setw(8)  << p.burstTime
                  << std::setw(10) << p.priority
                  << std::setw(12) << p.completionTime
                  << std::setw(12) << p.turnaroundTime
                  << std::setw(10) << p.waitingTime
                  << std::setw(10) << p.responseTime
                  << "\n";
    }
    std::cout << "----------------------------------------------------------------------------------------\n";
}

void CPUScheduleResult::printGantt() const {
    std::cout << "\nGantt Chart:\n";
    if (ganttChart.empty()) {
        std::cout << "[Empty]\n";
        return;
    }

    // Top border
    for (const auto& seg : ganttChart) {
        std::cout << "+-------";
    }
    std::cout << "+\n";

    // Process Labels
    for (const auto& seg : ganttChart) {
        if (seg.processId == -1) {
            std::cout << "| IDLE  ";
        } else {
            std::string label = "  P" + std::to_string(seg.processId) + "  ";
            if (label.length() < 7) label.append(7 - label.length(), ' ');
            std::cout << "|" << label.substr(0, 7);
        }
    }
    std::cout << "|\n";

    // Bottom border
    for (const auto& seg : ganttChart) {
        std::cout << "+-------";
    }
    std::cout << "+\n";

    // Timeline numbers
    std::cout << ganttChart.front().startTime;
    for (const auto& seg : ganttChart) {
        std::string s = std::to_string(seg.endTime);
        int spaces = 8 - static_cast<int>(s.length());
        if (spaces < 1) spaces = 1;
        std::cout << std::string(spaces, ' ') << seg.endTime;
    }
    std::cout << "\n";
}

void CPUScheduleResult::printSummary() const {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nPerformance Metrics Summary:\n";
    std::cout << "  - Average Waiting Time    : " << avgWaitingTime << " units\n";
    std::cout << "  - Average Turnaround Time : " << avgTurnaroundTime << " units\n";
    std::cout << "  - Average Response Time   : " << avgResponseTime << " units\n";
    std::cout << "  - CPU Utilization         : " << cpuUtilization << " %\n";
    std::cout << "  - Throughput              : " << throughput << " processes/unit time\n";
    std::cout << "  - Total Duration          : " << totalTime << " units\n";
    std::cout << "  - Idle Time               : " << idleTime << " units\n";
    std::cout << "========================================================================================\n";
}

std::string CPUScheduleResult::toJson() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{\n";
    ss << "  \"algorithm\": \"" << algorithmName << "\",\n";
    ss << "  \"timeQuantum\": " << timeQuantum << ",\n";
    ss << "  \"avgWaitingTime\": " << avgWaitingTime << ",\n";
    ss << "  \"avgTurnaroundTime\": " << avgTurnaroundTime << ",\n";
    ss << "  \"avgResponseTime\": " << avgResponseTime << ",\n";
    ss << "  \"cpuUtilization\": " << cpuUtilization << ",\n";
    ss << "  \"throughput\": " << throughput << ",\n";
    ss << "  \"totalTime\": " << totalTime << ",\n";
    ss << "  \"idleTime\": " << idleTime << ",\n";

    // Processes array
    ss << "  \"processes\": [\n";
    for (size_t i = 0; i < processes.size(); ++i) {
        const auto& p = processes[i];
        ss << "    {\"id\": " << p.id
           << ", \"arrivalTime\": " << p.arrivalTime
           << ", \"burstTime\": " << p.burstTime
           << ", \"priority\": " << p.priority
           << ", \"completionTime\": " << p.completionTime
           << ", \"turnaroundTime\": " << p.turnaroundTime
           << ", \"waitingTime\": " << p.waitingTime
           << ", \"responseTime\": " << p.responseTime << "}";
        if (i + 1 < processes.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    // Gantt chart array
    ss << "  \"ganttChart\": [\n";
    for (size_t i = 0; i < ganttChart.size(); ++i) {
        const auto& g = ganttChart[i];
        ss << "    {\"processId\": " << g.processId
           << ", \"startTime\": " << g.startTime
           << ", \"endTime\": " << g.endTime << "}";
        if (i + 1 < ganttChart.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}";
    return ss.str();
}
