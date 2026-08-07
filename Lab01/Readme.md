# Lab Assignment 1: Simulation of Router Queueing Delay

## Problem Statement

Develop a C program to simulate a simple packet-switched network consisting of a **source**, a **router with a bounded FIFO queue**, and a **destination**. Analyze the impact of traffic intensity (ρ) on queueing delay, end-to-end delay, and packet loss.

### Network Model

```
Source ─────────► Router (Bounded FIFO Queue) ─────────► Destination
```

The source generates packets according to a **Poisson process** (exponentially distributed inter-arrival times). The router processes incoming packets, stores them in a finite FIFO queue, and forwards them over a fixed-bandwidth output link. Packets arriving when the queue is full are **dropped**.

### Input Parameters

1. Packet length, `L` (bytes)
2. Source-to-router link bandwidth, `R_sr` (bps)
3. Router-to-destination link bandwidth, `R_rd` (bps)
4. Source-to-router propagation delay, `D_prop,sr` (seconds)
5. Router-to-destination propagation delay, `D_prop,rd` (seconds)
6. Router processing delay, `D_proc` (seconds/packet)
7. Router queue capacity, `B` (number of packets)
8. Packet generation rate, `λ` (packets/second)
9. Number of packets to simulate
10. Random number seed

### Output

A `results.csv` file containing, for each experiment:
- Traffic intensity (ρ)
- Packet generation rate (λ)
- Number of packets generated
- Number of packets delivered
- Number of packets dropped
- Packet-drop probability
- Average queueing delay
- Average end-to-end delay
- Maximum queue occupancy

---

## Mathematical Formulas Used

### Traffic Intensity (ρ)

$$\rho = \frac{\lambda \cdot L}{R_{rd}}$$

where:
- `λ` = Packet generation rate (packets/second)
- `L` = Packet length in bits (`L_bytes × 8`)
- `R_rd` = Router-to-destination link bandwidth (bits/second)

Since experiments are run by fixing ρ and solving for λ:

$$\lambda = \frac{\rho \cdot R_{rd}}{L}$$

### Poisson Arrival Process (Inter-Arrival Time)

Generated via Inverse Transform Sampling of the exponential distribution:

$$T_{inter} = -\frac{\ln(1 - U)}{\lambda}$$

where `U` is a uniform random variable in `(0, 1)`.

### Delay Components

- **Transmission Delay:**
$$D_{trans,sr} = \frac{L}{R_{sr}}, \qquad D_{trans,rd} = \frac{L}{R_{rd}}$$

- **Propagation Delay:** Fixed physical travel time across each link (`D_prop,sr`, `D_prop,rd`)

- **Processing Delay (`D_proc`):** Fixed router header-evaluation time per packet

- **Queueing Delay:**
$$D_{queue} = T_{service\_start} - T_{router\_arrival}$$

- **Total End-to-End Delay:**
$$D_{end\text{-}to\text{-}end} = D_{trans,sr} + D_{prop,sr} + D_{queue} + D_{proc} + D_{trans,rd} + D_{prop,rd}$$

### Drop Probability

$$P_{drop} = \frac{\text{Packets Dropped}}{\text{Packets Generated}}$$

---

## Execution

### Build

```bash
gcc simulation.c -o simulation -lm
```

### Run

```bash
./simulation
```

The program runs experiments across the following traffic intensities and writes results to `results.csv`:

$$\rho \in \{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.95, 1.0, 1.1, 1.2\}$$

### Graphs (generated from `results.csv` in a spreadsheet)

1. Traffic Intensity (ρ) vs. Average Queueing Delay
2. Traffic Intensity (ρ) vs. Average End-to-End Delay
3. Traffic Intensity (ρ) vs. Packet-Drop Probability

---

## Analysis Questions

### Q1. Which delay components remain approximately constant as traffic intensity increases?

- **Transmission Delay (`D_trans`):** Constant — it depends only on fixed packet size `L` and fixed link bandwidths `R_sr`, `R_rd`, neither of which changes as traffic intensity increases.
- **Propagation Delay (`D_prop`):** Constant — determined purely by the physical distance and signal propagation speed of the links, independent of traffic load.
- **Processing Delay (`D_proc`):** Constant — the router spends a fixed amount of time per packet on header inspection, regardless of how busy it is.

Only the **queueing delay** varies with traffic intensity, since it depends on how many packets are waiting ahead in the buffer.

### Q2. Why does the queueing delay increase rapidly as the traffic intensity approaches ρ = 1?

As `ρ → 1`, the average arrival rate `λ` approaches the router's maximum service rate (`R_rd / L`). Since packet arrivals follow a Poisson process, they occur in random, bursty patterns rather than at perfectly even intervals. As utilization nears 100%, the router has less and less spare capacity to absorb these bursts — the queue cannot fully drain between bursts of arrivals. This causes queue occupancy, and therefore queueing delay, to grow **non-linearly (steeply/exponentially)** rather than gradually, a classic result of queueing theory (e.g., M/M/1-type behavior where delay → ∞ as ρ → 1).

### Q3. What happens when ρ > 1? Explain its effect on packet delay and packet loss.

When `ρ > 1`, packets arrive faster than the router-to-destination link can transmit them, i.e., the arrival rate exceeds the service rate.

- **Packet Delay:** Queueing delay no longer grows without bound — it saturates at a maximum value set by the finite buffer size `B`, roughly:
$$D_{queue,max} \approx B \times D_{trans,rd}$$
  Once the queue is full, delay for accepted packets stays capped at this ceiling.
- **Packet Loss:** Since the queue is essentially always full under sustained overload, most newly arriving packets find no room and are dropped. The drop probability rises **steeply and roughly linearly** with ρ once ρ exceeds 1, since the router simply cannot keep up with the offered load.

### Q4. How does router queue capacity influence:

**A. Average Packet Delay?**
- **Larger queue capacity (`B`):** Allows more packets to accumulate during congestion instead of being dropped, so accepted packets experience **higher average queueing delay**, especially as ρ approaches or exceeds 1.
- **Smaller queue capacity:** Limits how much backlog can build up, which **caps the maximum queueing delay** at a lower value — but at the cost of more drops.

**B. Packet-Drop Probability?**
- **Larger queue capacity:** Better absorbs short-term traffic bursts, resulting in a **lower packet-drop probability** for the same load.
- **Smaller queue capacity:** Fills up quickly during bursts, leading to a **higher packet-drop probability**.

In short, queue capacity trades off delay against loss: a bigger buffer reduces drops but increases delay for the packets that do get through, while a smaller buffer keeps delay low but drops more packets under load — a manifestation of the classic **bufferbloat trade-off**.

---

## Submission Contents

- `simulation.c` — C source code
- `results.csv` — Generated simulation output
- Spreadsheet with plots of ρ vs. Queueing Delay, End-to-End Delay, and Drop Probability
