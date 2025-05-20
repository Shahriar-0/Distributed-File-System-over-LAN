# Distributed File System over Local Area Network (DFS-over-LAN)

## Report

### Client

Here’s a friendly, high‑level walkthrough of what this `Client` class does without drowning in details:

#### 1. Connection Setup

* **Constructor / Destructor**

  * Opens a `QTcpSocket` to a master server and hooks up slots for connect/disconnect, incoming data, and errors (for master).
  * Binds a `QUdpSocket` on a random port so it can both send and receive UDP datagrams (its for chunks).
  * Cleans up sockets on destruction.

* **sendCommand()**

  * Formats commands like `ALLOCATE_CHUNKS <fileId> <size>` or `LOOKUP_FILE <fileId>` and sends them via TCP.
  * On `ALLOCATE_CHUNKS`, it also opens the local file, computes its size and number of chunks, and remembers state for uploading.
  * On `LOOKUP_FILE`, it prepares an output file and state for downloading.

#### 2. Handling Master Server Replies (TCP)

* **onReadyRead()**

  * Reads lines like `OK Allocated 5 <chunkId1> <ip1> <port1> …` or `FILE_METADATA <chunkId> <ip> <port> …` from chunks.
  * Parses out the number of chunks and builds a list of `ChunkServerInfo` structs.
  * Kicks off either `uploadFileToChunk()` or `downloadFileFromChunk()` once it has the info.

#### 3. Upload Flow (UDP)

* **uploadFileToChunk()**

  * Seeks to the current chunk’s offset in the local file, reads up to 8 KB, and constructs a packet:

    ```txt
    STORE <chunkId> <dataLength>\n
    <binary data…>
    ```

  * Sends it as a UDP datagram to the chunk server’s IP/port (optionally pinging first for NAT hole‑punching).
  * Emits a log signal so a UI can show “Sent chunk123 → 192.0.2.5:5001”.

* **onUdpReadyRead()** (for ACKs)

  * Reads incoming UDP datagrams, splits header and payload.
  * If header starts with `ACK`, reads the next server info (in case chunks hop between servers), notes corruption flag, emits `chunkAckReceived`, updates a progress signal, and recursively uploads the next chunk.
  * When all chunks are ACKed, emits `uploadFinished`.

#### 4. Download Flow (UDP)

* **downloadFileFromChunk()**

  * Sends a simple `RETRIEVE <chunkId>\n` message to the chunk server.
* **onUdpReadyRead()** (for DATA)

  * For headers beginning with `DATA`, reads the chunk ID, corruption flag, and length, writes the payload to the output file, emits progress and chunk‑data signals, and requests the next chunk until done.
  * Closes the file, computes the final size/path, and emits `downloadFinished`.

#### 5. Signals & UI Integration

Throughout, the class emits Qt signals such as: (mostly are connected to qml)

* **connectionStateChanged()** when TCP connects/disconnects.
* **errorOccurred()** on any socket or file error.
* **uploadProgress()** and **downloadProgress()** for UI progress bars.
* **logReceived()** for simple textual logs.
* **chunkAckReceived** and **chunkDataReceived** for low‑level events.
* **uploadFinished()** and **downloadFinished()** when complete.

---

### Master

The `MasterServer` listens for TCP connections from `Client` instances, keeps track of every file’s chunk layout, and tells clients which chunk servers to contact. It:

1. Builds a binary‑tree view of chunk servers and precomputes a DFS traversal order.
2. On each new client TCP connection, parses simple text commands (`ALLOCATE_CHUNKS`, `LOOKUP_FILE`, etc.).
3. For uploads, allocates chunk IDs and assigns them to chunk servers in a rotating DFS order.
4. For downloads, looks up stored metadata and returns the same list of `<chunkId> <ip> <port>` back to the client.

#### 1. Networking & Signal Handling

* **Listening**

  * `startListening(address, port)` wraps `listen()` and logs success or failure.

* **New Connections**

  * On `newConnection`, grabs each `QTcpSocket*`, hooks up `readyRead` and `disconnected` signals, and tracks them in `m_clients`.

#### 2. Request Dispatch & Commands

