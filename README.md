# Embedded Packet Parser & Stream Simulator

A lightweight, bare-metal-compliant packet parser written in C. This project simulates how an embedded system processes a continuous stream of raw bytes from a serial interface (like UART, SPI, or Ethernet) and reconstructs them into structured data frames.

## Key Features
- **Circular Ring Buffer:** Implements a thread-safe-ready Ring Buffer (`FIFO`) to buffer asynchronous data coming from high-speed hardware interfaces, preventing data loss.
- **State Machine Architecture:** Implements an efficient finite state machine (FSM) to parse incoming bytes on-the-fly without blocking the system.
- **Zero Dynamic Memory Allocation:** Built purely with static memory structures (`no malloc`), ensuring high reliability and predictability for critical real-time systems.
- **Data Integrity Verification:** Validates packets using an XOR-based checksum (CRC) mechanism to filter out corrupt data.

## Packet Frame Structure
Each frame follows a strict telecommunication-inspired structure:
| Start of Frame (SOF) | Command ID | Length (N) | Payload (0-8 bytes) | Checksum (XOR) |
|---|---|---|---|---|
| 1 Byte (0xAA) | 1 Byte | 1 Byte | N Bytes | 1 Byte |

## How to Run
Compile using any standard C compiler (e.g., GCC):
```bash
gcc main.c -o parser_sim
./parser_sim