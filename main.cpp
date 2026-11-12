#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include "cpu_scheduler.h"
#include "traffic_scheduler.h"
#include "metrics.h"
#include "simulation.h"

void displayMainMenu() {
    std::cout << "\n========================================================================\n";
    std::cout << "          SMART-TRAFFIC : INTELLIGENT TRAFFIC & CPU SCHEDULER           \n";
    std::cout << "========================================================================\n";
    std::cout << " 1. CPU Scheduling Algorithms (FCFS, SJF, Priority, Round Robin)\n";
    std::cout << " 2. Traffic Task Scheduling (FCFS, Priority+Aging, Round Robin, EDF)\n";
    std::cout << " 3. Comparative Benchmarks (Side-by-Side Algorithm Comparison)\n";
    std::cout << " 4. Simulation Engine (Manual Input, Random Generator, Presets)\n";
    std::cout << " 5. Export Simulation Data to JSON (for Frontend Dashboard)\n";
    std::cout << " 6. Exit\n";
    std::cout << "========================================================================\n";
    std::cout << "Enter your choice (1-6): ";
}

std::string buildTrafficScenarioJson(const std::vector<TrafficTask>& tasks, int timeQuantum = 6) {
    auto f = scheduleTrafficFCFS(tasks);
    auto p = scheduleTrafficPriority(tasks);
    auto r = scheduleTrafficRoundRobin(tasks, timeQuantum);
    auto e = scheduleTrafficEDF(tasks);
    auto comp = MetricsManager::compareAllTraffic(tasks, timeQuantum);

    std::ostringstream ss;
    ss << "{\n";
    ss << "    \"fcfs\": " << f.toJson() << ",\n";
    ss << "    \"priority\": " << p.toJson() << ",\n";
    ss << "    \"roundRobin\": " << r.toJson() << ",\n";
    ss << "    \"edf\": " << e.toJson() << ",\n";
    ss << "    \"comparison\": " << MetricsManager::trafficComparisonToJson(comp) << "\n";
    ss << "  }";
    return ss.str();
}

std::string buildCPUScenarioJson(const std::vector<Process>& processes, int timeQuantum = 3) {
    auto f = scheduleFCFS(processes);
    auto s = scheduleSJF(processes);
    auto pnp = schedulePriorityNonPreemptive(processes);
    auto pp = schedulePriorityPreemptive(processes);
    auto rr = scheduleRoundRobin(processes, timeQuantum);
    auto comp = MetricsManager::compareAllCPU(processes, timeQuantum);

    std::ostringstream ss;
    ss << "{\n";
    ss << "    \"fcfs\": " << f.toJson() << ",\n";
    ss << "    \"sjf\": " << s.toJson() << ",\n";
    ss << "    \"priorityNonPreemptive\": " << pnp.toJson() << ",\n";
    ss << "    \"priorityPreemptive\": " << pp.toJson() << ",\n";
    ss << "    \"roundRobin\": " << rr.toJson() << ",\n";
    ss << "    \"comparison\": " << MetricsManager::cpuComparisonToJson(comp) << "\n";
    ss << "  }";
    return ss.str();
}

void exportDataToJson(SimulationEngine& simEngine,
                      const std::vector<Process>& activeProcesses,
                      const std::vector<TrafficTask>& activeTasks,
                      const std::string& filename = "frontend/data.json") {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open " << filename << " for writing.\n";
        return;
    }

    auto balancedTasks = simEngine.getPresetBalancedIntersection();
    auto rushTasks = simEngine.getPresetMorningRushHour();
    auto emergTasks = simEngine.getPresetEmergencyResponse();

    // CPU Variations
    std::vector<Process> simArrivalProcesses = {
        Process(1, 0, 6, 2),
        Process(2, 0, 3, 1),
        Process(3, 0, 8, 3),
        Process(4, 0, 2, 4)
    };

    std::vector<Process> longBurstProcesses = {
        Process(1, 0, 15, 3),
        Process(2, 2, 4, 1),
        Process(3, 4, 2, 2),
        Process(4, 6, 12, 4)
    };

    out << "{\n";
    out << "  \"trafficScenarios\": {\n";
    out << "    \"morningRush\": " << buildTrafficScenarioJson(rushTasks) << ",\n";
    out << "    \"balanced\": " << buildTrafficScenarioJson(balancedTasks) << ",\n";
    out << "    \"emergency\": " << buildTrafficScenarioJson(emergTasks) << ",\n";
    out << "    \"custom\": " << buildTrafficScenarioJson(activeTasks) << "\n";
    out << "  },\n";

    out << "  \"cpuScenarios\": {\n";
    out << "    \"standard\": " << buildCPUScenarioJson(activeProcesses) << ",\n";
    out << "    \"simultaneous\": " << buildCPUScenarioJson(simArrivalProcesses) << ",\n";
    out << "    \"heavyBurst\": " << buildCPUScenarioJson(longBurstProcesses) << "\n";
    out << "  }\n";
    out << "}\n";

    out.close();
    std::cout << "\n>>> Complete multi-scenario simulation data exported to: " << filename << " <<<\n";
}

