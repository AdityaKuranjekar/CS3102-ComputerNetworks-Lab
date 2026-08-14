# Lab Assignment 1: Simulation of Router Queueing Delay

> **Course:** CS3102 — Computer Networks Lab

---

## Table of Contents
1. [Objective](#1-objective)
2. [Problem Statement](#2-problem-statement)
3. [Background Theory](#3-background-theory)
4. [Mathematical Formulas](#4-mathematical-formulas-used)
5. [Code Walkthrough](#5-code-walkthrough-simulationc)
6. [Compilation & Execution](#6-compilation--execution)
7. [Simulation Results](#7-simulation-results)
8. [Graph Analysis](#8-graph-analysis--plots)
9. [Analysis Questions](#9-analysis-questions)
10. [Submission Contents](#10-submission-contents)

---

## 1. Objective

Develop a C program to simulate a simple **packet-switched network** consisting of a source, a router with a bounded FIFO queue, and a destination. The goal is to study — through simulation rather than pure theory — how **traffic intensity (ρ)** affects:

- Queueing delay
- End-to-end delay
- Packet-drop probability

This mirrors the real-world behavior of routers: as a link gets more heavily loaded, packets spend longer waiting in buffers, and once the buffer is full, packets start getting dropped.

---

## 2. Problem Statement

Design and simulate a network path of the form:

```
Source ─────────► Router (Bounded FIFO Queue) ─────────► Destination
        (S–R link)                              (R–D link)
```

- The **source** generates packets according to a **Poisson process** — inter-arrival times are exponentially distributed.
- Each packet travels over the **S–R link**, arrives at the router, and enters a **finite FIFO queue** of capacity `B` packets.
- The router services packets **one at a time** in FIFO order, applying a processing delay before forwarding over the **R–D link**.
- If a packet arrives when the queue holds `B` packets, it is **dropped** — modeling a real router's finite buffer.

The simulation runs across a range of traffic intensities and reports standardized performance statistics for each.

### Network Parameters

| Parameter | Symbol | Value |
|-----------|--------|-------|
| Packet length | `L` | 1000 bytes (8000 bits) |
| S–R link bandwidth | `R_sr` | 10 Mbps |
| R–D link bandwidth (bottleneck) | `R_rd` | 1 Mbps |
| S–R propagation delay | `D_prop,sr` | 1 ms |
| R–D propagation delay | `D_prop,rd` | 2 ms |
| Router processing delay | `D_proc` | 0.5 ms |
| Queue capacity | `B` | 20 packets |
| Packets per experiment | `N` | 100,000 |
| Random seed | `seed` | 42 |

### Output

A [`results.csv`](results.csv) file with one row per ρ value, containing:

| Column | Description |
|--------|-------------|
| ρ | Traffic intensity |
| λ | Packet generation rate (pkts/s) |
| Generated | Total packets generated |
| Delivered | Packets successfully received |
| Dropped | Packets lost at router buffer |
| P_drop | Packet-drop probability |
| Avg Queue Delay | Average queueing delay (s) |
| Avg E2E Delay | Average end-to-end delay (s) |
| Max Queue | Maximum queue occupancy observed |

---

## 3. Background Theory

### 3.1 Why a Poisson / Exponential Model?

Network traffic (many independent users sending small packets) is classically modeled as a **Poisson arrival process**: arrivals happen independently at a constant average rate `λ`, and the **time between arrivals** is **exponentially distributed**. This is the standard assumption behind queueing models like **M/M/1** and **M/M/1/B**.

The exponential PDF and CDF:

```
f(t) = λ · e^(−λt),   t ≥ 0
F(t) = 1 − e^(−λt)
```

### 3.2 Inverse Transform Sampling

To draw a random exponential sample from `rand()` (which gives `U ~ Uniform(0,1)`), we invert the CDF:

```
U = 1 − e^(−λT)  ⟹  T = −ln(1 − U) / λ
```

This is why the simulation computes `-log(1-u)/lambda` for each inter-arrival time.

### 3.3 The FIFO Queue Model (M/M/1/B)

The router is modeled as a **single-server finite-buffer queue**:

| Component | Model Element |
|-----------|--------------|
| **Server** | Outgoing R–D link (one packet at a time) |
| **Buffer** | Up to `B = 20` packets |
| **Discipline** | FIFO (First-In-First-Out) |
| **Blocking** | Arriving packet dropped if buffer full |

### 3.4 Traffic Intensity (ρ)

```
ρ = (λ · L) / R_rd       [load on the bottleneck link]
λ = (ρ · R_rd) / L       [arrival rate for a target ρ]
```

| ρ Range | System Behavior |
|---------|----------------|
| ρ < 1 | Link keeps up on average — system is **stable** |
| ρ = 1 | Exactly at capacity — delay theoretically diverges |
| ρ > 1 | Arrivals exceed capacity — queue saturates, heavy loss |

---

## 4. Mathematical Formulas Used

| # | Formula | Meaning |
|---|---------|---------|
| 1 | `ρ = λL / R_rd` | Traffic intensity of the bottleneck link |
| 2 | `λ = ρ · R_rd / L` | Arrival rate for a target ρ |
| 3 | `T_inter = −ln(1−U) / λ` | Exponential inter-arrival time (Inverse Transform Sampling) |
| 4 | `D_trans,sr = L / R_sr` | Transmission delay on S–R link |
| 5 | `D_trans,rd = L / R_rd` | Transmission delay on R–D link |
| 6 | `T_router_arrival = T_gen + D_trans,sr + D_prop,sr` | Packet fully arrives at router |
| 7 | `T_service_start = max(T_router_arrival, T_last_departure)` | Service starts when link is free AND packet has arrived |
| 8 | `D_queue = T_service_start − T_router_arrival` | Queueing (waiting) delay at router |
| 9 | `T_router_departure = T_service_start + D_proc + D_trans,rd` | Packet finishes leaving router |
| 10 | `T_dest_arrival = T_router_departure + D_prop,rd` | Packet reaches destination |
| 11 | `D_e2e = T_dest_arrival − T_gen` | Total end-to-end delay |
| 12 | `P_drop = Dropped / Generated` | Packet-drop probability |
| 13 | `Avg_D_queue = Σ D_queue / Delivered` | Average queueing delay |

> **Key insight on Formula 7:** A packet cannot start service until *both* (a) it has physically arrived, and (b) the router has finished serving every packet ahead of it. The `max()` is what correctly produces queueing delay — if the router is idle, `D_queue = 0`; if busy, the packet waits.

---

## 5. Code Walkthrough ([`simulation.c`](simulation.c))

### 5.1 `get_exponential_interarrival(lambda)`

```c
double get_exponential_interarrival(double lambda) {
    double u = (double)rand() / RAND_MAX;
    while (u == 0 || u == 1) { ... }
    return -log(1.0 - u) / lambda;
}
```

Applies **Inverse Transform Sampling** (Formula 3) — re-rolls if `u` is 0 or 1 since `log(0)` is undefined.

### 5.2 `run_experiment(...)` — Core Simulation Loop

For each target `ρ`, the function:

1. **Computes λ** from ρ (Formula 2) and pre-computes fixed transmission delays.
2. **Maintains `departure_times[]`** — stores scheduled departure time of every packet queued at the router (the FIFO queue state).
3. **Loops over 100,000 packets**, and for each:

| Step | Action |
|------|--------|
| **Generate** | Draw inter-arrival time, advance `current_time` |
| **Router Arrival** | Compute `router_arrival_time` via Formula 6 |
| **Drain queue** | Remove packets whose departure is already past |
| **Admission** | If full → **drop**; else → compute delays, log stats, enqueue |

4. After all packets, compute averages (Formulas 12–13) and write to `results.csv`.

### 5.3 `main()`

- Sets all fixed network parameters.
- Seeds RNG once: `srand(42)` for reproducibility.
- Loops over 13 target ρ values `{0.1, 0.2, ..., 1.2}`, calling `run_experiment()` for each.

---

## 6. Compilation & Execution

```bash
# Compile (link math library for log())
gcc simulation.c -o simulation -lm

# Run simulation
./simulation

# Output produced:
#   results.csv  — 13 rows, one per ρ value
```

---

## 7. Simulation Results

Full data: [`results.csv`](results.csv)

| ρ | λ (pkts/s) | Generated | Delivered | Dropped | P_drop | Avg Queue Delay (ms) | Avg E2E Delay (ms) | Max Queue |
|-----|-----------|-----------|-----------|---------|--------|----------------------|--------------------|-----------|
| 0.1 | 12.5 | 100,000 | 100,000 | 0 | 0.000000 | ~0.09 | — | — |
| 0.2 | 25.0 | 100,000 | 100,000 | 0 | 0.000000 | ~0.22 | — | — |
| 0.4 | 50.0 | 100,000 | 100,000 | 0 | 0.000000 | ~0.77 | — | — |
| 0.6 | 75.0 | 100,000 | 100,000 | 0 | 0.000000 | ~2.8 | — | — |
| 0.8 | 100.0 | 100,000 | 100,000 | 0 | 0.000000 | ~11.7 | — | — |
| 0.9 | 112.5 | 100,000 | ~99,800 | ~200 | ~0.002 | ~22.5 | — | 20 |
| 1.0 | 125.0 | 100,000 | ~97,500 | ~2,500 | ~0.025 | ~32.0 | — | 20 |
| 1.1 | 137.5 | 100,000 | ~91,000 | ~9,000 | ~0.090 | ~36.0 | — | 20 |
| 1.2 | 150.0 | 100,000 | ~83,000 | ~17,000 | ~0.170 | ~38.0 | — | 20 |

> Exact values depend on your run. Open [`results.csv`](results.csv) for precise figures.

---

## 8. Graph Analysis & Plots

All three graphs are plotted from [`results.csv`](results.csv) using Google Sheets / Excel (X-axis = ρ column).

---

### Graph 1: ρ vs. Average Queueing Delay

![Traffic Intensity vs. Average Queueing Delay](Traffic%20Intensity%20vs.%20Average%20Queueing%20Delay.png)

**Analysis:**
- For ρ ≤ 0.6: delay stays low and near-flat — the router drains bursts quickly.
- As ρ → 1.0: delay rises **sharply and non-linearly**, following the M/M/1 result:
  `D_queue ∝ ρ / (1 − ρ)` → diverges as ρ → 1.
- For ρ > 1.0: delay **saturates** at roughly `B × D_trans,rd` — the finite buffer caps maximum wait.

---

### Graph 2: ρ vs. Average End-to-End Delay

![Traffic Intensity vs. Average End-to-End Delay](Traffic%20Intensity%20vs.%20Average%20End-to-End%20Delay.png)

**Analysis:**
- **Identical curve shape** to Graph 1, shifted upward by the constant physical delay baseline:
  ```
  D_base = D_trans,sr + D_prop,sr + D_proc + D_trans,rd + D_prop,rd
  ```
- This constant baseline is independent of traffic load — only `D_queue` varies with ρ.

---

### Graph 3: ρ vs. Packet-Drop Probability

![Traffic Intensity vs. Packet-Drop Probability](Traffic%20Intensity%20vs.%20Packet-Drop%20Probability.png)

**Analysis:**
- P_drop ≈ **0** for all ρ ≤ 0.8 — the buffer successfully absorbs all bursts.
- At ρ ≈ 0.9: drops begin as bursts occasionally fill the 20-packet buffer.
- For ρ > 1.0: drops **scale roughly linearly** — the router sheds exactly the fraction of traffic it cannot serve.

---

## 9. Analysis Questions

### Q1. Which delay components remain constant as ρ increases?

| Delay Component | Depends On | Constant? |
|----------------|-----------|-----------|
| Transmission delay (`D_trans`) | `L`, `R_sr`, `R_rd` (fixed) | ✅ Yes |
| Propagation delay (`D_prop`) | Physical distance (fixed) | ✅ Yes |
| Processing delay (`D_proc`) | Per-packet header inspection (fixed) | ✅ Yes |
| **Queueing delay** (`D_queue`) | Queue backlog → depends on ρ | ❌ No |

Only **queueing delay** varies with traffic intensity.

---

### Q2. Why does queueing delay increase rapidly near ρ = 1?

As `ρ → 1`, the arrival rate approaches the router's maximum service rate. Because Poisson arrivals are inherently **random and bursty**, short back-to-back bursts occur even when the average rate is below capacity. At low ρ, spare capacity absorbs these bursts. Near ρ = 1, spare capacity shrinks to nearly zero — the queue accumulates faster than it drains. The M/M/1 formula:

```
D_queue ∝ ρ / (1 − ρ)
```

diverges as `ρ → 1`, explaining the **near-exponential rise** in the simulation.

---

### Q3. What happens when ρ > 1?

When `ρ > 1`, arrivals permanently exceed service capacity — the system is **overloaded**.

| Metric | Effect |
|--------|--------|
| **Queueing Delay** | **Saturates** at ~`B × D_trans,rd` — finite buffer caps maximum wait |
| **Packet Loss** | **Rises steeply and roughly linearly** — queue stays full, most new packets are dropped |

---

### Q4. How does queue capacity `B` influence performance?

| `B` Size | Avg Delay | Drop Probability |
|----------|-----------|-----------------|
| **Larger** | ↑ Higher (longer lines) | ↓ Lower (more burst absorption) |
| **Smaller** | ↓ Lower (bounded backlog) | ↑ Higher (fills up faster) |

> **The Bufferbloat Trade-off:** Larger `B` reduces loss but increases delay for accepted packets. Smaller `B` keeps delay bounded but sacrifices more packets. This is a fundamental design trade-off between **loss-sensitive** and **delay-sensitive** applications.

---

## 10. Submission Contents

| File | Description |
|------|-------------|
| [`simulation.c`](simulation.c) | C source code |
| [`results.csv`](results.csv) | Simulation output — 13 rows, one per ρ |
| [`Traffic Intensity vs. Average Queueing Delay.png`](Traffic%20Intensity%20vs.%20Average%20Queueing%20Delay.png) | Graph 1 |
| [`Traffic Intensity vs. Average End-to-End Delay.png`](Traffic%20Intensity%20vs.%20Average%20End-to-End%20Delay.png) | Graph 2 |
| [`Traffic Intensity vs. Packet-Drop Probability.png`](Traffic%20Intensity%20vs.%20Packet-Drop%20Probability.png) | Graph 3 |
| [`Lab1-QueueSim.pdf`](Lab1-QueueSim.pdf) | Lab assignment PDF |
| `README.md` | This document |

---

*CS3102 — Computer Networks Lab | Lab Assignment 1*
