HEAD
# SMART-TRAFFIC: Intelligent Traffic Management Simulator
### Using CPU Scheduling and Task Scheduling Algorithms

**CSE End-Semester Project**  
**Core Technologies**: C++17, CMake, HTML5, CSS3, JavaScript  

---

## 1. Project Introduction

Urban traffic intersections are classic examples of shared-resource contention where competing vehicular movements vie for a single critical section (the physical intersection crossroad). In computer operating systems, CPU schedulers solve an identical fundamental challenge: coordinating competing processes requesting exclusive execution time on a central processing core.

**SMART-TRAFFIC** bridges the domains of Operating Systems (OS) and Traffic Control Systems. It provides:
1. A rigorous, mathematically exact C++17 implementation of foundational CPU scheduling algorithms.
2. A direct real-world adaptation into intelligent 4-way traffic intersection scheduling, integrating traffic density, waiting queue delays, emergency vehicle prioritization, anti-starvation dynamic priority aging, and safety clearance intervals (yellow and all-red phases).
3. A web dashboard providing real-time visual simulation of the 4-way intersection, traffic signal state transitions, Gantt charts, and performance benchmarking charts.

---

## 2. Problem Statement

Conventional fixed-time traffic light controllers cycle signals through rigid, pre-programmed timers regardless of actual traffic demand. This leads to:
- Excessive vehicle delays on high-volume approaches while empty roads hold green lights.
- High risk of gridlock during asymmetric rush hour periods.
- Inability to safely and rapidly prioritize emergency vehicles (ambulances, fire engines).
- Starvation of low-density side streets if simple greedy algorithms are used.

By modeling traffic queue clearance as process execution, we apply operating system scheduling principles (fairness, preemption, aging, deadline adherence) to dynamically optimize vehicular throughput while guaranteeing strict safety standards.

---

## 3. Objectives

1. Implement core CPU scheduling algorithms in modern C++17:
   - First-Come, First-Served (FCFS) [Non-preemptive]
   - Shortest Job First (SJF) [Non-preemptive]
   - Priority Scheduling [Non-preemptive]
   - Priority Scheduling [Preemptive]
   - Round Robin (RR) [Preemptive with configurable time quantum]
2. Adapt these principles to simulate a 4-way intersection (North, South, East, West):
   - Traffic FCFS
   - Traffic Priority Scheduling (incorporating Density + Waiting Time + Dynamic Aging + Emergency Preemption)
   - Traffic Round Robin (time-sliced green intervals)
   - Traffic Earliest Deadline First (EDF)
3. Enforce real-world traffic safety constraints:
   - Zero conflicting movements receiving simultaneous green signals.
   - Mandatory yellow-light clearance ($2\text{s}$) and all-red clearance ($1\text{s}$) before switching active directions.
   - Guaranteed minimum ($3\text{s}$) and maximum ($20\text{s}$) green durations.
   - Safe emergency vehicle prioritization without bypassing clearance intervals.
4. Provide comprehensive performance evaluation:
   - CPU metrics: Waiting Time, Turnaround Time, Response Time, CPU Utilization, Throughput.
   - Traffic metrics: Vehicle Waiting Time / Delay, Max Queue Wait, Emergency Delay, Deadline Misses, Traffic Throughput, Green Signal Utilization.
5. Provide a simple, interactive frontend dashboard connecting directly to C++ scheduling data.
6. Verify algorithm correctness using automated unit and system test suites (17 test cases).

---

## 4. Technologies Used

- **C++17**: Pure standard library (`<vector>`, `<queue>`, `<algorithm>`, `<chrono>`, `<random>`) for high performance and zero external compilation dependencies.
- **CMake (3.15+)**: Cross-platform build automation.
- **JSON**: Data serialization format used to transfer scheduling results and timelines from C++ to the web frontend.
- **HTML5 / CSS3 / JavaScript (ES6)**: Clean, dependency-free dashboard visualization and interactive simulation playback.

---

## 5. Folder Structure

