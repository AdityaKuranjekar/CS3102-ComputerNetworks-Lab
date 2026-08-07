# CS3102 – Computer Networks Lab

Lab exercises and simulations for the **Computer Networks (CS3102)** course.

Each lab is self-contained in its own folder, with source code, generated results, and a detailed `README.md` explaining the problem statement, theory, and analysis.

---

## 📂 Lab Index

| Lab | Title | Description |
|-----|-------|-------------|
| [Lab 1](./Lab1) | Simulation of Router Queueing Delay | Simulates a source–router–destination network with a bounded FIFO queue; analyzes queueing delay, end-to-end delay, and packet loss vs. traffic intensity (ρ) |

*(More labs will be added here as the course progresses.)*

---

## 🗂️ Repository Structure

```
.
├── Lab1/
│   ├── simulation.c
│   ├── results.csv
│   ├── results.xlsx
│   └── README.md
├── Lab2/
│   └── ...
└── README.md
```

---

## ⚙️ General Build Instructions

Most labs are implemented in **C**. Unless stated otherwise in a lab's own README:

```bash
gcc <file>.c -o <file> -lm
./<file>
```

---

## 👤 Author

**Aditya Kailash Kuranjekar**
Roll No: **2403CS01**
5-Year Integrated B.Tech (CSE), IIT Patna + MBA in Digital Business and AI, IIM Bodh Gaya

Computer Networks (CS3102) — Lab Submissions
