# Avenza persistence and recovery

Each node owns `cache.snapshot` and `cache.aof`. Both contain length-prefixed records with a magic number, type, key/value sizes, expiry deadline, and FNV-1a checksum. AOF records only successful mutations: set, delete, expire, and flush. Reads and publish operations are never logged.

Startup loads the snapshot first and replays AOF second. Records whose expiry deadline passed while offline are skipped. On an incomplete, oversized, checksum-invalid, or unknown trailing record, replay preserves all prior valid records and stops; it never guesses past corruption.

`SAVE` writes `cache.snapshot.tmp`, flushes/closes it, rotates the old snapshot to a backup, renames the temporary file, removes the backup, and truncates AOF. Rename behavior is atomic on the usual same-filesystem Windows/Linux path, though durability against sudden power loss also depends on filesystem semantics; directory `fsync` is not implemented. AOF flushes C++ streams per mutation but does not call OS `fsync`, trading strict durability for simplicity/performance.
