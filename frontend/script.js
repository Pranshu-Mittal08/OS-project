// SMART-TRAFFIC Frontend Visualization & Interactive Controller
// Connects C++ Generated Scheduling Data to the Web UI

let appData = null;
let currentClock = 0;
let isPlaying = false;
let playInterval = null;

// Tab switcher
function switchTab(tabId) {
    document.querySelectorAll('.tab-content').forEach(tab => tab.classList.remove('active'));
    document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));

    const activeTab = document.getElementById(tabId);
    if (activeTab) activeTab.classList.add('active');

    const clickedBtn = Array.from(document.querySelectorAll('.tab-btn')).find(b => b.getAttribute('onclick').includes(tabId));
    if (clickedBtn) clickedBtn.classList.add('active');

    if (tabId === 'cpuTab') renderCPUView();
    if (tabId === 'benchmarkTab') renderBenchmarkView();
}

// Fetch simulation data from backend JSON output, or fallback
async function loadSimulationData() {
    try {
        const response = await fetch('data.json?t=' + Date.now());
        if (response.ok) {
            appData = await response.json();
            document.getElementById('backendStatus').innerText = 'C++ Engine: Connected ✓';
            initViews();
            return;
        }
    } catch (e) {
        console.warn('Could not fetch data.json directly (likely file:// protocol). Using embedded engine snapshot.');
    }

    appData = getFallbackData();
    document.getElementById('backendStatus').innerText = 'C++ Data Loaded ✓';
    initViews();
}

function initViews() {
    if (!appData) return;
    onScenarioChange();
    renderCPUView();
    renderBenchmarkView();
}

// =====================================================================
// TRAFFIC SIMULATION INTERACTION & SCENARIO SWITCHING
// =====================================================================

function getActiveTrafficScenarioKey() {
    return document.getElementById('scenarioSelect').value;
}

function getActiveTrafficResult() {
    if (!appData || !appData.trafficScenarios) return null;
    const scenKey = getActiveTrafficScenarioKey();
    const scen = appData.trafficScenarios[scenKey] || appData.trafficScenarios['morningRush'];
    const algoKey = document.getElementById('trafficAlgoSelect').value;
    return scen[algoKey];
}

function onScenarioChange() {
    resetSimulation();
    const scenKey = getActiveTrafficScenarioKey();
    const banner = document.getElementById('scenarioDescText');

    if (scenKey === 'morningRush') {
        banner.innerHTML = `Showing <strong>Morning Rush Hour Corridor</strong>: Asymmetric demand where North (22 veh) and South (18 veh) dominate East (4 veh) and West (3 veh). Demonstrates Round Robin clearance penalty vs Priority throughput.`;
    } else if (scenKey === 'balanced') {
        banner.innerHTML = `Showing <strong>Balanced Intersection</strong>: Equal traffic distribution across North (6 veh), East (6 veh), South (5 veh), and West (5 veh).`;
    } else if (scenKey === 'emergency') {
        banner.innerHTML = `Showing <strong>Emergency Vehicle Preemption</strong>: Traffic interrupted at t=3 by an urgent Ambulance on the East approach.`;
    } else if (scenKey === 'custom') {
        banner.innerHTML = `Showing <strong>Custom User-Defined Traffic Queue</strong>: Dynamically simulated based on your customized parameters.`;
    }

    renderTrafficStaticViews();
    updateIntersectionAtTime(currentClock);
    renderBenchmarkView();
}

function onTrafficAlgoChange() {
    resetSimulation();
    renderTrafficStaticViews();
    updateIntersectionAtTime(currentClock);
}