* **onReadyRead()**

  * Reads lines like `ALLOCATE_CHUNKS 100KB.txt 123456` or `LOOKUP_FILE 100KB.txt` and calls `handleRequest()`.

* **handleRequest()**

  * Splits the incoming line on spaces and dispatches:
    * `ALLOCATE_CHUNKS <fileId> <size>` → `allocateChunks()`.
    * `LOOKUP_FILE <fileId>` → `lookupFile()`.
    * Unknown → `ERROR Unknown command…`.

#### 3. Chunk Allocation & Lookup

* **allocateChunks()**

  * Computes `numChunks = ceil(size / CHUNK_SIZE)`.
  * Picks a random starting index in `dfsOrder` so uploads are load‑balanced across the tree.
  * For each chunk `i`, it:

    1. Creates an ID like `<fileId>_chunk_<i>`.
    2. Assigns it to the chunk server at port `dfsOrder[(startIdx + i) % dfsOrder.size()]`.
  * Stores this in `fileMetadata[fileId]`.
  * Sends back `OK Allocated <numChunks> <chunkId1> <ip1> <port1> …\n`, mirroring what the client’s `onReadyRead()` expects.

* **lookupFile()**

  * Finds the existing `FileMetadata` for `fileId`, and emits `FILE_METADATA <fileId> <chunkId1> <ip1> <port1> …\n`.
  * The client then parses that and starts its UDP download requests.

#### 4. Interaction With the Client

* The client’s `sendCommand("ALLOCATE_CHUNKS", fileId, size)` leads to `MasterServer::allocateChunks()`, which decides where to store each piece.
* The client then uses that metadata to open UDP connections to the chunk servers we assigned here.
* On download, the client’s `sendCommand("LOOKUP_FILE", fileId)` triggers `lookupFile()`, letting the client pull exactly the same chunk list back down.

---

### Chunk Server

### 1. Role & Startup

Each `ChunkServer` listens on UDP port `5000 + serverId` and keeps its own `./CHUNK-<id>` folder. On `start()`, it binds that port and waits for two simple commands from any client:

* **STORE `<chunkId>` `<length>\n<data>`**
* **RETRIEVE `<chunkId>`**

### 2. Storing a Chunk

1. **Receive STORE**: Parses the header, grabs exactly `<length>` bytes of payload.
2. **Decode (and “noise” if enabled)**: A placeholder `decodeData()` flips bits or checks integrity.
3. **Write to Disk**: Saves as `CHUNK-<id>/<chunkId>.bin`.
4. **Acknowledge**: Sends back

   ```txt
   ACK <chunkId> <thisIp> <thisPort> <corruptedFlag>
   ```

   so the client knows it arrived (and where to ask next, if the master chained servers).

### 3. Retrieving a Chunk

1. **Receive RETRIEVE**: Reads the requested `<chunkId>`.
2. **Load & Encode**: Reads the `.bin` file, runs `encodeData()` (e.g. add parity or noise).
3. **Send DATA**: Replies with

   ```txt
   DATA <chunkId> <corruptedFlag> <length>\n<encodedData>
   ```

### 4. How It Fits with Previous Parts

* **MasterServer** told the client which chunk servers (ports) to use, based on a DFS‑shuffled list.
* **Client** reads those replies over TCP, then talks directly UDP to each `ChunkServer`.
* The simple STORE/ACK loop drives uploads; RETRIEVE/DATA drives downloads.
* Corruption flags and optional “noise” let us test reliability and re‑routing code in `Client`.

## Questions

### 1. How are noises and corruptions created during transmission resolved in the network?

There are multiple ways to resolve this problem, but the most common ones are:

* Error Detection and ARQ
Networks detect corrupted packets using error‐detecting codes like CRC (Cyclic Redundancy Check) appended to each frame, when a mismatch is detected, the receiver discards the packet and may request retransmission via ARQ protocols.
ARQ trades additional round trip delay for reliability. it is commonly used in TCP, which ensures in‑order, error‑free delivery over unreliable links by resending lost or corrupted segments until acknowledged.

* Forward Error Correction
FEC proactively adds redundant parity symbols to each block of data so that a limited number of bit errors can be corrected at the receiver without retransmission.
things like include Reed–Solomon, Low‑Density Parity‑Check, and concatenated schemes (e.g., Viterbi + Reed–Solomon) for handling burst and random errors.

