#ifndef CPU_SCHEDULER_H
#define CPU_SCHEDULER_H

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

// Represents a single process in the CPU scheduling system
struct Process {
    int id;                   // Process ID (e.g., 1, 2, 3...)
    int arrivalTime;          // Time at which process arrives in ready queue
    int burstTime;            // Total CPU execution time required
    int priority;             // Priority value (Lower integer = Higher priority)
    
    // Output / Computed metrics
    int completionTime = 0;   // Time at which process completes execution
    int waitingTime = 0;      // Total time spent waiting in ready queue
    int turnaroundTime = 0;   // Total time from arrival to completion (CT - AT)
    int responseTime = -1;    // Time from arrival until first CPU allocation
    int remainingBurstTime = 0;// Remaining CPU burst time needed to finish
    int firstStartTime = -1;  // Timestamp when process first received CPU

    Process() : id(0), arrivalTime(0), burstTime(0), priority(0), remainingBurstTime(0) {}
    Process(int pId, int at, int bt, int prio = 0)
        : id(pId), arrivalTime(at), burstTime(bt), priority(prio),
          completionTime(0), waitingTime(0), turnaroundTime(0),
          responseTime(-1), remainingBurstTime(bt), firstStartTime(-1) {}
};

// Represents a continuous time slice in the Gantt chart
struct GanttSegment {
    int processId;  // Process ID, or -1 for IDLE period
    int startTime;  // Start timestamp
    int endTime;    // End timestamp

    GanttSegment(int pid, int start, int end)
        : processId(pid), startTime(start), endTime(end) {}
};

// Holds complete schedule results, execution timeline, and metrics
struct CPUScheduleResult {
    std::string algorithmName;
    int timeQuantum = 0;                       // Used for Round Robin
    std::vector<Process> processes;            // Evaluated processes with all computed times
    std::vector<GanttSegment> ganttChart;      // Merged Gantt chart intervals
    
    // Summary performance metrics
    double avgWaitingTime = 0.0;
    double avgTurnaroundTime = 0.0;
    double avgResponseTime = 0.0;
    double cpuUtilization = 0.0;               // Percentage (0.0% to 100.0%)
    double throughput = 0.0;                   // Completed processes per unit time
    int totalTime = 0;                         // Total duration of schedule (end of last process)
    int idleTime = 0;                          // Total idle time of CPU

    void printTable() const;
    void printGantt() const;
    void printSummary() const;
    std::string toJson() const;
};

// Merges adjacent Gantt intervals belonging to the same process
std::vector<GanttSegment> compressGanttChart(const std::vector<GanttSegment>& rawGantt);

// Computes summary performance metrics for a schedule result
void calculateMetrics(CPUScheduleResult& result);

// CPU Scheduling Algorithm Implementations
CPUScheduleResult scheduleFCFS(std::vector<Process> processes);
CPUScheduleResult scheduleSJF(std::vector<Process> processes);
CPUScheduleResult schedulePriorityNonPreemptive(std::vector<Process> processes);
CPUScheduleResult schedulePriorityPreemptive(std::vector<Process> processes);
CPUScheduleResult scheduleRoundRobin(std::vector<Process> processes, int timeQuantum);

#endif // CPU_SCHEDULER_H
