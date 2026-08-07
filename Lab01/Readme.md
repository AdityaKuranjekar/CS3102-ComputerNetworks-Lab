# Lab Assignment 1: Simulation of Router Queueing Delay
### Computer Networks Lab

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
        (S–R link)                          (R–D link)
```

- The **source** generates packets according to a **Poisson process** — inter-arrival times between packets are exponentially distributed.
- Each packet travels over the **source-to-router (S–R) link**, arrives at the router, and is placed into a **finite FIFO queue** of capacity `B` packets.
- The router services packets **one at a time**, in arrival order (First-In-First-Out), applying a processing delay before forwarding each packet over the **router-to-destination (R–D) link**.
- If a packet arrives when the queue already holds `B` packets, it is **dropped** (lost) — this models a real router's finite buffer memory.

The program must run this simulation for a range of traffic intensities and report standardized performance statistics for each.

### Input Parameters

| # | Parameter | Symbol | Units |
|---|-----------|--------|-------|
| 1 | Packet length | `L` | bytes |
| 2 | Source-to-router bandwidth | `R_sr` | bps |
| 3 | Router-to-destination bandwidth | `R_rd` | bps |
| 4 | Source-to-router propagation delay | `D_prop,sr` | seconds |
| 5 | Router-to-destination propagation delay | `D_prop,rd` | seconds |
| 6 | Router processing delay | `D_proc` | seconds/packet |
| 7 | Router queue capacity | `B` | packets |
| 8 | Packet generation rate | `λ` | packets/second |
| 9 | Number of packets to simulate | `N` | count |
| 10 | Random number seed | `seed` | integer |

### Output

A `results.csv` file with one row per experiment (per ρ value), containing:

- Traffic intensity (ρ)
- Packet generation rate (λ)
- Number of packets generated
- Number of packets delivered
- Number of packets dropped
- Packet-drop probability
- Average queueing delay
- Average end-to-end delay
- Maximum queue occupancy

### Required Graphs (plotted from `results.csv` in a spreadsheet)

1. Traffic Intensity (ρ) vs. Average Queueing Delay
2. Traffic Intensity (ρ) vs. Average End-to-End Delay
3. Traffic Intensity (ρ) vs. Packet-Drop Probability

---

## 3. Background Theory

### 3.1 Why a Poisson / Exponential Model?

Network traffic (many independent users sending small packets) is classically modeled as a **Poisson arrival process**: arrivals happen independently and at a constant average rate `λ`, and the **time between consecutive arrivals** is **exponentially distributed** with rate `λ`. This is the standard assumption behind queueing models like **M/M/1** and **M/M/1/B** (the "M" stands for "Markovian," i.e., memoryless/exponential).

The exponential PDF is:

$$f(t) = \lambda e^{-\lambda t}, \quad t \ge 0$$

and its CDF is:

$$F(t) = 1 - e^{-\lambda t}$$

### 3.2 Inverse Transform Sampling

To draw a random sample from this exponential distribution using only a uniform random number generator (`rand()` in C, which gives `U ~ Uniform(0,1)`), we invert the CDF:

$$U = 1 - e^{-\lambda T} \;\;\Longrightarrow\;\; T = -\frac{\ln(1-U)}{\lambda}$$

This is the **Inverse Transform Sampling** technique — it is why the simulation generates a uniform random number `u` and then computes `-log(1-u)/lambda` to get each inter-arrival time.

### 3.3 The FIFO Queue Model (M/M/1/B)

The router is modeled as a **single-server queue with finite buffer size `B`** (this is the M/M/1/B model):

- **Server** = the outgoing R–D link (transmits one packet at a time)
- **Buffer** = space for up to `B` packets waiting (including the one being served, depending on convention)
- **Discipline** = FIFO (First-In-First-Out)
- **Blocking** = if the buffer is full when a packet arrives, the packet is **dropped**, not queued

### 3.4 Traffic Intensity (ρ) — the Central Parameter

Traffic intensity (also called **utilization**) measures how loaded the output link is — the ratio of the rate at which "work" (bits) arrives to the rate at which the server can process it:

$$\boxed{\rho = \frac{\lambda \cdot L}{R_{rd}}}$$

where:
- `λ` = packet generation rate (packets/second)
- `L` = packet length in **bits** (`L_bytes × 8`)
- `R_rd` = router-to-destination link bandwidth (bits/second) — the **bottleneck** link's capacity

- `ρ < 1`: the link can — on average — keep up with incoming traffic; the system is **stable**.
- `ρ = 1`: the link is loaded at exactly its capacity; delay theoretically diverges.
- `ρ > 1`: arrivals permanently exceed service capacity; the queue saturates and stays full.

Since the lab fixes ρ and asks us to solve for the corresponding λ, we invert the formula:

$$\boxed{\lambda = \frac{\rho \cdot R_{rd}}{L}}$$

This is exactly what `run_experiment()` computes first, for each target ρ in `{0.1, 0.2, ..., 1.2}`.

---

## 4. Mathematical Formulas Used (Complete Reference)

| # | Formula | Meaning |
|---|---------|---------|
| 1 | $\rho = \dfrac{\lambda L}{R_{rd}}$ | Traffic intensity of the bottleneck (R–D) link |
| 2 | $\lambda = \dfrac{\rho \, R_{rd}}{L}$ | Arrival rate needed to hit a target ρ |
| 3 | $T_{inter} = -\dfrac{\ln(1-U)}{\lambda}$ | Exponential inter-arrival time (Inverse Transform Sampling), $U \sim \text{Uniform}(0,1)$ |
| 4 | $D_{trans,sr} = \dfrac{L}{R_{sr}}$ | Transmission delay of a packet onto the S–R link |
| 5 | $D_{trans,rd} = \dfrac{L}{R_{rd}}$ | Transmission delay of a packet onto the R–D link |
| 6 | $T_{gen}$ (given) | Time the packet is generated at the source |
| 7 | $T_{router\_arrival} = T_{gen} + D_{trans,sr} + D_{prop,sr}$ | Time the packet fully arrives at the router |
| 8 | $T_{service\_start} = \max(T_{router\_arrival},\ T_{last\_departure})$ | Service can only start once the link is free AND the packet has arrived |
| 9 | $D_{queue} = T_{service\_start} - T_{router\_arrival}$ | Queueing (waiting) delay at the router |
| 10 | $T_{router\_departure} = T_{service\_start} + D_{proc} + D_{trans,rd}$ | Time packet finishes leaving the router |
| 11 | $T_{dest\_arrival} = T_{router\_departure} + D_{prop,rd}$ | Time packet reaches the destination |
| 12 | $D_{end\text{-}to\text{-}end} = T_{dest\_arrival} - T_{gen}$ | Total delay experienced by the packet |
| 13 | $D_{end\text{-}to\text{-}end} = D_{trans,sr}+D_{prop,sr}+D_{queue}+D_{proc}+D_{trans,rd}+D_{prop,rd}$ | Equivalent decomposition into components |
| 14 | $P_{drop} = \dfrac{\text{Packets Dropped}}{\text{Packets Generated}}$ | Packet-drop (loss) probability |
| 15 | $\overline{D_{queue}} = \dfrac{\sum D_{queue}}{\text{Packets Delivered}}$ | Average queueing delay (over delivered packets) |
| 16 | $\overline{D_{end\text{-}to\text{-}end}} = \dfrac{\sum D_{end\text{-}to\text{-}end}}{\text{Packets Delivered}}$ | Average end-to-end delay (over delivered packets) |

**Note on formula 8:** this is the core of FIFO queue simulation. A packet cannot start service until *both* (a) it has physically arrived at the router, and (b) the router has finished transmitting every packet ahead of it. Taking the `max()` of these two times is what correctly produces queueing delay — if the router is idle when the packet arrives, `D_queue = 0`; if the router is still busy, the packet must wait.

---

## 5. Code Walkthrough (`simulation.c`)

### 5.1 `get_exponential_interarrival(lambda)`

```c
double get_exponential_interarrival(double lambda) {
    double u = (double)rand() / RAND_MAX;
    while (u == 0 || u == 1) { ... }
    return -log(1.0 - u) / lambda;
}
```

- Generates `u`, a uniform random number in `(0, 1)`.
- Re-rolls if `u` is exactly 0 or 1, since `log(0)` and `log(1)` are undefined/degenerate.
- Applies **Formula 3** (Inverse Transform Sampling) to convert `u` into an exponentially distributed inter-arrival time.
- This function is called once per packet to model Poisson arrivals.

### 5.2 `run_experiment(...)` — the Core Simulation Loop

For each target `ρ`, the function:

1. **Computes λ** from ρ using Formula 2, and pre-computes the fixed transmission delays `trans_sr`, `trans_rd` (Formulas 4–5).
2. **Maintains a `departure_times[]` array** — this acts as the FIFO queue's state, storing the scheduled departure time of every packet currently queued/in-service at the router.
3. **Loops over `total_packets` packets**, and for each one:
   - **Step A — Generate:** draws the next inter-arrival time and advances `current_time` to get the packet's `generation_time`.
   - **Step B — Arrival at router:** computes `router_arrival_time` using Formula 7.
   - **Step C — Update queue state:** removes any packets from `departure_times[]` whose departure time is already in the past relative to this packet's arrival (they've already left the router) — this keeps `current_queue_size` accurate.
   - **Step D — Admission control:**
     - If the queue is full (`current_queue_size >= queue_capacity`), the packet is **dropped**.
     - Otherwise, it is accepted:
       - `service_start_time` is computed via Formula 8 (`max` of arrival time and last queued packet's departure).
       - `queueing_delay` via Formula 9.
       - `router_departure_time` and `dest_arrival_time` via Formulas 10–11.
       - `end_to_end_delay` via Formula 12.
       - Running sums (`total_queueing_delay`, `total_end_to_end_delay`) and `max_queue_occupancy` are updated.
       - The new packet's departure time is pushed into `departure_times[]`, growing the simulated queue by one.
4. After all packets are processed, it computes the **averages and drop probability** (Formulas 14–16) and writes one row to `results.csv`.

### 5.3 `main()`

- Sets fixed network parameters (`L = 1000` bytes, `R_sr = 10` Mbps, `R_rd = 1` Mbps bottleneck, propagation/processing delays, `B = 20`, `N = 100000` packets, `seed = 42`).
- Seeds the RNG once with `srand(seed)` for reproducibility.
- Opens `results.csv` and writes the header row.
- Loops over all 13 target ρ values `{0.1, ..., 1.2}`, calling `run_experiment()` for each and appending a row of results.

---

## 6. Execution Instructions

### Build

```bash
gcc simulation.c -o simulation -lm
```

*(`-lm` links the math library, required for `log()`.)*

### Run

```bash
./simulation
```

This produces `results.csv` with 13 rows (one per ρ), which is then opened in a spreadsheet to plot the three required graphs.

### Expected Graph Shapes

- **ρ vs. Avg Queueing Delay:** stays low and flat for small ρ, then rises sharply near ρ = 1, then flattens out (saturates) for ρ > 1.
- **ρ vs. Avg End-to-End Delay:** same overall shape as queueing delay, just shifted upward by the constant transmission/propagation/processing delays.
- **ρ vs. Drop Probability:** near zero for ρ < 1, then increases roughly linearly once ρ exceeds 1.

---

## 7. Analysis Questions

### Q1. Which delay components remain approximately constant as traffic intensity increases?

- **Transmission Delay (`D_trans`):** Constant — depends only on the fixed packet size `L` and fixed link bandwidths `R_sr`, `R_rd` (Formulas 4–5), none of which change as ρ changes.
- **Propagation Delay (`D_prop`):** Constant — determined purely by physical link distance and signal propagation speed, independent of traffic load.
- **Processing Delay (`D_proc`):** Constant — the router spends a fixed, load-independent amount of time inspecting each packet's header.

Only **queueing delay** depends on traffic intensity, since it depends on how many packets are already waiting ahead of a given packet — which in turn depends on how loaded the link is.

### Q2. Why does the queueing delay increase rapidly as the traffic intensity approaches ρ = 1?

As `ρ → 1`, the arrival rate `λ` approaches the router's maximum service rate (`R_rd / L`). Because arrivals are **Poisson** (i.e., inherently random and "bursty" rather than perfectly spaced), short bursts of back-to-back arrivals occur even when the *average* rate is below capacity. At low ρ, the router has plenty of spare capacity to absorb these bursts and drain the queue between them. As ρ approaches 1, that spare capacity shrinks to almost nothing, so a burst can no longer be cleared before the next one arrives — the queue accumulates packets faster than it can drain them. This is the standard queueing-theory result (seen in M/M/1 models) where mean waiting time grows as:

$$\overline{D_{queue}} \propto \frac{\rho}{1-\rho}$$

which diverges as `ρ → 1`, explaining the sharp, **non-linear (near-exponential)** rise seen in the simulation.

### Q3. What happens when ρ > 1? Explain its effect on packet delay and packet loss.

When `ρ > 1`, the packet arrival rate permanently exceeds the router's service capacity — the system is **overloaded/unstable**.

- **Packet Delay:** Queueing delay stops growing without bound and instead **saturates** at a ceiling set by the finite buffer size `B`:
$$D_{queue,max} \approx B \times D_{trans,rd}$$
  This happens because the buffer can hold at most `B` packets — once full, delay for any newly *accepted* packet is capped at roughly the time to drain a full buffer.
- **Packet Loss:** Since the queue stays essentially full at all times under sustained overload, most newly arriving packets find no free buffer slot and are **dropped**. The drop probability rises **steeply, and roughly linearly**, with ρ once ρ exceeds 1 — because the router is now shedding, on average, exactly the excess fraction of traffic it cannot serve.

### Q4. How does router queue capacity influence:

**A. Average Packet Delay?**

- **Larger `B`:** More packets can accumulate in the buffer during congestion instead of being dropped, so **accepted** packets experience **higher average queueing delay** — they wait behind a longer line.
- **Smaller `B`:** Limits how much backlog can build up, which **caps maximum queueing delay** at a lower value, but at the cost of dropping more packets instead of queueing them.

**B. Packet-Drop Probability?**

- **Larger `B`:** Better absorbs short-term traffic bursts (temporary spikes above capacity), resulting in a **lower packet-drop probability** for the same offered load.
- **Smaller `B`:** Fills up quickly during bursts, resulting in a **higher packet-drop probability**.

**Summary — the delay/loss trade-off:** Queue capacity trades delay against loss. A larger buffer reduces packet loss but increases delay for packets that do get through (a phenomenon known in real networks as **bufferbloat**). A smaller buffer keeps delay bounded and low, but sacrifices more packets to drops under load. Choosing `B` is therefore a design trade-off between **loss-sensitive** and **delay-sensitive** application requirements.

---

## 8. Submission Contents

- `simulation.c` — C source code
- `results.csv` — Generated simulation output (13 rows, one per ρ)
- Spreadsheet (`.xlsx`) with the three required plots:
  1. ρ vs. Average Queueing Delay
  2. ρ vs. Average End-to-End Delay
  3. ρ vs. Packet-Drop Probability
- This `README.md` — problem statement, theory, formulas, and analysis answers
