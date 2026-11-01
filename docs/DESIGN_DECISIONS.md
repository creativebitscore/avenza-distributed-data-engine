# Avenza design decisions

1. **One mutex, not shared mutex:** `GET` mutates LRU order and hit counters, so reads are not truly read-only. A mutex makes invariants obvious.
2. **Quoted line protocol:** easy to inspect with a terminal while still representing spaces/newlines. It is not Redis RESP.
3. **Steady-clock TTL:** immune to NTP/manual time jumps in-process. Persistence converts remaining duration to an absolute Unix deadline, then reconstructs a steady deadline after restart.
4. **Checksummed length-prefixed persistence:** binary-safe values and deterministic truncated/corrupt-tail rejection without parsing ambiguous text.
5. **Snapshot then AOF:** snapshot is a base state; AOF contains later mutations. Successful SAVE replaces the snapshot then truncates AOF while holding the persistence lock.
6. **FNV-1a ring:** stable across processes/platforms, unlike implementation-defined `std::hash`. Virtual nodes smooth placement.
7. **No fallback routing:** sending a failed owner’s request elsewhere would create inconsistent duplicates. The router reports failure honestly.
8. **Lazy expiration:** avoids a background scheduler and reduces idle cost; expired memory can remain until touched or scanned.
