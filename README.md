#MULTI THREADED TRAIN SCHEDULER( MTS)
## Implemntation status
# Train Scheduler – Multithreaded Simulation (CSC361)

## Overview
This project is a **multithreaded train scheduling simulation** written in C.  
It models trains arriving at a station, loading, and crossing a shared main track while enforcing
**priority rules, fairness, and starvation prevention**.

The program was developed as part of **CSC361 (Operating Systems)** and demonstrates
practical use of **threads, mutexes, and condition variables**.

---

## Features
- Multi-threaded train simulation using **POSIX threads**
- Priority-based scheduling (high-priority trains cross first)
- Direction-based fairness to avoid starvation
- Dispatcher thread to control access to the main track
- Proper synchronization using mutexes and condition variables
- Deterministic and reproducible execution

---

## How It Works
- Each train is represented by a **separate thread**
- Trains load for a specified time before becoming ready
- A **dispatcher thread** selects the next train to cross
- Only one train may occupy the main track at a time
- After two consecutive trains in the same direction, an opposite-direction train is prioritized (if available)

---

## Input
The program takes **one command-line argument**:

### Input File Format
The input file specifies trains with:
- Direction (East / West)
- Priority (High / Low)
- Loading time
- Crossing time

An example input file (`input.txtx`) is included.

---

## Output
- Simulation output is written to `output.txtx`
- If the program is run multiple times, the output file is overwritten
- Output format follows the assignment specification

---

## Compilation & Execution
```bash
make
./mts input.txt

