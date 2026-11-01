# Avenza wire protocol

Requests and responses are UTF-8-oriented byte lines terminated by LF (CRLF accepted). Tokens are whitespace separated; double quotes allow whitespace and the escapes `\\`, `\"`, `\n`, `\r`, `\t`. Keys must be non-empty. A request is limited to 1 MiB.

Successful reads return `VALUE "escaped bytes"`; a missing key returns `NIL`. Integer/boolean results are decimal. Errors begin `ERR `. Pub/Sub delivery is `MESSAGE "channel" "message"`. After `SUBSCRIBE`, command responses and message frames share the connection; clients must continuously read and distinguish the first token.

| Command | Response |
|---|---|
| `PING` | `PONG` |
| `SET k v [EX positive-seconds]` | `OK` |
| `GET k` | `VALUE "v"` or `NIL` |
| `DEL/EXISTS k` | `1` or `0` |
| `EXPIRE k seconds` | `1` or `0`; non-positive deletes an existing key |
| `TTL k` | seconds, `-1` permanent, `-2` absent |
| `STATS/INFO` | space-separated `name=value` fields |
| `FLUSHALL`, `SAVE` | `OK` |
| `PUBLISH channel message` | subscriber count |
| `SUBSCRIBE/UNSUBSCRIBE channel` | `OK`, or `1`/`0` |

Unknown commands, malformed quotes, invalid integers, and wrong arity are errors and do not mutate state.