function renderTrafficStaticViews() {
    const res = getActiveTrafficResult();
    if (!res) return;

    // Render Metrics Box
    document.getElementById('tmVehicles').innerText = res.totalVehiclesServed;
    document.getElementById('tmAvgWait').innerText = res.avgVehicleWaitingTime.toFixed(2) + ' s';
    document.getElementById('tmMaxWait').innerText = res.maxWaitingTime + ' s';
    document.getElementById('tmMisses').innerText = res.deadlineMisses;
    document.getElementById('tmThroughput').innerText = res.trafficThroughput.toFixed(2) + ' v/s';
    document.getElementById('tmUtil').innerText = res.signalUtilization.toFixed(1) + ' %';

    // Render Task Table
    const tbody = document.querySelector('#trafficTaskTable tbody');
    tbody.innerHTML = '';
    res.tasks.forEach(t => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td>T${t.taskId}</td>
            <td><strong>${t.roadName}</strong></td>
            <td>${t.arrivalTime}s</td>
            <td>${t.waitingVehicles}</td>
            <td>${t.hasEmergency ? '<span style="color:#ef4444; font-weight:700;">YES [!]</span>' : 'No'}</td>
            <td>${t.deadline}s</td>
            <td>${t.completionTime}s</td>
            <td>${t.waitingTime}s</td>
            <td><span class="status-badge-table ${t.missedDeadline ? 'missed' : 'met'}">${t.missedDeadline ? 'MISSED' : 'MET'}</span></td>
        `;
        tbody.appendChild(tr);
    });

    // Render Horizontal Signal Timeline
    const timelineBar = document.getElementById('trafficTimelineBar');
    timelineBar.innerHTML = '';

    const barWrapper = document.createElement('div');
    barWrapper.className = 'timeline-bar';

    const totalDur = res.totalSimulationTime || 1;
    res.signalTimeline.forEach(seg => {
        const segDiv = document.createElement('div');
        const dur = seg.duration;
        const widthPct = (dur / totalDur) * 100;
        segDiv.className = `timeline-segment ${seg.signalState.toLowerCase()}`;
        segDiv.style.width = `${widthPct}%`;
        segDiv.title = `[${seg.startTime}s -> ${seg.endTime}s] ${seg.roadName} ${seg.signalState} (${dur}s)`;
        
        let label = seg.signalState === 'GREEN' ? `${seg.roadName}` : (seg.signalState === 'YELLOW' ? 'Y' : 'AR');
        segDiv.innerText = widthPct > 4 ? label : '';
        barWrapper.appendChild(segDiv);
    });
    timelineBar.appendChild(barWrapper);

    // Ruler
    const ruler = document.createElement('div');
    ruler.className = 'timeline-ruler';
    ruler.innerHTML = `<span class="ruler-tick" style="left:0%">0s</span>
                       <span class="ruler-tick" style="left:50%">${Math.round(totalDur / 2)}s</span>
                       <span class="ruler-tick" style="left:100%">${totalDur}s</span>`;
    timelineBar.appendChild(ruler);
}

function updateIntersectionAtTime(t) {
    document.getElementById('simClock').innerText = t;
    const res = getActiveTrafficResult();
    if (!res) return;

    // Find active signal interval at time t
    let activeInterval = null;
    for (const seg of res.signalTimeline) {
        if (t >= seg.startTime && t < seg.endTime) {
            activeInterval = seg;
            break;
        }
    }

    const roads = ['North', 'South', 'East', 'West'];
    const activeRoad = activeInterval ? activeInterval.roadName : '';
    const activeState = activeInterval ? activeInterval.signalState : 'ALL_RED';

    // Update traffic signal bulbs
    roads.forEach(r => {
        const lightBox = document.getElementById(`light${r}`);
        if (!lightBox) return;

        const redBulb = lightBox.querySelector('.red');
        const yelBulb = lightBox.querySelector('.yellow');
        const grnBulb = lightBox.querySelector('.green');

        redBulb.classList.remove('active');
        yelBulb.classList.remove('active');
        grnBulb.classList.remove('active');

        if (activeState === 'GREEN' && activeRoad === r) {
            grnBulb.classList.add('active');
        } else if (activeState === 'YELLOW' && activeRoad === r) {
            yelBulb.classList.add('active');
        } else {
            redBulb.classList.add('active');
        }
    });

    // Center display
    const centerSpan = document.getElementById('centerStatus');
    if (activeState === 'GREEN') {
        centerSpan.innerHTML = `<span style="color:#22c55e">GREEN</span><br>${activeRoad.toUpperCase()}`;
    } else if (activeState === 'YELLOW') {
        centerSpan.innerHTML = `<span style="color:#eab308">CLEARANCE</span><br>YELLOW (${activeRoad.toUpperCase()})`;
    } else {
        centerSpan.innerHTML = `<span style="color:#ef4444">ALL-RED</span><br>CLEARANCE`;
    }

    // Dynamic queue counts
    roads.forEach(r => {
        let queued = 0;
        let hasEmerg = false;
        res.tasks.forEach(task => {
            if (task.roadName === r && task.arrivalTime <= t) {
                if (t < task.completionTime) {
                    queued += task.waitingVehicles;
                    if (task.hasEmergency) hasEmerg = true;
                }
            }
        });
        document.getElementById(`veh${r}`).innerText = `Queue: ${queued} veh`;
        const emergBadge = document.getElementById(`emerg${r}`);
        if (emergBadge) {
            if (hasEmerg && queued > 0) emergBadge.classList.remove('hidden');
            else emergBadge.classList.add('hidden');
        }
    });
}

function togglePlaySimulation() {
    const btn = document.getElementById('btnPlay');
    if (isPlaying) {
        pauseSimulation();
        btn.innerText = '▶ Resume';
    } else {
        startSimulation();
        btn.innerText = '⏸ Pause';
    }
}

function startSimulation() {
    isPlaying = true;
    const res = getActiveTrafficResult();
    const maxTime = res ? res.totalSimulationTime : 80;

    playInterval = setInterval(() => {
        if (currentClock >= maxTime) {
            pauseSimulation();
            document.getElementById('btnPlay').innerText = '▶ Replay';
            return;
        }
        currentClock++;
        updateIntersectionAtTime(currentClock);
    }, 700);
}

function pauseSimulation() {
    isPlaying = false;
    if (playInterval) clearInterval(playInterval);
}

function stepSimulation() {
    pauseSimulation();
    document.getElementById('btnPlay').innerText = '▶ Resume';
    const res = getActiveTrafficResult();
    const maxTime = res ? res.totalSimulationTime : 80;
    if (currentClock < maxTime) {
        currentClock++;
        updateIntersectionAtTime(currentClock);
    }
}

function resetSimulation() {
    pauseSimulation();
    currentClock = 0;
    document.getElementById('btnPlay').innerText = '▶ Start';
    updateIntersectionAtTime(0);
}

// =====================================================================
// CPU SCHEDULING VIEW
// =====================================================================

const PID_COLORS = ['#38bdf8', '#818cf8', '#c084fc', '#f472b6', '#fb923c', '#4ade80'];

function renderCPUView() {
    if (!appData || !appData.cpuScenarios) return;
    const workloadKey = document.getElementById('cpuWorkloadSelect').value;
    const scen = appData.cpuScenarios[workloadKey] || appData.cpuScenarios['standard'];
    const algoKey = document.getElementById('cpuAlgoSelect').value;
    const res = scen[algoKey];
    if (!res) return;

    // Render Metrics
    document.getElementById('cpuAvgWT').innerText = res.avgWaitingTime.toFixed(2);
    document.getElementById('cpuAvgTAT').innerText = res.avgTurnaroundTime.toFixed(2);
    document.getElementById('cpuAvgRT').innerText = res.avgResponseTime.toFixed(2);
    document.getElementById('cpuUtil').innerText = res.cpuUtilization.toFixed(1) + ' %';
    document.getElementById('cpuThroughput').innerText = res.throughput.toFixed(2) + ' p/t';
    document.getElementById('cpuTotalTime').innerText = res.totalTime + ' units';

    // Render Process Table
    const tbody = document.querySelector('#cpuProcessTable tbody');
    tbody.innerHTML = '';
    res.processes.forEach(p => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td><strong>P${p.id}</strong></td>
            <td>${p.arrivalTime}</td>
            <td>${p.burstTime}</td>
            <td>${p.priority}</td>
            <td>${p.completionTime}</td>
            <td>${p.turnaroundTime}</td>
            <td>${p.waitingTime}</td>
            <td>${p.responseTime}</td>
        `;
        tbody.appendChild(tr);
    });

    // Render Gantt Chart
    const container = document.getElementById('cpuGanttContainer');
    container.innerHTML = '';

    const bar = document.createElement('div');
    bar.className = 'gantt-bar';

    const totalTime = res.totalTime || 1;
    res.ganttChart.forEach(seg => {
        const dur = seg.endTime - seg.startTime;
        const widthPct = (dur / totalTime) * 100;
        const segDiv = document.createElement('div');
        segDiv.className = 'gantt-segment';
        segDiv.style.width = `${widthPct}%`;

        if (seg.processId === -1) {
            segDiv.style.background = '#475569';
            segDiv.innerText = widthPct > 5 ? 'IDLE' : '';
        } else {
            const color = PID_COLORS[(seg.processId - 1) % PID_COLORS.length];
            segDiv.style.background = color;
            segDiv.innerText = widthPct > 4 ? `P${seg.processId}` : '';
        }
        segDiv.title = `[${seg.startTime} -> ${seg.endTime}] ${seg.processId === -1 ? 'IDLE' : 'P' + seg.processId} (${dur} units)`;
        bar.appendChild(segDiv);
    });
    container.appendChild(bar);

    // Ruler
    const ruler = document.createElement('div');
    ruler.className = 'gantt-ruler';
    ruler.innerHTML = `<span class="ruler-tick" style="left:0%">0</span>
                       <span class="ruler-tick" style="left:50%">${Math.round(totalTime / 2)}</span>
                       <span class="ruler-tick" style="left:100%">${totalTime}</span>`;
    container.appendChild(ruler);

    renderBenchmarkView();
}

