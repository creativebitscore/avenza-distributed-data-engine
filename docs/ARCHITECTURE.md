# Avenza architecture

## Components and request lifecycle

`avenza-server` accepts TCP connections and gives each a session thread. The session buffers bytes until newline, enforces a 1 MiB cap, parses quoted tokens, validates arity and types, and dispatches to the cache, persistence, or Pub/Sub component. A per-client write mutex prevents asynchronous `MESSAGE` frames from interleaving with command responses.

The cache owns an `unordered_map<string, Entry>` and a `list<string>` ordered most-to-least recent. Each map entry holds its list iterator, so touch and removal use `splice`/`erase` in constant time. A single mutex makes map/list/statistics changes atomic. TTL deadlines use `steady_clock`, avoiding wall-clock jumps. Expired entries are removed on access and during size/snapshot scans.

## Complexity

| Operation | Expected time | Reason |
|---|---:|---|
| SET / GET / DEL / EXISTS / EXPIRE / TTL | O(1) average | hash lookup plus constant-time list work |
| LRU eviction | O(1) | list back + stored iterator |
| FLUSHALL | O(n) | destroy all entries |
| SAVE / startup recovery | O(n) | serialize/replay records |
| expired purge during scan | O(n) | intentional full scan |
| publish | O(s) | copy and call `s` channel subscribers |
| ring lookup | O(log v) | ordered-map lower bound across virtual points |
| add/remove node | O(v) scan/insertion | `v` virtual points |

Worst-case hash-table operations are O(n). Memory is O(n + virtual nodes + subscriptions).

## Concurrency and shutdown

Cache, Pub/Sub, persistence, and socket output have separate locks. Pub/Sub copies callbacks while locked and invokes them after unlocking, avoiding lock re-entry. The listener and client sockets are shut down to wake blocking calls. SIGINT/SIGTERM initiates shutdown. Sessions remove subscriptions on disconnect.

The design favors clarity over peak scalability: one global cache mutex and one thread per connection are appropriate for an educational local deployment, not millions of connections.
