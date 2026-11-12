#ifndef SIMULATION_H
#define SIMULATION_H

#include "cpu_scheduler.h"
#include "traffic_scheduler.h"
#include <vector>
#include <random>
#include <string>

// Simulation execution states
enum class SimState {
    STOPPED,
    RUNNING,
    PAUSED
};

// Simulation generator and runner for manual and automated modes
class SimulationEngine {
private:
    unsigned int randomSeed;
    std::mt19937 rng;
    SimState currentState = SimState::STOPPED;
    int currentSimulationStep = 0;

public:
    SimulationEngine(unsigned int seed = 42);

    void setSeed(unsigned int seed);
    unsigned int getSeed() const { return randomSeed; }

    // State control methods
    void start();
    void pause();
    void resume();
    void reset();
    SimState getState() const { return currentState; }
    std::string getStateString() const;

    // Automatic Generation (Reproducible with seed)
    std::vector<Process> generateRandomProcesses(int count = 5, int maxArrival = 10, int minBurst = 2, int maxBurst = 10, int minPriority = 1, int maxPriority = 4);
    std::vector<TrafficTask> generateRandomTraffic(int count = 6, int maxArrival = 20, int minVehicles = 2, int maxVehicles = 12, double emergencyProbability = 0.2);

    // Preset Realistic Scenarios
    std::vector<TrafficTask> getPresetBalancedIntersection();
    std::vector<TrafficTask> getPresetMorningRushHour();
    std::vector<TrafficTask> getPresetEmergencyResponse();

    // Manual Input Helpers (Command-line)
    static std::vector<Process> promptManualProcesses();
    static std::vector<TrafficTask> promptManualTraffic();
};

#endif // SIMULATION_H
