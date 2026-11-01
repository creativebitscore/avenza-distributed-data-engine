# Avenza distributed layer

The cluster client constructs the same static ring everywhere from ordered node arguments. For each node it hashes `node-id#virtual-index` into 128 positions using deterministic 64-bit FNV-1a. A key hashes clockwise to the first point, wrapping at the end. Adding a node moves approximately only its share; removing it sends affected ranges to successors. Tests verify removal restores the former mapping.

Avenza partitions data; nodes do not replicate it. Membership changes do not migrate existing keys. The router detects connection failure at request time and reports the unavailable owner. This is failure detection, not failover: rerouting without a replica would make a miss look authoritative and could split ownership.

Production evolution would require membership epochs, migration with dual reads/writes, replica placement on distinct failure domains, quorum/leader semantics, health hysteresis, timeouts, authentication, and observability. Those are explicitly outside this educational scope.
