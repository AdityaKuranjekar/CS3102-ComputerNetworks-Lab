# CS3102 – Computer Networks Lab

Lab exercises and simulations for the **Computer Networks (CS3102)** course.

Each lab is self-contained in its own folder with source code, generated results, and a detailed `README.md` explaining the problem statement, theory, and analysis.

---

## 📂 Lab Index

| Lab | Title | Topics Covered |
|-----|-------|----------------|
| [Lab 1](./Lab01) | Simulation of Router Queueing Delay | Single-source Poisson arrivals, bounded FIFO queue (M/M/1/B), queueing delay, end-to-end delay, packet-loss vs. traffic intensity (ρ) |
| [Lab 2](./Lab02) | Multi-Source Queueing Simulation & Wireshark Packet Analysis | Multi-source (N=4) Poisson traffic multiplexing, heterogeneous load distributions, queueing invariance; Wireshark — ICMP, DNS, HTTP, HTTPS/TLS analysis |

---

## 🗂️ Repository Structure

```
.
├── Lab01/
│   ├── simulation.c
│   ├── results.csv
│   ├── Traffic Intensity vs. Average Queueing Delay.png
│   ├── Traffic Intensity vs. Average End-to-End Delay.png
│   ├── Traffic Intensity vs. Packet-Drop Probability.png
│   ├── Lab1-QueueSim.pdf
│   └── README.md
│
├── Lab02/
│   ├── lab2_queue_sim.c
│   ├── lab2_part1_results.csv
│   ├── lab2_part2_results.csv
│   ├── Traffic Intensity (rho) vs Avg Queueing Delay (s).png
│   ├── Traffic Intensity (rho) vs Avg End-to-End Delay (s).png
│   ├── Traffic Intensity (rho) vs Packet-Drop Probability.png
│   ├── Lab2_2403CS01.pcapng
│   ├── Lab2-QueueSim-Wireshark.pdf
│   └── README.md
│
└── README.md
```

---

## ⚙️ General Build Instructions

Most labs are implemented in **C**. Unless stated otherwise in a lab's own README:

```bash
# Compile
gcc <file>.c -o <file> -lm

# Run
./<file>
```

---

## 👤 Author

**Aditya Kailash Kuranjekar**
Roll No: **2403CS01**
5-Year Integrated B.Tech (CSE), IIT Patna + MBA in Digital Business and AI, IIM Bodh Gaya

*Computer Networks (CS3102) — Lab Submissions*