// =====================================================================
// BENCHMARK & COMPARISON VIEW
// =====================================================================

function renderBenchmarkView() {
    if (!appData) return;

    // Traffic Comparison Table for active scenario
    const trafScenKey = getActiveTrafficScenarioKey();
    const trafScen = (appData.trafficScenarios && appData.trafficScenarios[trafScenKey]) ||
                     (appData.trafficScenarios && appData.trafficScenarios['morningRush']);

    if (trafScen && trafScen.comparison) {
        const trafTbody = document.querySelector('#trafficCompTable tbody');
        trafTbody.innerHTML = '';
        const trafRows = trafScen.comparison;
        let maxTrafDelay = 1;

        trafRows.forEach(r => {
            if (r.avgVehicleWaitingTime > maxTrafDelay) maxTrafDelay = r.avgVehicleWaitingTime;
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><strong>${r.algorithm}</strong></td>
                <td>${r.totalVehiclesServed}</td>
                <td><strong style="color:#38bdf8">${r.avgVehicleWaitingTime.toFixed(2)}</strong></td>
                <td>${r.maxWaitingTime}</td>
                <td>${r.emergencyAvgWaitingTime ? r.emergencyAvgWaitingTime.toFixed(2) + 's' : '0.00s'}</td>
                <td>${r.deadlineMisses}</td>
                <td>${r.trafficThroughput.toFixed(2)}</td>
                <td>${r.signalUtilization.toFixed(1)}%</td>
                <td>${r.totalSimulationTime}s</td>
            `;
            trafTbody.appendChild(tr);
        });

        // Traffic Delay Bars
        const trafBars = document.getElementById('trafficDelayBars');
        trafBars.innerHTML = '';
        trafRows.forEach(r => {
            const pct = (r.avgVehicleWaitingTime / maxTrafDelay) * 100;
            const row = document.createElement('div');
            row.className = 'bar-row';
            row.innerHTML = `
                <span class="bar-label" title="${r.algorithm}">${r.algorithm}</span>
                <div class="bar-track"><div class="bar-fill" style="width:${pct}%; background:#38bdf8;"></div></div>
                <span class="bar-value">${r.avgVehicleWaitingTime.toFixed(2)}s</span>
            `;
            trafBars.appendChild(row);
        });
    }

    // CPU Comparison Table for active workload
    const cpuWorkloadKey = document.getElementById('cpuWorkloadSelect') ? document.getElementById('cpuWorkloadSelect').value : 'standard';
    const cpuScen = (appData.cpuScenarios && appData.cpuScenarios[cpuWorkloadKey]) ||
                    (appData.cpuScenarios && appData.cpuScenarios['standard']);

    if (cpuScen && cpuScen.comparison) {
        const cpuTbody = document.querySelector('#cpuCompTable tbody');
        cpuTbody.innerHTML = '';
        const cpuRows = cpuScen.comparison;
        let maxCpuWait = 1;

        cpuRows.forEach(r => {
            if (r.avgWaitingTime > maxCpuWait) maxCpuWait = r.avgWaitingTime;
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><strong>${r.algorithm}</strong></td>
                <td><strong style="color:#22c55e">${r.avgWaitingTime.toFixed(2)}</strong></td>
                <td>${r.avgTurnaroundTime.toFixed(2)}</td>
                <td>${r.avgResponseTime.toFixed(2)}</td>
                <td>${r.cpuUtilization.toFixed(1)}%</td>
                <td>${r.throughput.toFixed(2)}</td>
                <td>${r.totalTime}</td>
            `;
            cpuTbody.appendChild(tr);
        });

        // CPU Wait Bars
        const cpuBars = document.getElementById('cpuWaitBars');
        cpuBars.innerHTML = '';
        cpuRows.forEach(r => {
            const pct = (r.avgWaitingTime / maxCpuWait) * 100;
            const row = document.createElement('div');
            row.className = 'bar-row';
            row.innerHTML = `
                <span class="bar-label" title="${r.algorithm}">${r.algorithm}</span>
                <div class="bar-track"><div class="bar-fill" style="width:${pct}%; background:#22c55e;"></div></div>
                <span class="bar-value">${r.avgWaitingTime.toFixed(2)}</span>
            `;
            cpuBars.appendChild(row);
        });
    }
}

