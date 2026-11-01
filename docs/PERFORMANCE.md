# Avenza performance methodology

Build Release mode and run `avenza-benchmark [operations] [capacity]` on an otherwise idle machine. The executable preloads data, uses a fixed RNG seed (42), measures each in-process operation with `steady_clock`, sorts samples, and reports operations/second plus average/P50/P95/P99 microseconds for SET-only, GET-only, and 80% GET/20% SET.

Record OS version, CPU, RAM, compiler/version, build type, operation count, capacity, and whether power/thermal controls were active. Run at least five warm repetitions and report distributions, not the best run. The benchmark measures the engine—not TCP, persistence, or multiple threads.

Optional Redis comparison: install Redis separately, use `redis-benchmark -t set,get -n 100000 -q`, and clearly label that its protocol, batching, allocator, persistence, and server path differ. For a fairer Avenza network comparison, add a dedicated pipelined TCP benchmark; the current client is one request/response at a time. No Redis dependency is required and this repository publishes no fabricated numbers.