```
SMART-TRAFFIC/
│
├── main.cpp                  # Interactive CLI menu, benchmarking, and JSON export
├── cpu_scheduler.h           # Process struct, GanttSegment, and CPU scheduler declarations
├── cpu_scheduler.cpp         # Implementations of FCFS, SJF, Priority (NP/P), Round Robin
├── traffic_scheduler.h       # TrafficTask, SignalInterval, and Traffic Scheduler declarations
├── traffic_scheduler.cpp     # Traffic FCFS, Priority+Aging, Round Robin, and EDF algorithms
├── metrics.h                 # Dynamic metric computation and comparative benchmark headers
├── metrics.cpp               # Benchmark evaluation and ASCII table formatting
├── simulation.h              # Simulation engine headers (Random generators & presets)
├── simulation.cpp            # Seeded random traffic generation and manual input prompts
│
├── frontend/
│   ├── index.html            # Web dashboard (Intersection view, Gantt chart, metrics)
│   ├── style.css             # Dark-mode dashboard styling
│   ├── script.js             # Simulation playback controller & UI renderer
│   └── data.json             # Serialized output generated by C++ engine
│
├── tests.cpp                 # 17 Automated unit and system test cases with assertions
├── CMakeLists.txt            # CMake build definitions
└── README.md                 # Project documentation and specifications
```

---

## 6. CPU Scheduling Algorithms & Pseudocode

### 1. FCFS (First Come First Serve)
- **Type**: Non-preemptive.
- **Mechanism**: Processes are dispatched in the exact order of their arrival. If CPU is idle, the scheduler waits until the next arrival.
```text
Algorithm FCFS(Processes):
    Sort Processes by arrivalTime ascending, then by PID
    currentTime = 0
    For each process p in Processes:
        If currentTime < p.arrivalTime:
            Record IDLE segment from currentTime to p.arrivalTime
            currentTime = p.arrivalTime
        p.firstStartTime = currentTime
        p.responseTime = currentTime - p.arrivalTime
        Record Gantt slice [p.id, currentTime, currentTime + p.burstTime]
        currentTime = currentTime + p.burstTime
        p.completionTime = currentTime
        p.turnaroundTime = p.completionTime - p.arrivalTime
        p.waitingTime = p.turnaroundTime - p.burstTime
```

### 2. SJF (Shortest Job First)
- **Type**: Non-preemptive.
- **Mechanism**: Among all processes that have arrived at or before the current timestamp, select the one with the smallest burst time. Ties are broken by earlier arrival time, then by PID.

### 3. Priority Scheduling (Non-Preemptive & Preemptive)
- **Type**: Non-preemptive and Preemptive variants.
- **Convention**: **Lower numerical value indicates higher priority** ($1 > 2 > 3$).
- **Preemptive Mechanism**: At each discrete time unit, find all ready processes (`arrivalTime <= t`). Select the process with the lowest priority number. If a higher-priority process arrives while another is executing, the current process is preempted immediately and returned to the ready state.

### 4. Round Robin (RR)
- **Type**: Preemptive with configurable time quantum $Q$.
- **Mechanism**: Maintains a FIFO ready queue. The executing process runs for $\min(Q, \text{remainingBurstTime})$. Newly arriving processes at the time slice boundary enter the ready queue *before* the preempted process is re-enqueued.

---

## 7. Traffic Task Scheduling Algorithms & Pseudocode

### Universal Emergency Preemption (EVP)
All 4 traffic scheduling algorithms implement **Universal Emergency Preemption**:
- Whenever an emergency vehicle (Ambulance, Fire Engine, Police) is detected or arrives midway during any signal phase:
  1. The currently active road is allowed to safely complete its minimum green constraint ($3\text{s}$) to avoid hazardous sudden stops.
  2. The controller immediately transitions through mandatory safety clearance: **Yellow ($2\text{s}$) $\rightarrow$ All-Red ($1\text{s}$)**.
  3. The emergency approach receives green immediately.
  4. Once cleared, the controller safely returns to the base scheduling algorithm (FCFS, Priority, Round Robin, or EDF).

### 1. Traffic FCFS (with Universal Emergency Preemption)
- Dispatches intersection approaches based on queue arrival timestamp.
- Emergency vehicles immediately preempt the arrival sequence after safety clearance.

### 2. Traffic Priority Scheduling (Density, Wait, Aging, Emergency)
- **Dynamic Priority Formula**:
  $$\text{Priority} = \text{BasePriority} + (\text{WaitingVehicles} \times 2) + (\text{WaitingTime} \times \text{AgingRate}) + \text{EmergencyBoost}$$
