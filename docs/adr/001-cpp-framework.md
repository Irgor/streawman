# 001 — C++ HTTP Framework

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: Options and tradeoffs presented by AI. Developer chose Drogon over Oat++ after weighing performance vs code clarity, prioritising raw throughput for the streaming use case.

## Context

Streawman requires two C++ HTTP services: `uploader-api` (file upload + transcoding) and `watcher-api` (concurrent adaptive chunk streaming). The framework choice determines how async concurrency is handled, which directly affects streaming performance under load.

The developer has prior REST/Node.js experience but is learning C++, so approachability was also considered. The learning plan is to start with `cpp-httplib` (synchronous, single-header) to understand the mechanics, then migrate to the chosen production framework before any business logic is built.

## Options Considered

**cpp-httplib**
- Single header, synchronous, minimal setup
- Good for learning what a C++ HTTP server actually does
- Not suitable for production concurrent streaming
- Used only in the learning phase (Path A)

**Crow**
- Header-only, Flask/Express-like routing
- Approachable for someone coming from web backgrounds
- Lower throughput than Drogon; less active maintenance

**Oat++**
- Clean REST API, type-safe endpoints, good documentation
- Readable code — good for portfolio reviewability
- Async support exists but not as performant as Drogon
- HTTP/2 support less mature

**Drogon**
- Consistently top-ranked in TechEmpower benchmarks (1–7M req/s)
- Coroutine-based async (`co_await`) — natural fit for concurrent streaming
- Native HTTP/1.1 + HTTP/2 + WebSocket
- Built-in async PostgreSQL and Redis clients
- Steeper initial setup (uses `drogon_ctl` scaffold tooling)

## Decision

**Drogon**

The project's primary BE challenge is concurrent chunk delivery (`watcher-api`) — this is an I/O concurrency problem where Drogon's coroutine model is the most natural fit. The performance gap over Oat++ is real and demonstrable via k6 reports, which strengthens the portfolio case. The learning curve is acceptable given the developer's existing REST knowledge.

## Consequences

- CMake is effectively required (Drogon's ecosystem assumes it)
- All DB and cache access must use Drogon's async clients — no synchronous drivers
- Event loop must never be blocked — FFmpeg calls and file I/O run in separate threads or async tasks
- The `cpp-httplib` learning phase must be completed and discarded before business logic is written
