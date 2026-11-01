# Avenza interview guide

## 60-second introduction

Avenza is my C++20 distributed in-memory data engine. Each TCP node combines a thread-safe hash-map cache with a true O(1)-average LRU list, per-key steady-clock TTLs, checksummed AOF and snapshots, monitoring counters, and ephemeral channel Pub/Sub. A separate client partitions keys across static nodes with deterministic consistent hashing and virtual nodes. I emphasized honest boundaries: it has failure detection but no replication or automatic failover. CMake, behavior tests, benchmarks, Docker Compose, and Windows/Linux automation make the design reproducible.

## Three-minute architecture and lifecycle

A client serializes a newline-delimited command; quoted tokens preserve spaces and escapes. The server accepts connections and starts a session thread. That thread bounds and frames input, parses it, validates command arity/types, then dispatches it. Cache operations lock one mutex that protects the map, LRU list, TTL metadata, and counters. GET locates the map entry, deletes it if expired, otherwise splices its list node to the MRU front. Mutations append a checksummed, length-prefixed AOF record after changing memory. SAVE enumerates live entries into a temporary snapshot, replaces the old file, and clears the AOF. Startup loads snapshot then AOF.

Pub/Sub maintains channel-to-callback maps behind a separate mutex. It copies callbacks before delivery, and each client has an output mutex so asynchronous messages cannot corrupt responses. The cluster router hashes a key with stable FNV-1a, finds the clockwise virtual point in an ordered map, and connects to that owner. If it cannot connect, it reports the failure because no replica exists.

## Core explanations

- **LRU:** `unordered_map` provides key lookup. `list` provides stable iterators and O(1) splice/erase. Every entry stores its list iterator; front is MRU, back is LRU.
- **Networking:** Winsock and POSIX adapters share the same blocking API. One thread serves each connection; newline framing supports partial/coalesced packets.
- **Thread safety:** component-level mutexes protect invariants. GET is a writer because it moves LRU state. Pub/Sub never invokes callbacks while holding its registry lock.
- **Persistence:** snapshot is the base and AOF is the ordered delta. Absolute persisted expiry deadlines preserve time elapsed while the process was stopped.
- **Consistent hashing:** 128 points per node improve distribution. Adding/removing a node changes only adjacent ranges, but Avenza does not migrate existing data.
- **Pub/Sub:** messages are transient fan-out events and are neither stored nor replayed; key/value state is persistent when persistence is enabled.

## Questions and answers

