# Lab Assignment 3: TCP Client-Server Statistics Application

## Overview

This lab implements an iterative TCP client-server application in C using the POSIX Socket API. The client collects a dataset of $N$ integers from the user and transmits them over a reliable byte stream to the server. The server computes basic descriptive statistics (minimum, maximum, sum, and average) and returns the aggregated results in a structured payload to the client for display.

---

## Theoretical Foundations

### 1. Connection Socket Pair

Every active TCP session across the Internet is uniquely identified by an immutable 4-tuple known as the **Connection Socket Pair**:


$$\text{Connection Pair} = (\text{Client IP} : \text{Client Port},\ \text{Server IP} : \text{Server Port})$$

* **Addressing (Network Layer - IP):** Identifies the specific host on the network (e.g., `127.0.0.1` for loopback interface, `128.2.194.242`).
* **Multiplexing / Demultiplexing (Transport Layer - Port):** Ensures incoming packets are routed to the specific process/service descriptor (e.g., Port `80` for HTTP, `21` for FTP, `8080` for custom TCP servers).

---

### 2. Network Byte Order vs. Host Byte Order

Multi-byte integers (such as port numbers and IP addresses) are stored differently depending on the CPU architecture:

* **Host Byte Order (Little-Endian on x86/ARM):** The least significant byte is stored at the lowest memory address.
* **Network Byte Order (Big-Endian):** The most significant byte is stored at the lowest memory address (Internet Standard).

Conversion functions bridge this architecture gap:

* `htons()`: Host to Network Short (16-bit port numbers).
* `htonl()`: Host to Network Long (32-bit IPv4 addresses).
* `ntohs()`: Network to Host Short (incoming port numbers).
* `ntohl()`: Network to Host Long (incoming IP addresses).

---

### 3. Socket API Lifecycle & Primitive Mapping

```
      CLIENT (Active Endpoint)                           SERVER (Passive Endpoint)
    ┌──────────────────────────┐                       ┌───────────────────────────┐
    │  socket(AF_INET, ...)    │                       │   socket(AF_INET, ...)    │
    └────────────┬─────────────┘                       └─────────────┬─────────────┘
                 │                                                   │
                 │                                                   ▼
                 │                                     ┌───────────────────────────┐
                 │                                     │     bind(port 8080)       │
                 │                                     └─────────────┬─────────────┘
                 │                                                   │
                 │                                                   ▼
                 │                                     ┌───────────────────────────┐
                 │                                     │     listen(backlog=5)     │
                 │                                     └─────────────┬─────────────┘
                 │                                                   │
                 ▼                                                   ▼
    ┌──────────────────────────┐   TCP 3-Way Handshake ┌───────────────────────────┐
    │        connect()         ├──────────────────────►│    accept() [BLOCKS]      │
    └────────────┬─────────────┘                       └─────────────┬─────────────┘
                 │                                                   │ (Spawns client_fd)
                 ▼                                                   ▼
    ┌──────────────────────────┐    N & Array Payload  ┌───────────────────────────┐
    │    send() / write()      ├──────────────────────►│     recv() / read()       │
    └──────────────────────────┘                       └─────────────┬─────────────┘
                 ▲                                                   │ (Computes Stats)
                 │          Statistics Struct Return                 ▼
                 │◄────────────────────────────────────┤    send() / write()       │
    ┌────────────┴─────────────┐                       └─────────────┬─────────────┘
    │     recv() / read()      │                                     │
    └────────────┬─────────────┘                                     ▼
                 ▼                                     ┌───────────────────────────┐
    ┌──────────────────────────┐   TCP FIN Packet      │    recv() returns 0 (EOF) │
    │     close(sock_fd)       ├──────────────────────►├───────────────────────────┤
    └──────────────────────────┘                       │    close(client_fd)       │
                                                       └─────────────┬─────────────┘
                                                                     │
                                                       (Loops back to accept next client)

```

#### API Primitives

* **`socket(int domain, int type, int protocol)`**: Allocates a socket descriptor in the OS kernel. `AF_INET` sets IPv4, `SOCK_STREAM` specifies TCP, and `0` selects default IP protocol.
* **`bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)`**: Associates the socket descriptor with a specific local IP address and port number.
* **`listen(int sockfd, int backlog)`**: Converts an active socket into a passive listening socket. `backlog` defines the maximum length of pending unaccepted connections in the kernel queue.
* **`accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)`**: Extracts the first connection on the pending queue, creating a new connected socket descriptor (`new_socket` / `client_fd`) dedicated to that specific client session.
* **`connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)`**: Initiates the TCP three-way handshake (`SYN`, `SYN-ACK`, `ACK`) with the listening server.
* **`send()` / `write()` & `recv()` / `read()`**: Streams data bytes over the full-duplex TCP channel.
* **`close(int fd)`**: Terminates the session by sending a TCP `FIN` packet. Causes the peer's `recv()` to return `0` (indicating **EOF**).

---

### 4. TCP Framing & Serialization Strategy

TCP is a byte-stream protocol that guarantees in-order, reliable byte delivery, but **does not preserve message boundaries**.

To prevent partial reads or stream corruption:

1. **Length-Prefixed Framing:** The client first sends the integer count $N$ (`sizeof(int)` bytes).
2. **Fixed-Size Data Chunking:** The client immediately sends the array payload (`N * sizeof(int)` bytes).
3. **Structured Response Packaging:** The server aggregates all computed metrics into a single contiguous binary struct (`struct Statistics`) and transfers it in a single transmission:
```c
struct Statistics {
    int min;
    int max;
    int sum;
    float avg;
};
```

---

### 5. Architectural Considerations: Iterative vs. Concurrent Servers

* **Iterative Server (Implemented Model):** Handles one client to completion within a synchronous loop (`while(1)`). If a connected client blocks (e.g., waiting for input or ungraceful pauses), the server cannot process any incoming client in the backlog.
* **Concurrent Server (Production Model):** Utilizes `fork()`, worker threads (`pthread_create`), or non-blocking I/O multiplexers (`select`/`poll`/`epoll`) so the main listening socket descriptor never blocks waiting on client I/O.

---

## Directory Structure

```text
├── server.c      # Iterative TCP server implementation
├── client.c      # TCP client interface
└── README.md     # Project technical documentation
```

---

## Compilation and Execution

### Compilation (Linux / WSL / POSIX)

```bash
gcc -Wall server.c -o server
gcc -Wall client.c -o client
```

### Running the Application

**Step 1: Start the Server (Terminal 1)**

```bash
./server
```

*Console output:*

```text
Server Listening on Port 8080...
```

**Step 2: Execute the Client (Terminal 2)**

```bash
./client
```

*Interactive prompts and execution:*

```text
Enter Server IP address: 127.0.0.1
Enter Server Port number: 8080
Connected to server.

Enter number of integers N: 5
Enter 5 values: 12 7 25 4 18

Minimum = 4
Maximum = 25
Sum = 66
Average = 13.20
```

**Step 3: Verify Server Logs (Terminal 1)**

```text
Connected to client: IP = 127.0.0.1, Port = 48738
Integers received from client: 12 7 25 4 18 
Computed: Minimum = 4, Maximum = 25, Sum = 66, Average = 13.20
Client disconnected.
```
