# 017 — Database Ownership, Connection Pooling, and Scaling Strategy

**Date**: 2026-05-18
**Status**: Accepted
**Origin**: Developer identified the DB ownership question as a classic system design topic and raised connection exhaustion under load independently. The read replica vs sharding discussion was developer-initiated — developer correctly identified that splitting DBs doesn't solve connection exhaustion when the bottleneck is a single service.

## Context

Three services (core-api, uploader-api, watcher-api) need access to PostgreSQL. Decisions required:
1. Which service owns which tables (data ownership)
2. How to prevent connection exhaustion under load
3. How far to take the scaling strategy

## Decision 1: Shared Database with Logical Ownership (Option C)

### Options Considered

**Option A — Shared database, no ownership rules**
All services read/write any table freely. Simple but creates tight coupling at the data layer — schema changes in one service can silently break another.

**Option B — Database per service**
Each service owns a separate PostgreSQL database. True isolation but forces API calls for every cross-service data access. `watcher-api` checking video existence on every chunk request = hundreds of HTTP calls per video playback. Eliminated by performance requirements.

**Option C — Shared database, documented ownership**
One PostgreSQL instance. Tables are clearly owned by one service. Only the owner writes to its tables; other services read where needed but never write across boundaries.

### Decision: Option C

`watcher-api` needs to validate video existence and permissions on streaming requests. Making this an API call per chunk would add latency to every segment delivery, degrading playback. Direct DB reads (read-only, across service boundary) are an explicit documented exception.

### Table Ownership

| Table | Owner | Writers | Readers |
|---|---|---|---|
| `users` | core-api | core-api only | core-api |
| `videos` | core-api | core-api, uploader-api (status updates only) | all services |
| `transcode_jobs` | uploader-api | uploader-api only | uploader-api |
| `favorites` | core-api | core-api only | core-api |
| `sessions` (Redis) | core-api | core-api only | core-api |

Cross-service writes are forbidden — `uploader-api` updating `videos.transcode_status` is permitted as a documented exception because the column is operationally owned by the upload pipeline.

## Decision 2: Connection Pooling via PgBouncer

### The Problem

PostgreSQL's default `max_connections=100` means 100 simultaneous connections before new requests are rejected. Each connection uses ~5–10 MB RAM. On a 4 GB VPS, the practical ceiling is ~200–300 connections. 10,000 concurrent users = 10,000 connection attempts = PostgreSQL crash without pooling.

Splitting databases does not solve this — the bottleneck is the service handling the traffic (core-api), not the shared topology.

### Solution: Two-Layer Connection Pooling

**Layer 1 — Drogon's built-in async connection pool (application level)**
Each service configures a pool of DB connections. Requests queue at the application layer.

**Layer 2 — PgBouncer (database level)**
Lightweight proxy (~10 MB RAM) between all services and PostgreSQL. Accepts unlimited client connections, maintains a small pool of real PostgreSQL connections. Transaction-mode pooling: connection returned to pool immediately after each transaction.

```
core-api     (Drogon pool: 20) ──→ ┐
uploader-api (Drogon pool: 10) ──→ ├── PgBouncer (pool: 50) ──→ PostgreSQL
watcher-api  (Drogon pool: 10) ──→ ┘

PostgreSQL max connections seen: 50 (regardless of concurrent users)
```

PgBouncer runs as a Docker container in the compose file. Services connect to `pgbouncer:5432` instead of `postgres:5432` directly.

## Decision 3: Scaling Roadmap

### Phase 1 (current — VPS)
- PgBouncer + Drogon connection pools
- Redis caching for video metadata and listings (already planned, ADR 002)
- Handles: thousands of concurrent users within VPS hardware limits

### Phase 2 (home server migration)
- PostgreSQL read replica via streaming replication
- `watcher-api` and read-heavy `core-api` queries → replica
- Writes → primary
- Home server (12 GB RAM, 4 cores) comfortably runs both primary and replica
- Handles: significantly higher read throughput

### Explicitly Not Planned: Sharding
Sharding splits data horizontally across multiple database instances to handle write throughput or dataset size that exceeds a single machine. Streawman's data model (users, video metadata) will not approach the volume that requires sharding in any realistic scenario. A properly indexed single PostgreSQL primary handles billions of rows. Adding sharding would introduce enormous operational complexity with zero benefit — this is a documented non-decision, not an oversight.

## Consequences

- All services connect to `pgbouncer:5432`, not `postgres:5432` directly
- PgBouncer config file lives at `infra/pgbouncer/pgbouncer.ini`
- PgBouncer is added as a service in `docker-compose.prod.yml`
- Cross-service table reads are allowed but must be read-only and documented in this ADR
- PostgreSQL schema migrations are owned by the service that owns the table — `core-api` runs users/videos migrations, `uploader-api` runs transcode_jobs migrations
- Phase 2 read replica: update connection strings in `watcher-api` and read paths in `core-api` to point to replica; write paths stay on primary
