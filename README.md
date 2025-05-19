# CN_CA_2

fuck cn

## Questions

### 1. How are noises and corruptions created during transmission resolved in the network?

There are multiple ways to resolve this problem, but the most common ones are:

- Error Detection and ARQ
Networks detect corrupted packets using error‐detecting codes like CRC (Cyclic Redundancy Check) appended to each frame, when a mismatch is detected, the receiver discards the packet and may request retransmission via ARQ protocols.
ARQ trades additional round trip delay for reliability. it is commonly used in TCP, which ensures in‑order, error‑free delivery over unreliable links by resending lost or corrupted segments until acknowledged.

- Forward Error Correction
FEC proactively adds redundant parity symbols to each block of data so that a limited number of bit errors can be corrected at the receiver without retransmission.
things like include Reed–Solomon, Low‑Density Parity‑Check, and concatenated schemes (e.g., Viterbi + Reed–Solomon) for handling burst and random errors.

### 2. Video streaming platforms, due to their nature of use, utilize connectionless UDP communication. How do they manage these corruptions?

Because UDP does not retransmit lost or late packets automatically, video applications must implement their own loss‐recovery and smoothing mechanisms at the application layer. Video streaming services often integrate FEC at the application or transport layer to correct a percentage of lost or corrupted packets without waiting for retransmissions, which could introduce unacceptable lag. There are other solutions like Adaptive Bitrate Streaming to dynamically adjusts video quality based on measured packet loss and bandwidth, reducing resolution or bitrate when network quality degrades to minimize the impact of packet corruption and loss.

### 3. How are noises and corruptions at the time of deployment (information stored somewhere but may become corrupted) resolved, and what are the advantages of distributed file systems in this regard?

they are addressed via a combination of on‐disk error‐detecting codes like CRC/checksums, FEC, and replication across nodes.
Advantages of Distributed File Systems are:

- Redundant Replication & Geodiversity: Multiple replicas are stored on separate nodes/racks/data‐centers, so a single device’s silent corruption can be masked by reading a clean replica.

- Erasure Coding Across Nodes: Compared to simple replication, erasure coding reduces storage overhead while tolerating multiple failures or corruptions, reconstructing missing data from surviving shards.

- Scalability & Fault Isolation: DFS architectures localize failures, a corrupt block on one server affects only that node’s portion of the data, which can be rebuilt independently without taking the entire system offline.

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

- Redundancy for Fault Tolerance
- Load Balancing and Availability
- Consistency Models

as they are obvious, we won't go into the details.

### 6. How can bottlenecks be eliminated using distributed file systems?

- Horizontal Scalability: By distributing data and metadata workloads across many servers, DFS avoid single‐node throughput limits. Client requests are routed to multiple chunk servers, enabling parallel reads and writes that scale with cluster size

- Client‑Side Caching and Write‑Back Models: Systems like Coda use write‐back caching, allowing client applications to proceed without immediate server interaction for metadata updates, eliminating RPC bottlenecks and smoothing load peaks

- Dynamic Load Balancing: Metadata servers can monitor server load and redirect new requests to underutilized nodes. Background rebalancing jobs migrate chunks from hot nodes to colder ones, maintaining uniform distribution over time.

### 7. Compare Content Delivery Networks (CDNs) with distributed file systems. How can these networks reduce user access time to content, and what role do distributed file systems play in this?

CDNs are globally distributed caches that store copies of web assets close to end users (edge servers), reducing network latency and improving page load times while DFS are distributed origin storage systems that provide scalable, durable, and consistent storage across data centers. CDNs intercept user requests at the network edge, serving cached content directly from the nearest edge server, which can be orders of magnitude faster than fetching from a centralized origin server. While CDNs manage geo‑distributed caching autonomously, they rely on DFS to maintain integrity and consistency of origin content.