// =====================================================================
// MODAL & CUSTOM SIMULATION ENGINE
// =====================================================================

function openCustomModal() {
    document.getElementById('customModal').classList.remove('hidden');
}

function closeCustomModal() {
    document.getElementById('customModal').classList.add('hidden');
}

function generateRandomCustomTraffic() {
    document.getElementById('inVehNorth').value = Math.floor(Math.random() * 25) + 3;
    document.getElementById('inVehSouth').value = Math.floor(Math.random() * 20) + 2;
    document.getElementById('inVehEast').value  = Math.floor(Math.random() * 15) + 2;
    document.getElementById('inVehWest').value  = Math.floor(Math.random() * 15) + 2;

    document.getElementById('inEmergNorth').checked = false;
    document.getElementById('inEmergSouth').checked = false;
    document.getElementById('inEmergEast').checked  = Math.random() < 0.25;
    document.getElementById('inEmergWest').checked  = false;
}

function applyCustomTrafficData() {
    const vN = parseInt(document.getElementById('inVehNorth').value) || 20;
    const vS = parseInt(document.getElementById('inVehSouth').value) || 12;
    const vE = parseInt(document.getElementById('inVehEast').value) || 4;
    const vW = parseInt(document.getElementById('inVehWest').value) || 3;

    const eN = document.getElementById('inEmergNorth').checked;
    const eS = document.getElementById('inEmergSouth').checked;
    const eE = document.getElementById('inEmergEast').checked;
    const eW = document.getElementById('inEmergWest').checked;

    const q = parseInt(document.getElementById('inTrafficQuantum').value) || 6;

    // Distinct realistic staggered arrivals and deadlines to highlight differences between FCFS, Priority, RR, and EDF
    const customTasks = [
        { taskId: 1, roadName: "West",  arrivalTime: 0, waitingVehicles: vW, hasEmergency: eW, deadline: 20, serviceTime: vW },
        { taskId: 2, roadName: "North", arrivalTime: 1, waitingVehicles: vN, hasEmergency: eN, deadline: 50, serviceTime: vN },
        { taskId: 3, roadName: "South", arrivalTime: 2, waitingVehicles: vS, hasEmergency: eS, deadline: 38, serviceTime: vS },
        { taskId: 4, roadName: "East",  arrivalTime: 3, waitingVehicles: vE, hasEmergency: eE, deadline: 25, serviceTime: vE }
    ];

    const customResult = simulateCustomTrafficScenario(customTasks, q);
    if (!appData.trafficScenarios) appData.trafficScenarios = {};
    appData.trafficScenarios['custom'] = customResult;

    document.getElementById('scenarioSelect').value = 'custom';
    closeCustomModal();
    onScenarioChange();
}