void handleCPUSubmenu(const std::vector<Process>& processes) {
    std::cout << "\n--- CPU Scheduling Algorithms ---\n";
    std::cout << " 1. FCFS (First Come First Serve)\n";
    std::cout << " 2. SJF (Shortest Job First - Non-Preemptive)\n";
    std::cout << " 3. Priority Scheduling (Non-Preemptive)\n";
    std::cout << " 4. Priority Scheduling (Preemptive)\n";
    std::cout << " 5. Round Robin (RR)\n";
    std::cout << " 6. Run All 5 Algorithms\n";
    std::cout << "Select an algorithm (1-6): ";
    int c = 1;
    if (!(std::cin >> c)) return;

    switch (c) {
        case 1: { auto res = scheduleFCFS(processes); res.printTable(); res.printGantt(); res.printSummary(); break; }
        case 2: { auto res = scheduleSJF(processes); res.printTable(); res.printGantt(); res.printSummary(); break; }
        case 3: { auto res = schedulePriorityNonPreemptive(processes); res.printTable(); res.printGantt(); res.printSummary(); break; }
        case 4: { auto res = schedulePriorityPreemptive(processes); res.printTable(); res.printGantt(); res.printSummary(); break; }
        case 5: {
            int q = 3; std::cout << "Enter time quantum [default 3]: ";
            int uq; if (std::cin >> uq && uq > 0) q = uq;
            auto res = scheduleRoundRobin(processes, q); res.printTable(); res.printGantt(); res.printSummary();
            break;
        }
        case 6: {
            auto f = scheduleFCFS(processes); f.printTable(); f.printGantt(); f.printSummary();
            auto s = scheduleSJF(processes);  s.printTable(); s.printGantt(); s.printSummary();
            auto pnp = schedulePriorityNonPreemptive(processes); pnp.printTable(); pnp.printGantt(); pnp.printSummary();
            auto pp = schedulePriorityPreemptive(processes); pp.printTable(); pp.printGantt(); pp.printSummary();
            auto rr = scheduleRoundRobin(processes, 3); rr.printTable(); rr.printGantt(); rr.printSummary();
            break;
        }
        default: break;
    }
}

void handleTrafficSubmenu(const std::vector<TrafficTask>& tasks) {
    std::cout << "\n--- Traffic Task Scheduling Algorithms ---\n";
    std::cout << " 1. Traffic FCFS (First Come First Serve)\n";
    std::cout << " 2. Traffic Priority (Density + Wait + Aging + Emergency Preemption)\n";
    std::cout << " 3. Traffic Round Robin (Configurable Green-Light Time Quantum)\n";
    std::cout << " 4. Traffic Earliest Deadline First (EDF)\n";
    std::cout << " 5. Run All 4 Algorithms\n";
    std::cout << "Select an algorithm (1-5): ";
    int c = 1;
    if (!(std::cin >> c)) return;

    switch (c) {
        case 1: { auto res = scheduleTrafficFCFS(tasks); res.printTimeline(); res.printTaskReport(); res.printMetrics(); break; }
        case 2: { auto res = scheduleTrafficPriority(tasks); res.printTimeline(); res.printTaskReport(); res.printMetrics(); break; }
        case 3: {
            int q = 6; std::cout << "Enter Green-Light Time Quantum in seconds [default 6]: ";
            int uq; if (std::cin >> uq && uq > 0) q = uq;
            auto res = scheduleTrafficRoundRobin(tasks, q); res.printTimeline(); res.printTaskReport(); res.printMetrics();
            break;
        }
        case 4: { auto res = scheduleTrafficEDF(tasks); res.printTimeline(); res.printTaskReport(); res.printMetrics(); break; }
        case 5: {
            auto f = scheduleTrafficFCFS(tasks); f.printTimeline(); f.printTaskReport(); f.printMetrics();
            auto p = scheduleTrafficPriority(tasks); p.printTimeline(); p.printTaskReport(); p.printMetrics();
            auto r = scheduleTrafficRoundRobin(tasks, 6); r.printTimeline(); r.printTaskReport(); r.printMetrics();
            auto e = scheduleTrafficEDF(tasks); e.printTimeline(); e.printTaskReport(); e.printMetrics();
            break;
        }
        default: break;
    }
}