1. **Why both a map and list?** The map gives average O(1) lookup; the list gives O(1) recency reorder and oldest selection.
2. **Why store a list iterator in each entry?** It avoids an O(n) search when touching or deleting a key.
3. **What is GET complexity?** Expected O(1): one hash lookup, expiry comparison, and list splice; hash collision worst case can be O(n).
4. **How is LRU correctness tested?** Insert A and B, access A, insert C at capacity two, and assert B—not A—was evicted.
5. **Does SET reset TTL?** Yes. SET without EX makes the value permanent; SET with EX installs a new deadline.
6. **Why `steady_clock`?** It is monotonic, so NTP or user wall-clock changes do not alter live TTL behavior.
7. **How does TTL survive restart?** Persistence stores a Unix-millisecond deadline; recovery computes remaining wall time and creates a new steady deadline.
8. **What do TTL -1 and -2 mean?** -1 is a live permanent key; -2 is absent or expired.
9. **What happens with non-positive EXPIRE?** An existing key is deleted, matching the explicit implementation contract.
10. **Why lazy expiration?** It avoids a scheduler thread; access and scans remove expired entries, at the cost of temporary stale memory.
11. **Why not a shared mutex?** GET modifies recency and counters, so its normal path needs exclusive access anyway.
12. **What invariant does the cache lock protect?** Every map entry has exactly one valid list node, and list order agrees with stored iterators.
13. **Could callbacks deadlock Pub/Sub?** Delivery copies callbacks under lock and invokes them after unlocking.
14. **How are response/message writes protected?** Each client session has its own output mutex.
15. **How are TCP packet boundaries handled?** Bytes accumulate until LF; several commands or a partial command are handled correctly.
16. **Can values contain spaces?** Yes, quoted tokens preserve them; standard escapes preserve control characters and quotes.
17. **Is this Redis RESP compatible?** No. It is a documented educational quoted-line protocol.
18. **How is malformed input handled?** Parser/arity/integer errors return `ERR` without mutation; oversized buffers close the session.
19. **What happens when a client disconnects?** Its socket loop ends, all subscriptions are removed, and the socket closes.
20. **Why one thread per client?** It is portable and understandable; a bounded pool/event loop would scale further.
21. **What goes into AOF?** Only successful SET, DEL, EXPIRE, and FLUSHALL mutations.
22. **Why not log GET or PUBLISH?** Neither changes persistent key/value state; Pub/Sub is explicitly ephemeral.
23. **How is a torn AOF handled?** Length, bounds, magic, and checksum are validated; replay stops at the bad trailing record while retaining valid prefixes.
24. **What is snapshot recovery order?** Snapshot first, then AOF, because AOF represents mutations after the snapshot.
25. **Is SAVE fully crash-proof?** It uses same-directory temporary/rename/backup replacement, but omits file/directory fsync, so power-loss guarantees depend on the OS/filesystem.
26. **Why checksum records?** It detects partial writes and accidental corruption that sizes alone cannot.
27. **Does AOF fsync each command?** It flushes the C++ stream but does not issue OS fsync; this is a documented durability trade-off.
28. **Why not use `std::hash` for the ring?** Its result is not guaranteed stable across implementations; FNV-1a is deterministic.
29. **Why virtual nodes?** They distribute each physical node over many ring ranges, reducing imbalance with few nodes.
30. **What is ring lookup complexity?** O(log V) using `std::map::lower_bound`, where V is total virtual points.
31. **What moves when a node joins?** Only key ranges whose clockwise successor becomes one of the new node’s points—approximately its proportional share.
32. **Does Avenza migrate those keys?** No. Membership changes placement but the current educational client has no migration protocol.
33. **What happens if an owner fails?** Connection failure is reported; requests are not redirected because there is no replica.
34. **Why is that safer than naive fallback?** Fallback could return false misses and create divergent duplicate ownership.
35. **Is there automatic failover?** No; failure detection is synchronous connection feedback only.
36. **How does Pub/Sub differ from stored values?** Subscribers receive live events only; messages have no durability, TTL, recovery, or later retrieval.
37. **How are statistics maintained?** Counters are updated under the cache mutex and returned with live size/capacity.
38. **What does the benchmark measure?** In-process engine SET, GET, and 80/20 mixed throughput plus average/P50/P95/P99 latency with a fixed seed.
39. **Why avoid publishing benchmark numbers in README?** Numbers without machine/compiler/load context would be misleading; the executable produces actual local measurements.
40. **How is concurrency tested?** Eight threads repeatedly set/get distinct keys and verify completion and capacity invariants.
41. **How is restart recovery tested?** A test writes records and a snapshot, appends a later mutation, reconstructs cache/persistence objects, and checks state.
42. **What are the largest current bottlenecks?** The global cache mutex, one thread per connection, synchronous AOF stream flush, and per-command TCP round trip.
43. **What would you shard first?** Partition the key space into cache shards, each with its own map/list/mutex, accepting per-shard rather than global LRU.
44. **How would you add replication?** Define epochs and ordered mutation offsets, stream AOF records to replicas, acknowledge by a chosen durability policy, and promote through an external consensus/membership mechanism.
45. **What Redis features are absent?** Data structures beyond strings, RESP, replication, Sentinel/Cluster consensus, transactions, Lua, ACL/TLS, pipelining, active defrag, and mature persistence policies.

## Failure scenarios and trade-offs

Port conflicts fail startup; disconnects clean sessions; malformed commands return errors; a corrupt persistence tail preserves its valid prefix; expired persisted keys are skipped; an unavailable cluster owner is surfaced. Process/power failure can lose recently buffered OS writes. Node loss loses its unreplicated partition. A membership change changes routing without migrating old values. These are limitations to explain, not conceal.