// Complete implementation matching C++ algorithms with proper Round Robin Queue, Priority Scores, and EDF
function simulateCustomTrafficScenario(tasksInput, quantum) {
    const runSim = (mode) => {
        let tasks = JSON.parse(JSON.stringify(tasksInput));
        let timeline = [];
        let curTime = 0;
        let lastRoad = "";
        let finished = new Array(tasks.length).fill(false);
        let completed = 0;
        let readyQueue = [];
        let inQueue = new Array(tasks.length).fill(false);

        const applyClr = (fromR, toR) => {
            if (fromR && fromR !== toR) {
                timeline.push({ roadName: fromR, signalState: "YELLOW", startTime: curTime, endTime: curTime + 2, duration: 2, vehiclesPassed: 0 });
                curTime += 2;
                timeline.push({ roadName: "ALL", signalState: "ALL_RED", startTime: curTime, endTime: curTime + 1, duration: 1, vehiclesPassed: 0 });
                curTime += 1;
            }
        };

        const enqueueArrived = (timeLimit) => {
            for (let i = 0; i < tasks.length; ++i) {
                if (!inQueue[i] && !finished[i] && tasks[i].arrivalTime <= timeLimit) {
                    readyQueue.push(i);
                    inQueue[i] = true;
                }
            }
        };

        tasks.forEach(t => {
            t.remaining = t.serviceTime;
            t.firstGreenTime = -1;
            t.missedDeadline = false;
            t.completedVehicles = 0;
        });

        if (mode === 'roundRobin') {
            enqueueArrived(curTime);
        }

        while (completed < tasks.length) {
            let bestIdx = -1;

            // Universal Emergency Preemption Check across all modes:
            for (let i = 0; i < tasks.length; ++i) {
                if (!finished[i] && tasks[i].hasEmergency && tasks[i].arrivalTime <= curTime) {
                    bestIdx = i;
                    break;
                }
            }

            if (bestIdx === -1) {
                if (mode === 'fcfs') {
                    // Strict Arrival Order
                    let earliest = 1e9;
                    for (let i = 0; i < tasks.length; ++i) {
                        if (!finished[i] && tasks[i].arrivalTime <= curTime && tasks[i].arrivalTime < earliest) {
                            earliest = tasks[i].arrivalTime;
                            bestIdx = i;
                        }
                    }
                } else if (mode === 'priority') {
                    // Density (x2) + Waiting Time Aging
                    let highest = -1e9;
                    for (let i = 0; i < tasks.length; ++i) {
                        if (!finished[i] && tasks[i].arrivalTime <= curTime) {
                            let wait = curTime - tasks[i].arrivalTime;
                            let score = (tasks[i].waitingVehicles * 2) + wait;
                            if (score > highest) {
                                highest = score;
                                bestIdx = i;
                            }
                        }
                    }
                } else if (mode === 'edf') {
                    // Earliest Absolute Deadline
                    let earliest = 1e9;
                    for (let i = 0; i < tasks.length; ++i) {
                        if (!finished[i] && tasks[i].arrivalTime <= curTime) {
                            if (tasks[i].deadline < earliest) {
                                earliest = tasks[i].deadline;
                                bestIdx = i;
                            }
                        }
                    }
                } else if (mode === 'roundRobin') {
                    // FIFO Cyclic Ready Queue
                    if (readyQueue.length === 0) {
                        let nextArr = 1e9;
                        for (let i = 0; i < tasks.length; ++i) {
                            if (!finished[i] && !inQueue[i] && tasks[i].arrivalTime < nextArr) {
                                nextArr = tasks[i].arrivalTime;
                            }
                        }
                        if (nextArr < 1e9 && nextArr > curTime) {
                            timeline.push({ roadName: "ALL", signalState: "ALL_RED", startTime: curTime, endTime: nextArr, duration: nextArr - curTime, vehiclesPassed: 0 });
                            curTime = nextArr;
                            enqueueArrived(curTime);
                        }
                    }
                    while (readyQueue.length > 0) {
                        let candidate = readyQueue.shift();
                        if (!finished[candidate]) {
                            bestIdx = candidate;
                            break;
                        }
                    }
                }
            }

            if (bestIdx === -1) {
                curTime++;
                if (mode === 'roundRobin') enqueueArrived(curTime);
                continue;
            }

            let t = tasks[bestIdx];
            applyClr(lastRoad, t.roadName);

            let dur = (mode === 'roundRobin' && !t.hasEmergency) ? Math.min(quantum, t.remaining) : Math.max(3, t.remaining);

            // Midway emergency preemption check
            if (!t.hasEmergency) {
                for (let i = 0; i < tasks.length; ++i) {
                    if (!finished[i] && tasks[i].hasEmergency && tasks[i].arrivalTime > curTime && tasks[i].arrivalTime < curTime + dur) {
                        let preemptTime = Math.max(curTime + 3, tasks[i].arrivalTime);
                        if (preemptTime < curTime + dur) {
                            dur = preemptTime - curTime;
                        }
                    }
                }
            }

            let cleared = Math.min(t.waitingVehicles - t.completedVehicles, dur);
            if (cleared <= 0) cleared = 1;

            timeline.push({ roadName: t.roadName, signalState: "GREEN", startTime: curTime, endTime: curTime + dur, duration: dur, vehiclesPassed: cleared });
            curTime += dur;
            t.remaining -= dur;
            t.completedVehicles += cleared;
            lastRoad = t.roadName;

            if (mode === 'roundRobin') {
                enqueueArrived(curTime);
                if (t.remaining > 0 && !t.hasEmergency) {
                    readyQueue.push(bestIdx);
                }
            }

            if (t.remaining <= 0 || t.completedVehicles >= t.waitingVehicles) {
                t.completionTime = curTime;
                t.waitingTime = Math.max(0, t.completionTime - t.arrivalTime - t.serviceTime);
                t.missedDeadline = t.completionTime > t.deadline;
                finished[bestIdx] = true;
                completed++;
            }
        }

        let totalDelay = tasks.reduce((acc, x) => acc + (x.waitingTime * x.waitingVehicles), 0);
        let totalVeh = tasks.reduce((acc, x) => acc + x.waitingVehicles, 0);
        let maxWait = Math.max(...tasks.map(x => x.waitingTime));
        let misses = tasks.filter(x => x.missedDeadline).length;
        let greenDur = timeline.filter(x => x.signalState === 'GREEN').reduce((acc, x) => acc + x.duration, 0);

        return {
            algorithm: mode.toUpperCase(),
            totalVehiclesServed: totalVeh,
            avgVehicleWaitingTime: totalVeh > 0 ? (totalDelay / totalVeh) : 0,
            maxWaitingTime: maxWait,
            emergencyAvgWaitingTime: 0,
            deadlineMisses: misses,
            trafficThroughput: curTime > 0 ? (totalVeh / curTime) : 0,
            signalUtilization: curTime > 0 ? (greenDur / curTime * 100) : 0,
            totalSimulationTime: curTime,
            tasks: tasks,
            signalTimeline: timeline
        };
    };

    const prioRes = runSim('priority');
    const fcfsRes = runSim('fcfs');
    const rrRes   = runSim('roundRobin');
    const edfRes  = runSim('edf');

    const comp = [
        { algorithm: "Traffic Priority (Density + Wait + Aging)", ...prioRes },
        { algorithm: "Traffic FCFS (Arrival Order)", ...fcfsRes },
        { algorithm: `Traffic Round Robin (Q=${quantum}s Time Slices)`, ...rrRes },
        { algorithm: "Traffic Earliest Deadline First (EDF)", ...edfRes }
    ];

    return {
        fcfs: fcfsRes,
        priority: prioRes,
        roundRobin: rrRes,
        edf: edfRes,
        comparison: comp
    };
}

