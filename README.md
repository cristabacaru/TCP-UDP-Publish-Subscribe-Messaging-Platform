# TCP/UDP Publish–Subscribe Messaging Platform

## Overview
This project implements a **real-time messaging platform** based on the publish–subscribe model.  

The system allows UDP clients to publish messages to specific topics, while TCP clients can subscribe (with or without wildcards) to receive only the messages they are interested in.

---

## Features
- **Dual protocol support** – TCP for subscribers, UDP for publishers.
- **Topic-based filtering** – clients receive only relevant messages.
- **Wildcard subscriptions** – flexible matching using `*` (multi-level) and `+` (single-level).
- **Persistent subscriptions** – reconnecting with the same ID restores previous subscriptions.
- **Custom application-layer protocol** – ensures proper message framing and parsing over TCP.
- **Multiplexed I/O** – handles multiple TCP and UDP clients simultaneously using `select()`.
- **Defensive programming** – handles invalid input, client disconnects, and malformed packets gracefully.

---

## Architecture
1. **Server**  
   - Listens on both TCP and UDP sockets.
   - Acts as a message broker: receives messages from UDP publishers and forwards them to subscribed TCP clients.
   - Tracks subscriptions and applies wildcard filtering.

2. **TCP Subscriber Client**  
   - Connects to the server and subscribes/unsubscribes to topics.
   - Displays incoming messages in a human-readable format:
     ```
     <publisher_IP>:<publisher_port> - <topic> - <type> - <value>
     ```

3. **UDP Publisher Client**  
   - Provided for testing (can also be custom-built).
   - Sends messages to the server in the predefined binary format.

---

## Example Subscription with Wildcards
- `subscribe UPB/precis/*/value` → matches:
  - `UPB/precis/temperature/value`
  - `UPB/precis/room1/temperature/value`
- `subscribe a/+/b` → matches:
  - `a/room1/b`
  - `a/room2/b`

---

## Technologies Used
- **Language:** C
- **Networking:** TCP, UDP, sockets API, `select()` multiplexing
- **Protocol Design:** custom binary application-layer protocol
- **Tools:** GCC, Make, Linux, Bash

---

## How to Build
```bash
make
```

## How to Run
**Server:**
```bash
./server <PORT>
```

**Subscriber:**
```bash
./subscriber <ID_CLIENT> <SERVER_IP> <SERVER_PORT>
```

---

## Example Output
```text
New client sub1 connected from 127.0.0.1:54321.
Subscribed to topic UPB/precis/temperature
1.2.3.4:4573 - UPB/precis/temperature - SHORT_REAL - 23.5
```

---

## Skills Demonstrated
- Network programming in C (TCP & UDP)
- Concurrency with multiplexed I/O
- Application-layer protocol design
- Pattern matching with wildcards
- Robust error handling and resource management

---
