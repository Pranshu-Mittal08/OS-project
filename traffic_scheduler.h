#ifndef TRAFFIC_SCHEDULER_H
#define TRAFFIC_SCHEDULER_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

// Traffic road identifiers
enum class RoadDirection {
    NORTH,
    SOUTH,
    EAST,
    WEST
};

// Converts road enum to string
std::string roadToString(RoadDirection road);
RoadDirection stringToRoad(const std::string& name);

// Represents a schedulable traffic movement task for an intersection approach
struct TrafficTask {
    int taskId;
    std::string roadName;           // "North", "South", "East", "West"
    int arrivalTime;                // Time when vehicle queue arrived/was registered
    int waitingVehicles;            // Total number of vehicles queued
    int estimatedServiceTime;       // Total estimated green time needed to clear
    int priority;                   // Priority score (Higher score = higher priority)
    int deadline;                   // Absolute time by which queue should be cleared
    int waitingTime = 0;            // Total delay experienced before service finishes
    bool hasEmergency = false;      // True if queue contains ambulance/fire truck/police
    int remainingServiceTime = 0;   // Remaining service time needed to clear remaining vehicles
    int completionTime = 0;         // Time when all vehicles in this task finished
    int firstGreenTime = -1;        // Timestamp when signal first turned green for this task
    int vehiclesServed = 0;         // Total vehicles cleared
    bool missedDeadline = false;    // Whether completion exceeded deadline

    TrafficTask()
        : taskId(0), roadName("North"), arrivalTime(0), waitingVehicles(0),
          estimatedServiceTime(0), priority(0), deadline(0), remainingServiceTime(0) {}

    TrafficTask(int id, std::string road, int arrival, int vehicles, int estService, int prio, int dline, bool emerg = false)
        : taskId(id), roadName(std::move(road)), arrivalTime(arrival), waitingVehicles(vehicles),
          estimatedServiceTime(estService), priority(prio), deadline(dline),
          hasEmergency(emerg), remainingServiceTime(estService) {}
};

// Represents a signal state interval in the intersection execution timeline
struct SignalInterval {
    std::string roadName;           // "North", "South", "East", "West", or "ALL"
    std::string signalState;        // "GREEN", "YELLOW", "ALL_RED"
    int startTime;
    int endTime;
    int vehiclesPassed = 0;

    SignalInterval(std::string road, std::string state, int start, int end, int passed = 0)
        : roadName(std::move(road)), signalState(std::move(state)), startTime(start), endTime(end), vehiclesPassed(passed) {}

    int duration() const { return endTime - startTime; }
};

// Configurable traffic intersection parameters & safety constraints
struct TrafficConfig {
    int yellowDuration = 2;         // Yellow light clearance duration
    int allRedDuration = 1;         // All-red intersection clearance interval
    int minGreenTime = 3;           // Minimum green duration once granted
    int maxGreenTime = 20;          // Maximum continuous green duration
    int serviceRatePerVehicle = 1;  // Time units required to clear 1 vehicle
    int agingRate = 1;              // Priority boost per unit of waiting time to prevent starvation
    int emergencyPriorityBoost = 1000; // Priority bonus for emergency vehicles
};

// Holds complete traffic schedule results, signal timelines, and performance metrics
struct TrafficScheduleResult {
    std::string algorithmName;
    int timeQuantum = 0;
    std::vector<TrafficTask> completedTasks;
    std::vector<SignalInterval> signalTimeline;

    // Traffic Performance Metrics
    double avgVehicleWaitingTime = 0.0;
    int maxWaitingTime = 0;
    int totalVehiclesServed = 0;
    double emergencyAvgWaitingTime = 0.0;
    int deadlineMisses = 0;
    double trafficThroughput = 0.0; // Vehicles served per unit time
    double signalUtilization = 0.0; // Percentage of total time where signal was GREEN
    int totalSimulationTime = 0;
    int totalGreenTime = 0;
    int totalClearanceTime = 0;     // Yellow + All-Red time

    void printTimeline() const;
    void printTaskReport() const;
    void printMetrics() const;
    std::string toJson() const;
};

// Priority calculation helper considering density, waiting time, aging, and emergency
int calculateDynamicPriority(const TrafficTask& task, int currentTime, const TrafficConfig& cfg);

// Calculates all traffic performance metrics
void calculateTrafficMetrics(TrafficScheduleResult& result);

// Traffic Scheduling Algorithm Implementations
TrafficScheduleResult scheduleTrafficFCFS(std::vector<TrafficTask> tasks, TrafficConfig cfg = TrafficConfig());
TrafficScheduleResult scheduleTrafficPriority(std::vector<TrafficTask> tasks, TrafficConfig cfg = TrafficConfig());
TrafficScheduleResult scheduleTrafficRoundRobin(std::vector<TrafficTask> tasks, int timeQuantum, TrafficConfig cfg = TrafficConfig());
TrafficScheduleResult scheduleTrafficEDF(std::vector<TrafficTask> tasks, TrafficConfig cfg = TrafficConfig());

#endif // TRAFFIC_SCHEDULER_H