int main(int argc, char* argv[]) {
    SimulationEngine simEngine(42);

    std::vector<Process> activeProcesses = {
        Process(1, 0, 8, 3),
        Process(2, 1, 4, 1),
        Process(3, 2, 9, 4),
        Process(4, 3, 5, 2)
    };

    std::vector<TrafficTask> activeTasks = simEngine.getPresetMorningRushHour();

    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--export" || arg == "-e") {
            exportDataToJson(simEngine, activeProcesses, activeTasks);
            return 0;
        }
    }

    exportDataToJson(simEngine, activeProcesses, activeTasks);

    int choice = 0;
    while (true) {
        displayMainMenu();
        if (!(std::cin >> choice)) break;
        if (choice == 6) {
            std::cout << "Exiting SMART-TRAFFIC. Thank you!\n";
            break;
        }

        switch (choice) {
            case 1: handleCPUSubmenu(activeProcesses); break;
            case 2: handleTrafficSubmenu(activeTasks); break;
            case 3: {
                std::cout << "\n--- Side-by-Side Algorithm Performance Benchmarks ---\n";
                auto cpuComp = MetricsManager::compareAllCPU(activeProcesses, 3);
                MetricsManager::printCPUComparison(cpuComp);

                auto trafComp = MetricsManager::compareAllTraffic(activeTasks, 6);
                MetricsManager::printTrafficComparison(trafComp);
                break;
            }
            case 4: {
                std::cout << "\n--- Simulation Generator & Input Modes ---\n";
                std::cout << " 1. Use Balanced Intersection Preset\n";
                std::cout << " 2. Use Morning Rush Hour Corridor Preset\n";
                std::cout << " 3. Use Emergency Response Scenario Preset\n";
                std::cout << " 4. Generate Random Data (Configurable Seed)\n";
                std::cout << " 5. Manual Process & Traffic Input\n";
                std::cout << "Select option (1-5): ";
                int sm = 1;
                if (std::cin >> sm) {
                    if (sm == 1) {
                        activeTasks = simEngine.getPresetBalancedIntersection();
                        std::cout << "Loaded Balanced Intersection Preset (4 approaches).\n";
                    } else if (sm == 2) {
                        activeTasks = simEngine.getPresetMorningRushHour();
                        std::cout << "Loaded Morning Rush Hour Preset.\n";
                    } else if (sm == 3) {
                        activeTasks = simEngine.getPresetEmergencyResponse();
                        std::cout << "Loaded Emergency Response Preset (with arriving ambulance).\n";
                    } else if (sm == 4) {
                        unsigned int s = 42;
                        std::cout << "Enter random seed [e.g., 42, 100, 999]: ";
                        if (std::cin >> s) simEngine.setSeed(s);
                        activeProcesses = simEngine.generateRandomProcesses(5);
                        activeTasks = simEngine.generateRandomTraffic(6);
                        std::cout << "Generated reproducible random processes and traffic tasks with seed " << s << ".\n";
                    } else if (sm == 5) {
                        activeProcesses = SimulationEngine::promptManualProcesses();
                        activeTasks = SimulationEngine::promptManualTraffic();
                    }
                }
                exportDataToJson(simEngine, activeProcesses, activeTasks);
                break;
            }
            case 5: exportDataToJson(simEngine, activeProcesses, activeTasks); break;
            default: std::cout << "Invalid choice. Please select 1-6.\n"; break;
        }
    }

    return 0;
}
