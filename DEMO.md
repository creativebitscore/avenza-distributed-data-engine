# Avenza demonstration guide

This walkthrough uses the real protocol and executable arguments implemented by Avenza. Windows examples assume a Visual Studio multi-configuration build under `build/Release`; for Ninja or Linux, replace `build/Release/` with `build/` and omit `.exe`.

## 1. Start the server

Open terminal 1 from the repository root:

```powershell
.\build\Release\avenza-server.exe --port 7001 --capacity 10000 --data-dir .\demo-data
```

Expected startup line:

```text
Avenza listening on port 7001
```

Persistence is enabled unless `--no-persistence` is supplied.

## 2. Connect with the CLI

Open terminal 2:

```powershell
.\build\Release\avenza-cli.exe --host 127.0.0.1 --port 7001
```

Confirm connectivity:

```text
> PING
PONG
```

## 3. Store and retrieve a key

Quotes allow values containing spaces:

```text
> SET candidate "Aparna Software Engineer"
OK
> GET candidate
VALUE "Aparna Software Engineer"
> EXISTS candidate
1
```

## 4. Delete a key

```text
> DEL candidate
1
> GET candidate
NIL
```

A second `DEL candidate` returns `0` because the key no longer exists.

## 5. Demonstrate TTL expiration

`SET` supports the optional `EX seconds` form:

```text
> SET verification pending EX 3
OK
> TTL verification
3
```

Wait at least three seconds:

```text
> GET verification
NIL
> TTL verification
-2
```

`-2` means missing or expired; `-1` means a live key with no expiration.

## 6. Demonstrate restart recovery

With the server still using `--data-dir .\demo-data`:

```text
> SET durable "survives restart"
OK
> SAVE
OK
```

`SAVE` writes `cache.snapshot`, installs it through a temporary-file replacement, and truncates the AOF. Stop terminal 1 with Ctrl+C, then restart with the same port and data directory:

```powershell
.\build\Release\avenza-server.exe --port 7001 --capacity 10000 --data-dir .\demo-data
```

Reconnect the CLI and verify:

```text
> GET durable
VALUE "survives restart"
```

To demonstrate AOF-only recovery, write another key without `SAVE`, stop and restart with the same directory, then `GET` it. Mutating commands flush an AOF record before returning.

## 7. Demonstrate Pub/Sub

Pub/Sub is implemented, but the interactive CLI performs synchronous request/response reads rather than continuously reading asynchronous messages. Use a raw TCP subscriber for a clean demonstration.

### Windows subscriber

Open terminal 3 and run:

```powershell
$tcp = [Net.Sockets.TcpClient]::new('127.0.0.1', 7001)
$stream = $tcp.GetStream()
$writer = [IO.StreamWriter]::new($stream, [Text.UTF8Encoding]::new($false))
$writer.AutoFlush = $true
$reader = [IO.StreamReader]::new($stream, [Text.UTF8Encoding]::new($false))
$writer.WriteLine('SUBSCRIBE updates')
$reader.ReadLine()       # OK
$reader.ReadLine()       # waits for MESSAGE
```

In the normal CLI in terminal 2:

```text
> PUBLISH updates "build passed"
1
```

Terminal 3 receives:

```text
MESSAGE "updates" "build passed"
```

Close the raw subscriber when finished:

```powershell
$tcp.Dispose()
```

On Linux, `nc 127.0.0.1 7001` can be used as the subscriber; enter `SUBSCRIBE updates` and keep the connection open.

## 8. Inspect statistics

```text
> STATS
size=1 capacity=10000 hits=1 misses=0 sets=2 deletes=0 evictions=0 expirations=1
```

Exact counters depend on the commands already executed. `INFO` returns the same fields.

## 9. Run the benchmark

Close the CLI or use a separate terminal; the benchmark is in-process and does not need the server:

```powershell
.\build\Release\avenza-benchmark.exe 100000 10000
```

Arguments are operation count and cache capacity. Output contains throughput plus average, P50, P95, and P99 latency for SET-only, GET-only, and an 80% GET/20% SET workload. These measurements cover the cache engine, not TCP or persistence.

## Optional: three-node routing

Start nodes on ports 7001–7003 using three data directories, or use:

```powershell
.\scripts\start-cluster.ps1
```

Then launch the router:

```powershell
.\build\Release\avenza-cluster.exe 127.0.0.1:7001 127.0.0.1:7002 127.0.0.1:7003
```

```text
cluster> NODES
cluster> MAP candidate
cluster> SET candidate "Aparna"
cluster> GET candidate
```

The router reports a failed owner instead of rerouting because replication and automatic failover are not implemented.