- **Anti-Starvation Aging**: Every second a low-density approach waits, its priority score steadily rises, guaranteeing service.

### 3. Traffic Round Robin (with Universal Emergency Preemption)
- Cycles green signals among active approaches using a green-light quantum $Q$ (e.g., $6\text{s}$).
- Emergency arrivals interrupt the cyclic queue, receive green, and return cleanly to the queued approaches.

### 4. Traffic Earliest Deadline First (EDF) (with Universal Emergency Preemption)
- Selects the traffic approach whose queue faces the most imminent deadline ($t_{\text{deadline}} = t_{\text{arrival}} + \text{serviceTime} + \text{maxDelay}$).
- Emergency tasks receive negative deadlines (highest urgency). Detects and counts deadline misses dynamically.

---

## 8. Performance Formulas

### CPU Metrics:
- **Turnaround Time ($TAT$)**:
  $$TAT_i = CT_i - AT_i$$
- **Waiting Time ($WT$)**:
  $$WT_i = TAT_i - BT_i$$
- **Response Time ($RT$)**:
  $$RT_i = \text{FirstCPUAllocationTime}_i - AT_i$$
- **Average Waiting Time**:
  $$\overline{WT} = \frac{1}{N} \sum_{i=1}^N WT_i$$
- **Average Turnaround Time**:
  $$\overline{TAT} = \frac{1}{N} \sum_{i=1}^N TAT_i$$
- **CPU Utilization**:
  $$\text{Utilization (\%)} = \left(\frac{\sum BT_i}{\text{Total Schedule Time}}\right) \times 100\%$$
- **Throughput**:
  $$\text{Throughput} = \frac{N}{\text{Total Schedule Time}} \quad (\text{processes/unit time})$$

### Traffic Metrics:
- **Average Vehicle Delay**:
  $$\overline{\text{Delay}} = \frac{\sum (WT_{\text{task}} \times \text{Vehicles}_{\text{task}})}{\text{Total Vehicles Served}} \quad (\text{seconds/vehicle})$$
- **Traffic Throughput**:
  $$\text{Throughput} = \frac{\text{Total Vehicles Served}}{\text{Total Simulation Duration}} \quad (\text{vehicles/second})$$
- **Signal Utilization**:
  $$\text{Green Utilization (\%)} = \left(\frac{\text{Total Green Duration}}{\text{Total Simulation Duration}}\right) \times 100\%$$
- **Total Clearance Time**:
  $$\text{Clearance Time} = \sum (\text{Yellow Intervals} + \text{All-Red Intervals})$$

---

## 9. Compilation & Execution Instructions

### Prerequisites:
- Any modern C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+).
- CMake 3.15 or newer.

### Build Steps:
Open a terminal in the project directory:

```bash
# 1. Generate build files
cmake -B build -G "MinGW Makefiles"

# 2. Compile all targets (core executables & test suite)
cmake --build build
```

*(On Linux / macOS, replace `-G "MinGW Makefiles"` with standard `cmake -B build`)*

### Running Automated Tests:
```bash
./build/run_tests.exe
```

### Running Interactive Terminal Program:
```bash
./build/smart_traffic.exe
```

### Viewing Web Dashboard:
1. Simply open `frontend/index.html` in any web browser (Chrome, Firefox, Edge, Safari).
2. The frontend automatically displays pre-compiled engine data or reads live updates from `frontend/data.json`.
3. To re-export fresh simulation runs from C++ to the frontend:
   ```bash
   ./build/smart_traffic.exe --export
   ```

---

## 10. Sample Inputs and Outputs

### Sample CPU Comparison on 4 Processes:
**Input**:
- $P_1$: Arrival = $0$, Burst = $8$, Priority = $3$
- $P_2$: Arrival = $1$, Burst = $4$, Priority = $1$
- $P_3$: Arrival = $2$, Burst = $9$, Priority = $4$
- $P_4$: Arrival = $3$, Burst = $5$, Priority = $2$

**C++ Benchmark Output Table**:
```
========================================================================================================
                             CPU SCHEDULING BENCHMARK COMPARISON TABLE                                  
========================================================================================================
Algorithm                             Avg Wait      Avg TAT       Avg Resp      CPU Util(%)   Throughput  
--------------------------------------------------------------------------------------------------------
FCFS (First Come First Serve)         8.75          15.25         8.75          100.00        0.15        
SJF (Shortest Job First - Non-Preempt)7.75          14.25         7.75          100.00        0.15        
Priority Scheduling (Non-Preemptive)  7.75          14.25         7.75          100.00        0.15        
Priority Scheduling (Preemptive)      6.50          13.00         4.25          100.00        0.15        
Round Robin (RR)                      13.50         20.00         3.00          100.00        0.15        
========================================================================================================
```