### 2. Video streaming platforms, due to their nature of use, utilize connectionless UDP communication. How do they manage these corruptions?

Because UDP does not retransmit lost or late packets automatically, video applications must implement their own loss‐recovery and smoothing mechanisms at the application layer. Video streaming services often integrate FEC at the application or transport layer to correct a percentage of lost or corrupted packets without waiting for retransmissions, which could introduce unacceptable lag. There are other solutions like Adaptive Bitrate Streaming to dynamically adjusts video quality based on measured packet loss and bandwidth, reducing resolution or bitrate when network quality degrades to minimize the impact of packet corruption and loss.

### 3. How are noises and corruptions at the time of deployment (information stored somewhere but may become corrupted) resolved, and what are the advantages of distributed file systems in this regard?

they are addressed via a combination of on‐disk error‐detecting codes like CRC/checksums, FEC, and replication across nodes.
Advantages of Distributed File Systems are:

* Redundant Replication & Geodiversity: Multiple replicas are stored on separate nodes/racks/data‐centers, so a single device’s silent corruption can be masked by reading a clean replica.

* Erasure Coding Across Nodes: Compared to simple replication, erasure coding reduces storage overhead while tolerating multiple failures or corruptions, reconstructing missing data from surviving shards.

* Scalability & Fault Isolation: DFS architectures localize failures, a corrupt block on one server affects only that node’s portion of the data, which can be rebuilt independently without taking the entire system offline.

### 4. Considering the parameters you have stored for noise and denoising information, on average, what percentage of sent information will be irrecoverable? What percentage of errors are undetectable?

(calculation may vary since the project is still in development)

Given a per‐bit error rate *p* ≈ 10⁻⁶ and a 8 KB chunk, the expected bit‐flips per chunk is

$$
E[k] = Np \approx 8.4 × 10^{-3}
$$

With an RS code adding *m* parity symbols, able to correct up to 64 bit‐errors, the probability that a chunk has more than 64 errors is

$$
P_{\rm irrecoverable} = \sum_{k=65}^{N} \binom{N}{k} p^k (1-p)^{N-k},
$$

which for these parameters is *extremely* small, on the order of 10⁻¹² to 10⁻¹⁸, effectively negligible.

### 5. Do distributed file systems prevent all types of data corruption? Explain the use of replicas (Replica) in these systems

No system can guard against all corruption types, especially silent bit‐flips that evade detection or catastrophic metadata loss if checksums themselves become corrupt. For absolute protection, DFS rely on background scrubbing (periodic checksum verification) and off‑site backups to mitigate very rare, undetectable errors, other methods for more minor errors are explained above.
Replicas are used for many reasons:

* Redundancy for Fault Tolerance
* Load Balancing and Availability
* Consistency Models

as they are obvious, we won't go into the details.

### 6. How can bottlenecks be eliminated using distributed file systems?

* Horizontal Scalability: By distributing data and metadata workloads across many servers, DFS avoid single‐node throughput limits. Client requests are routed to multiple chunk servers, enabling parallel reads and writes that scale with cluster size

* Client‑Side Caching and Write‑Back Models: Systems like Coda use write‐back caching, allowing client applications to proceed without immediate server interaction for metadata updates, eliminating RPC bottlenecks and smoothing load peaks

* Dynamic Load Balancing: Metadata servers can monitor server load and redirect new requests to underutilized nodes. Background rebalancing jobs migrate chunks from hot nodes to colder ones, maintaining uniform distribution over time.

### 7. Compare Content Delivery Networks (CDNs) with distributed file systems. How can these networks reduce user access time to content, and what role do distributed file systems play in this?

CDNs are globally distributed caches that store copies of web assets close to end users (edge servers), reducing network latency and improving page load times while DFS are distributed origin storage systems that provide scalable, durable, and consistent storage across data centers. CDNs intercept user requests at the network edge, serving cached content directly from the nearest edge server, which can be orders of magnitude faster than fetching from a centralized origin server. While CDNs manage geo‑distributed caching autonomously, they rely on DFS to maintain integrity and consistency of origin content.