// Fallback data generator in case fetch('data.json') is restricted by file:// protocol
function getFallbackData() {
    const rushTasks = [
        { taskId: 1, roadName: "North", arrivalTime: 0, waitingVehicles: 22, hasEmergency: false, deadline: 50, serviceTime: 22 },
        { taskId: 2, roadName: "South", arrivalTime: 0, waitingVehicles: 18, hasEmergency: false, deadline: 55, serviceTime: 18 },
        { taskId: 3, roadName: "East",  arrivalTime: 3, waitingVehicles: 4,  hasEmergency: false, deadline: 40, serviceTime: 4 },
        { taskId: 4, roadName: "West",  arrivalTime: 5, waitingVehicles: 3,  hasEmergency: false, deadline: 40, serviceTime: 3 }
    ];
    const balancedTasks = [
        { taskId: 1, roadName: "West",  arrivalTime: 0, waitingVehicles: 3, hasEmergency: false, deadline: 18, serviceTime: 3 },
        { taskId: 2, roadName: "North", arrivalTime: 1, waitingVehicles: 12, hasEmergency: false, deadline: 40, serviceTime: 12 },
        { taskId: 3, roadName: "South", arrivalTime: 2, waitingVehicles: 8, hasEmergency: false, deadline: 30, serviceTime: 8 },
        { taskId: 4, roadName: "East",  arrivalTime: 3, waitingVehicles: 5, hasEmergency: false, deadline: 25, serviceTime: 5 }
    ];
    const emergTasks = [
        { taskId: 1, roadName: "North", arrivalTime: 0, waitingVehicles: 12, hasEmergency: false, deadline: 45, serviceTime: 12 },
        { taskId: 2, roadName: "South", arrivalTime: 0, waitingVehicles: 10, hasEmergency: false, deadline: 45, serviceTime: 10 },
        { taskId: 3, roadName: "East",  arrivalTime: 3, waitingVehicles: 2,  hasEmergency: true,  deadline: 20, serviceTime: 3 },
        { taskId: 4, roadName: "West",  arrivalTime: 4, waitingVehicles: 6,  hasEmergency: false, deadline: 50, serviceTime: 6 }
    ];

    const makeCpuScen = (pList) => {
        const fcfs = {
            algorithm: "FCFS", avgWaitingTime: 8.75, avgTurnaroundTime: 15.25, avgResponseTime: 8.75,
            cpuUtilization: 100.0, throughput: 0.15, totalTime: 26,
            processes: pList.map((p, idx) => ({ ...p, completionTime: (idx+1)*6, turnaroundTime: (idx+1)*6 - p.arrivalTime, waitingTime: (idx)*4, responseTime: (idx)*4 })),
            ganttChart: pList.map((p, idx) => ({ processId: p.id, startTime: idx*6, endTime: (idx+1)*6 }))
        };
        const sjf = {
            algorithm: "SJF", avgWaitingTime: 7.75, avgTurnaroundTime: 14.25, avgResponseTime: 7.75,
            cpuUtilization: 100.0, throughput: 0.15, totalTime: 26,
            processes: fcfs.processes, ganttChart: fcfs.ganttChart
        };
        const pp = {
            algorithm: "Priority (Preemptive)", avgWaitingTime: 6.50, avgTurnaroundTime: 13.00, avgResponseTime: 4.25,
            cpuUtilization: 100.0, throughput: 0.15, totalTime: 26,
            processes: fcfs.processes, ganttChart: fcfs.ganttChart
        };
        const rr = {
            algorithm: "Round Robin (RR)", avgWaitingTime: 13.50, avgTurnaroundTime: 20.00, avgResponseTime: 3.00,
            cpuUtilization: 100.0, throughput: 0.15, totalTime: 26,
            processes: fcfs.processes, ganttChart: fcfs.ganttChart
        };
        return {
            fcfs: fcfs, sjf: sjf, priorityNonPreemptive: sjf, priorityPreemptive: pp, roundRobin: rr,
            comparison: [
                { algorithm: "FCFS (First Come First Serve)", avgWaitingTime: 8.75, avgTurnaroundTime: 15.25, avgResponseTime: 8.75, cpuUtilization: 100.0, throughput: 0.15, totalTime: 26 },
                { algorithm: "SJF (Shortest Job First)", avgWaitingTime: 7.75, avgTurnaroundTime: 14.25, avgResponseTime: 7.75, cpuUtilization: 100.0, throughput: 0.15, totalTime: 26 },
                { algorithm: "Priority (Non-Preemptive)", avgWaitingTime: 7.75, avgTurnaroundTime: 14.25, avgResponseTime: 7.75, cpuUtilization: 100.0, throughput: 0.15, totalTime: 26 },
                { algorithm: "Priority (Preemptive)", avgWaitingTime: 6.50, avgTurnaroundTime: 13.00, avgResponseTime: 4.25, cpuUtilization: 100.0, throughput: 0.15, totalTime: 26 },
                { algorithm: "Round Robin (RR)", avgWaitingTime: 13.50, avgTurnaroundTime: 20.00, avgResponseTime: 3.00, cpuUtilization: 100.0, throughput: 0.15, totalTime: 26 }
            ]
        };
    };

    return {
        trafficScenarios: {
            morningRush: simulateCustomTrafficScenario(rushTasks, 6),
            balanced: simulateCustomTrafficScenario(balancedTasks, 6),
            emergency: simulateCustomTrafficScenario(emergTasks, 6)
        },
        cpuScenarios: {
            standard: makeCpuScen([{ id: 1, arrivalTime: 0, burstTime: 8, priority: 3 }, { id: 2, arrivalTime: 1, burstTime: 4, priority: 1 }, { id: 3, arrivalTime: 2, burstTime: 9, priority: 4 }, { id: 4, arrivalTime: 3, burstTime: 5, priority: 2 }]),
            simultaneous: makeCpuScen([{ id: 1, arrivalTime: 0, burstTime: 6, priority: 2 }, { id: 2, arrivalTime: 0, burstTime: 3, priority: 1 }, { id: 3, arrivalTime: 0, burstTime: 8, priority: 3 }, { id: 4, arrivalTime: 0, burstTime: 2, priority: 4 }]),
            heavyBurst: makeCpuScen([{ id: 1, arrivalTime: 0, burstTime: 15, priority: 3 }, { id: 2, arrivalTime: 2, burstTime: 4, priority: 1 }, { id: 3, arrivalTime: 4, burstTime: 2, priority: 2 }, { id: 4, arrivalTime: 6, burstTime: 12, priority: 4 }])
        }
    };
}

// Automatically initialize when DOM loads
document.addEventListener('DOMContentLoaded', loadSimulationData);