### Sample Traffic Signal Timeline (Priority with Aging & Emergency):
```
Time      Road        Signal      Duration    Vehicles Cleared
----------------------------------------------------------------------------------------
[0 -> 20] North       GREEN       20s         20
[20 -> 23]North       GREEN       3s          2
[23 -> 25]North       YELLOW      2s          0
[25 -> 26]ALL         ALL_RED     1s          0
[26 -> 44]South       GREEN       18s         18
[44 -> 46]South       YELLOW      2s          0
[46 -> 47]ALL         ALL_RED     1s          0
[47 -> 61]North       GREEN       14s         14
[61 -> 63]North       YELLOW      2s          0
[63 -> 64]ALL         ALL_RED     1s          0
[64 -> 68]East        GREEN       4s          4
[68 -> 70]East        YELLOW      2s          0
[70 -> 71]ALL         ALL_RED     1s          0
[71 -> 74]West        GREEN       3s          3
```

---

## 11. Testing & Verification

The project includes 17 automated tests in `tests.cpp` validating every requirement:

### CPU Tests:
1. **FCFS Idle & Different Arrivals**: Verifies CPU idle tracking and metric calculations.
2. **FCFS Simultaneous Arrivals**: Verifies deterministic tie-breaking.
3. **SJF Non-Preemptive**: Verifies shortest burst selection and tie-breaking.
4. **Priority Non-Preemptive**: Verifies selection of highest priority task ($1 > 3$).
5. **Priority Preemptive**: Verifies preemption event when a higher-priority task arrives.
6. **Round Robin Quantum Variations**: Verifies behaviour under $Q=2$ vs $Q=4$.
7. **Single Process Edge Case**: Verifies initial idle interval before first arrival.

### Traffic Tests:
8. **Equal Traffic All Roads**: 4 approaches with 5 vehicles each at $t=0$.
9. **Heavy Traffic on One Road**: Validates density prioritization.
10. **Low Traffic Road Minimum Green Constraint**: Enforces $\ge 3\text{s}$ green duration.
11. **Emergency Vehicle Arrival & Preemption**: Emergency arrives midway, current road safely clears, emergency gets green.
12. **Multiple Emergency Vehicles**: Validates emergency prioritization over normal queues.
13. **Continuous Staggered Arrivals**: Evaluates wave arrivals over time.
14. **Starvation Prevention via Priority Aging**: Verifies that low-density road priority increases dynamically to exceed heavy road priority.
15. **Deadline Misses Detection**: Verifies accurate deadline miss flags under tight thresholds.
16. **Empty Traffic Queue Edge Case**: Verifies safe handling of 0 vehicles without crash.
17. **Signal Safety Clearance Verification**: Validates strict mutual exclusion, zero overlapping greens, and mandatory Yellow + All-Red sequences.

**Result**: 17 / 17 Tests Pass with 100% assertion coverage.

---

## 12. Limitations & Future Scope

### Current Limitations:
- The intersection currently models each directional approach (North, South, East, West) as one movement without dedicated protected left-turn phases.
- Pedestrian crosswalk signal phases are not explicitly modeled.
- Clearances use fixed conservative yellow ($2\text{s}$) and all-red ($1\text{s}$) intervals rather than speed-dependent clearing formulas.

### Future Scope:
- **Dual-Ring 8-Phase NEMA Controller**: Model dual-ring concurrent movements (e.g., North and South through traffic simultaneous green).
- **Multi-Intersection Network Coordination**: Connect multiple adjacent intersections in a green-wave arterial corridor.
- **V2X (Vehicle-to-Infrastructure) Telemetry**: Ingest real-time GPS coordinates and speed vectors from connected vehicles.
- **Machine Learning Reinforcement Learning Agent**: Compare classic OS heuristic schedulers against Deep Q-Network (DQN) traffic signal controllers.
# OS-project
ea78c388f7a8b0d30974113f30eb6727bd2b324e
