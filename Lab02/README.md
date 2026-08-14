# Lab Assignment 2: Multi-Source Router Queueing Simulation & Wireshark Packet Analysis

---

## Table of Contents
1. [Overview & Objectives](#overview--objectives)
2. [Problem 1: Multi-Source Router Queueing Simulation](#problem-1-multi-source-router-queueing-simulation)
   - [Problem Statement](#problem-statement)
   - [Mathematical Model & Delay Formulations](#mathematical-model--delay-formulations)
   - [C Code Implementation & Logic Explanation](#c-code-implementation--logic-explanation)
   - [Compilation & Execution Instructions](#compilation--execution-instructions)
   - [Simulation Results & Data Tables](#simulation-results--data-tables)
   - [Plotting Guide & Graph Analysis](#plotting-guide--graph-analysis)
3. [Problem 2: Wireshark Packet Analysis](#problem-2-wireshark-packet-analysis)
   - [Problem Statement & Methodology](#problem-statement--methodology)
   - [Section 1: Exploring Wireshark Interface](#section-1-exploring-wireshark-interface)
   - [Section 2: ICMP Packet Analysis](#section-2-icmp-packet-analysis)
   - [Section 3: DNS Packet Analysis](#section-3-dns-packet-analysis)
   - [Section 4: HTTP Packet Analysis](#section-4-http-packet-analysis)
   - [Section 5: HTTPS Observation](#section-5-https-observation)
   - [Section 6: Display Filters Evaluation](#section-6-display-filters-evaluation)
4. [Key Insights & Conclusions](#key-insights--conclusions)
5. [Submission Deliverables Checklist](#submission-deliverables-checklist)

---

## Overview & Objectives

This lab consists of two major parts:

1. **Discrete-Event Network Simulation (C Program):** Extending a single-source FIFO queue model to simulate **N = 4** independent Poisson traffic sources multiplexing into a single bounded router queue. We evaluate queueing delay, end-to-end latency, and packet loss across different traffic intensities (ρ) and heterogeneous load distributions.

2. **Real-World Packet Inspection (Wireshark):** Capturing and dissecting actual network traffic at the Data Link, Network, Transport, and Application layers using Wireshark — analyzing ARP, ICMP, DNS, HTTP, and TLS/HTTPS.

---

## Problem 1: Multi-Source Router Queueing Simulation

### Problem Statement

Extend the C discrete-event queueing model from Lab 1 to support **N independent Poisson sources** transmitting packets to a shared destination through a common bounded FIFO router queue.

```
Source 1 (λ₁) ───┐
Source 2 (λ₂) ───┼──► Router (Bounded FIFO Queue, Q = 20) ──► Destination Link (R₂)
Source 3 (λ₃) ───┤
Source 4 (λ₄) ───┘
```

#### Experiments Required

**Part 1 — Equal Load across 4 Sources:**

`λ₁ = λ₂ = λ₃ = λ₄ = λ/4`  across  `ρ ∈ {0.2, 0.4, 0.6, 0.8, 0.9, 1.0, 1.2}`

**Part 2 — Heterogeneous Traffic at constant `λ = 1000 pkts/s`, `ρ = 0.80`:**

| Config | λ₁ | λ₂ | λ₃ | λ₄ |
|--------|-----|-----|-----|-----|
| 1 | 100 | 100 | 100 | 700 |
| 2 | 150 | 150 | 250 | 450 |
| 3 | 250 | 250 | 250 | 250 |
| 4 | 400 | 300 | 200 | 100 |

---

### Mathematical Model & Delay Formulations

| Parameter | Formula | Value |
|-----------|---------|-------|
| Aggregate Arrival Rate | `λ = Σ λᵢ` | — |
| Traffic Intensity | `ρ = (λ · L) / R₂` | — |
| Transmission Delay (Link 1) | `d_trans1 = L / R₁` | 0.8 ms |
| Propagation Delay (Link 1) | `d_prop1` | 1.0 ms |
| Router Processing Delay | `d_proc` | 0.1 ms |
| Transmission Delay (Link 2) | `d_trans2 = L / R₂` | 0.8 ms |
| Propagation Delay (Link 2) | `d_prop2` | 2.0 ms |
| **Total End-to-End Delay** | `D_e2e = d_trans1 + d_prop1 + d_proc + d_queue + d_trans2 + d_prop2` | **4.7 ms + d_queue** |

> **Network Parameters:** L = 1000 Bytes (8000 bits), R₁ = R₂ = 10 Mbps, Queue Capacity = 20 packets.

---

### C Code Implementation & Logic Explanation

**File:** [`lab2_queue_sim.c`](lab2_queue_sim.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PACKET_LENGTH_BYTES     1000          // L = 1000 Bytes (8000 bits)
#define LINK1_BANDWIDTH_BPS     10000000.0    // R1 = 10 Mbps
#define LINK2_BANDWIDTH_BPS     10000000.0    // R2 = 10 Mbps
#define LINK1_PROP_DELAY_SEC    0.001         // 1 ms
#define LINK2_PROP_DELAY_SEC    0.002         // 2 ms
#define ROUTER_PROC_DELAY_SEC   0.0001        // 0.1 ms
#define QUEUE_CAPACITY          20            // Buffer limit
#define TOTAL_PACKETS           100000        // Packets per experiment
#define RANDOM_SEED             42            // Seed for reproducibility
```

#### Core Logic Breakdown

| Mechanism | Description |
|-----------|-------------|
| **Event Scheduling** (`next_arr_times`) | Maintains independent generation timelines for all 4 sources. Selects `min(t_next,i)`, executes that arrival, and advances only that source's clock. |
| **Bounded FIFO Buffer** | Circular array of size `QUEUE_CAPACITY = 20`. Drains packets whose transmission has started; drops incoming packets via tail-drop when buffer is full. |
| **Deterministic Seeding** | Fixed seed (`42`) ensures benchmark reproducibility across simulation runs. |

---

### Compilation & Execution Instructions

```bash
# Compile with GCC and link the math library
gcc -O2 -o lab2_queue_sim lab2_queue_sim.c -lm

# Execute the simulation
./lab2_queue_sim

# Output files generated:
#   1. lab2_part1_results.csv
#   2. lab2_part2_results.csv
```

---

### Simulation Results & Data Tables

#### Table 1: Part 1 — Equal Load Distribution ([`lab2_part1_results.csv`](lab2_part1_results.csv))

| ρ | λ_agg (pkts/s) | Generated | Delivered | Dropped | P_drop | Avg Queue Delay (s) | Avg E2E Delay (s) | Max Queue |
|-----|----------------|-----------|-----------|---------|--------|---------------------|-------------------|-----------|
| 0.20 | 250.00 | 100,000 | 100,000 | 0 | 0.000000 | 0.000102 | 0.004802 | 4 |
| 0.40 | 500.00 | 100,000 | 100,000 | 0 | 0.000000 | 0.000267 | 0.004967 | 7 |
| 0.60 | 750.00 | 100,000 | 100,000 | 0 | 0.000000 | 0.000606 | 0.005306 | 12 |
| 0.80 | 1000.00 | 100,000 | 100,000 | 0 | 0.000000 | 0.001564 | 0.006264 | 18 |
| 0.90 | 1125.00 | 100,000 | 99,832 | 168 | 0.001680 | 0.003368 | 0.008068 | 20 |
| 1.00 | 1250.00 | 100,000 | 97,618 | 2,382 | 0.023820 | 0.007860 | 0.012560 | 20 |
| 1.20 | 1500.00 | 100,000 | 83,758 | 16,242 | 0.162420 | 0.013675 | 0.018375 | 20 |

#### Table 2: Part 2 — Heterogeneous Load at ρ = 0.80 ([`lab2_part2_results.csv`](lab2_part2_results.csv))

| Distribution (λ₁, λ₂, λ₃, λ₄) | ρ | λ_agg | Delivered | Dropped | P_drop | Avg Queue Delay (s) | Avg E2E Delay (s) | Max Q |
|--------------------------------|-----|-------|-----------|---------|--------|---------------------|-------------------|-------|
| (100, 100, 100, 700) | 0.80 | 1000 | 99,998 | 2 | 0.000020 | 0.001586 | 0.006286 | 20 |
| (150, 150, 250, 450) | 0.80 | 1000 | 99,997 | 3 | 0.000030 | 0.001607 | 0.006307 | 20 |
| (250, 250, 250, 250) | 0.80 | 1000 | 100,000 | 0 | 0.000000 | 0.001572 | 0.006272 | 19 |
| (400, 300, 200, 100) | 0.80 | 1000 | 99,997 | 3 | 0.000030 | 0.001591 | 0.006291 | 20 |

---

### Plotting Guide & Graph Analysis

Using `lab2_part1_results.csv` in Google Sheets or Excel:

**Graph 1: ρ vs. Average Queueing Delay**
- X-axis: Column A (ρ ∈ [0.2, 1.2]), Y-axis: Column G (Avg Queueing Delay)
- 📈 **Analysis:** Stays near-zero (< 0.6 ms) until ρ ≈ 0.6, after which it grows exponentially as ρ → 1.0. Beyond ρ = 1.0, the curve plateaus — finite queue capacity bounds the maximum wait time.

**Graph 2: ρ vs. Average End-to-End Delay**
- X-axis: Column A (ρ), Y-axis: Column H (Avg End-to-End Delay)
- 📈 **Analysis:** Same curve shape as Graph 1, shifted upward by the constant physical baseline **D_base = 4.7 ms** (d_trans1 + d_prop1 + d_proc + d_trans2 + d_prop2).

**Graph 3: ρ vs. Packet-Drop Probability**
- X-axis: Column A (ρ), Y-axis: Column F (Packet-Drop Probability)
- 📈 **Analysis:** P_drop = 0 for ρ ≤ 0.80. As ρ exceeds 1.0, queue saturates and drops grow rapidly, approaching ~16.24% at ρ = 1.2.

| Graph | Preview |
|-------|---------|
| ρ vs. Avg Queueing Delay | ![Queue Delay](Traffic%20Intensity%20(rho)%20vs%20Avg%20Queueing%20Delay%20(s).png) |
| ρ vs. Avg End-to-End Delay | ![E2E Delay](Traffic%20Intensity%20(rho)%20vs%20Avg%20End-to-End%20Delay%20(s).png) |
| ρ vs. Packet-Drop Probability | ![Drop Prob](Traffic%20Intensity%20(rho)%20vs%20Packet-Drop%20Probability.png) |

---

## Problem 2: Wireshark Packet Analysis

### Problem Statement & Methodology

Analyze real-world network communications using Wireshark to understand Network and Application layer protocols.

- **Trace File:** [`Lab2_2403CS01.pcapng`](Lab2_2403CS01.pcapng)
- **Total Captured Packets:** 3,560 packets

**Wireshark Window Layout:**
```
┌─────────────────────────────────────────────────────────────┐
│ 1. Packet List Pane   (No., Time, Src, Dst, Protocol)       │
├─────────────────────────────────────────────────────────────┤
│ 2. Packet Details Pane (Layer breakdown: L2, L3, L4, App)   │
├─────────────────────────────────────────────────────────────┤
│ 3. Packet Bytes Pane  (Raw Hexadecimal & ASCII mapping)     │
└─────────────────────────────────────────────────────────────┘
```

---

### Section 1: Exploring Wireshark Interface

**Q: How many packets were captured?**
> **3,560 packets** (confirmed in the Wireshark status bar).

**Q: Protocols observed in the Protocol column:**

| Protocol | Description |
|----------|-------------|
| ARP | Address Resolution Protocol — MAC lookup broadcasts |
| MDNS | Multicast DNS — device discovery on 224.0.0.251 |
| DNS | Domain Name System — lookups to resolver 172.16.1.82 |
| TCP | Transmission Control Protocol — port 443 sessions |
| UDP | User Datagram Protocol — various streams |
| TLSv1.2 / TLSv1.3 | Encrypted transport security handshakes |
| SSDP | Simple Service Discovery Protocol — M-SEARCH broadcasts |

**Q: Most frequent protocol?**
> ARP and MDNS dominate due to local LAN device discovery broadcasts. Among unicast application traffic, **TLSv1.2 / TCP** is most frequent.

**Q: Purpose of the three Wireshark panes:**

| Pane | Purpose |
|------|---------|
| **Top** (Packet List) | Summarized table: packet number, timestamp, src/dst IP, protocol, and high-level info |
| **Middle** (Packet Details) | Hierarchical, expandable protocol stack: Ethernet → IP → TCP/UDP → Application Layer |
| **Bottom** (Packet Bytes) | Raw byte content in hexadecimal (left) and readable ASCII (right) |

---

### Section 2: ICMP Packet Analysis

**Command:** `ping google.com` | **Filter:** `icmp`

| Field | Echo Request | Echo Reply |
|-------|-------------|------------|
| Source IP | 172.9.13.60 (Local Host) | 142.250.x.x (Google) |
| Destination IP | 142.250.x.x (Google) | 172.9.13.60 (Local Host) |
| ICMP Data Payload | 32 bytes | 32 bytes |
| Total Frame Size | 74 bytes | 74 bytes |
| TTL | 128 (Windows default) | 57–117 (decremented per hop) |

**Encapsulation:** ICMP is encapsulated directly in **IPv4** (Protocol Number = 1).

**RTT Comparison across websites:**

| Website | Approx RTT | Reason |
|---------|-----------|--------|
| google.com | ~20 ms | Local CDN / edge cache |
| bbc.com | ~125 ms | Trans-regional European routing |
| stanford.edu | ~215 ms | Trans-oceanic submarine fiber to US West Coast |

> RTT varies due to **physical propagation delay**, **number of routing hops**, and **path congestion**.

---

### Section 3: DNS Packet Analysis

**Command:** `nslookup www.wikipedia.org` | **Filter:** `dns`

| Field | Value |
|-------|-------|
| Domain Queried | www.wikipedia.org (Type A) |
| IP Returned | 208.80.154.224 |
| Responding DNS Server | 172.16.1.82 |
| Query → Response Time | ~22.8 ms (Frame 823 → Frame 831) |

**Why DNS is required:** Humans use mnemonic domain names, but routers require 32-bit numerical IP addresses to route datagrams across the Internet. DNS is the distributed translation service that maps one to the other.

---

### Section 4: HTTP Packet Analysis

**Website:** `http://neverssl.com` | **Filter:** `http`

| Field | Value |
|-------|-------|
| Resource Requested | `/` (Root homepage) |
| Host | neverssl.com |
| HTTP Version | HTTP/1.1 |
| Response Status | 200 OK |
| Content-Type | text/html; charset=utf-8 |

**Can you view the contents?**
> ✅ **Yes.** HTTP operates over unencrypted TCP port 80. All headers, HTML, and cookies are transmitted in **cleartext** and are fully readable in Wireshark.

---

### Section 5: HTTPS Observation

**Website:** `https://www.google.com` | **Filter:** `tls` or `tcp.port == 443`

**Protocol:** TLS (TLSv1.3) over TCP port 443.

**Can you view the webpage contents?**
> ❌ **No.** Application data is encrypted with symmetric session keys negotiated during the TLS handshake — appearing as opaque **Application Data**.

**What remains visible (even in HTTPS):**

| Visible | Not Visible |
|---------|------------|
| Source/Destination IPs | HTTP headers |
| Port numbers | HTML content |
| Packet sizes, TCP seq/ack numbers | Cookies, query params |
| SNI (domain name) in Client Hello | Response body |

**Why HTTPS is more secure than HTTP:**

| Property | Mechanism |
|----------|-----------|
| **Confidentiality** | End-to-end symmetric encryption prevents eavesdropping |
| **Integrity** | Message Authentication Codes (MACs) prevent tampering |
| **Authentication** | PKI digital certificates prevent Man-in-the-Middle attacks |

---

### Section 6: Display Filters Evaluation

**Trace:** `Lab2_2403CS01.pcapng` (Total = 3,560 packets)

| Display Filter | Packets Displayed | % of Total | Purpose |
|----------------|-------------------|------------|---------|
| `icmp` | 12 | 0.34% | Ping echo requests and replies |
| `dns` | 38 | 1.07% | DNS domain resolution transactions |
| `http` | 16 | 0.45% | Plaintext HTTP GET requests and responses |
| `ip` | 3,142 | 88.26% | All IPv4 packets (TCP, UDP, ICMP, DNS, TLS) |

**Q: Which filter displayed the largest number of packets?**
> **`ip`** — Almost all internet communications (TCP, UDP, ICMP) are encapsulated inside IPv4 packets; only Layer 2 non-IP broadcasts (like ARP) are excluded.

**Q: Why are display filters useful?**
> Real networks carry thousands of irrelevant broadcast, multicast, and background sync packets per minute. Display filters allow engineers to **instantly isolate** specific connections, protocols, or IP pairs without discarding captured data or restarting the capture.

---

## Key Insights & Conclusions

1. **Superposition of Poisson Processes:** Multiplexing N independent Poisson sources with rates λ₁, …, λₙ produces an aggregate arrival process that behaves identically to a **single Poisson process** of rate `λ = Σλᵢ`.

2. **FIFO Queue Invariance to Source Distribution:** In an unprioritized FIFO queue, queueing delay and packet drop probability depend **strictly on aggregate load λ**, and are independent of how that load is partitioned across individual sources.

3. **Protocol Layering Visibility:** Wireshark provides transparent insight into OSI/TCP-IP layering — network metadata (IP headers, TCP ports) remains accessible for routing while application payloads can be fully protected using TLS encryption.

---

## Submission Deliverables Checklist

- [x] **Source Code:** [`lab2_queue_sim.c`](lab2_queue_sim.c)
- [x] **Data Files:** [`lab2_part1_results.csv`](lab2_part1_results.csv) and [`lab2_part2_results.csv`](lab2_part2_results.csv)
- [x] **Graphs:**
  - Graph 1: ρ vs. Average Queueing Delay
  - Graph 2: ρ vs. Average End-to-End Delay
  - Graph 3: ρ vs. Packet-Drop Probability
- [x] **Wireshark Capture:** [`Lab2_2403CS01.pcapng`](Lab2_2403CS01.pcapng)
- [x] **Lab Report PDF:** [`Lab2-QueueSim-Wireshark.pdf`](Lab2-QueueSim-Wireshark.pdf)

---

*CS3102 — Computer Networks Lab | Lab Assignment 2*
