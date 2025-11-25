# TCP-like Serial Communication Protocol in C

This project implements a **TCP-inspired reliable data transfer protocol** over a **serial link**, built entirely from scratch in **C**. The goal was to design and implement a full link-layer and application-layer communication system capable of transferring binary files (such as images) between two computers.

The project **passed all tests**, including robustness trials with **noise injection and cable disconnections**, even when simulating interference using a *magnet on the cables*.

---

## 📄 Overview

This project simulates a custom, reliable, TCP-like protocol over a serial connection. It includes:

* Error detection and handling
* Framing and byte stuffing
* Sequence and acknowledgment mechanisms
* Timeouts and retransmissions
* Support for binary file transfer
* Resilience to noise and signal interference

---

## 📁 Project Structure

```
bin/       -> Compiled binaries
src/       -> Source code for link-layer and application-layer (edit here)
include/   -> Header files (DO NOT modify)
cable/     -> Virtual cable program (DO NOT modify)
main.c     -> Entry point (DO NOT modify)
Makefile   -> Build and run automation
penguin.gif -> Sample file used for testing
```

---

## 🛠️ Building the Project

Use the provided Makefile:

```
make
```

This compiles both the protocol implementation and the virtual cable program.

---

## Running the Virtual Cable 

The virtual cable simulates a serial link and supports:

* Normal operation
* Disconnection
* Noise injection

Run it using:

```
./bin/cable_app
```

Or via Makefile:

```
make run_cable
```

---

## 🔄 Basic Transmission Test (No Noise)

### **1. Start the Receiver**

```
./bin/main /dev/ttyS11 rx penguin-received.gif
```

Or:

```
make run_rx
```

### **2. Start the Transmitter**

```
./bin/main /dev/ttyS10 tx penguin.gif
```

Or:

```
make run_tx
```

### **3. Verify File Integrity**

```
diff -s penguin.gif penguin-received.gif
```

Or:

```
make check_files
```

If everything is correct, the files should match exactly.

---

##  Noise and Disconnection Tests

The protocol must remain reliable even when the virtual cable introduces errors.

### **Steps:**

1. Run the transmitter and receiver as before.
2. Switch to the cable emulator console and press:

   * `0` → Unplug cable
   * `1` → Normal mode
   * `2` → Add noise
3. Observe the protocol handling:

   * Retransmissions
   * Re-synchronization
   * Error detection/recovery
4. Check file integrity again using `diff`.

This validates the robustness of the protocol under realistic conditions, including **electromagnetic interference** (like passing a magnet near the cable).

---

## Example

The provided `penguin.gif` is ideal for testing because binary files highlight protocol errors immediately. Any corruption results in a mismatched output.

---

## 👥 Authors

* **Manuel Amorim**
* **Diogo Costa**
