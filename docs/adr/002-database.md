# 002 — Database and Cache

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: Developer's decision. Proposed PostgreSQL + Redis combination independently; AI confirmed the pairing and clarified the role separation between the two.

## Context

The system needs to store persistent data (users, video metadata, permissions) and handle fast, ephemeral data (auth sessions, caches, rate limits). These are different access patterns with different consistency and durability requirements.

## Options Considered

**PostgreSQL only**
- Relational, ACID, industry standard
- Could handle sessions via a sessions table, but adds latency and DB load for every request validation

**MongoDB + Redis**
- Flexible schema useful for variable metadata
- Video metadata is actually well-structured and relational — document store adds complexity without benefit here

**PostgreSQL + Redis**
- PostgreSQL for durable relational data
- Redis for fast ephemeral data: token store, cache, rate limiting
- Clear separation of concerns, both are industry standard, both have async Drogon clients

**SQLite + Redis**
- SQLite is simpler to operate (no server process)
- Not suitable for a project demonstrating production-readiness at scale

## Decision

**PostgreSQL** (primary) + **Redis** (cache and session store)

PostgreSQL stores:
- Users and roles
- Video/photo metadata (title, duration, available resolutions, file paths, upload timestamp)
- Upload records and status

Redis stores:
- Refresh tokens (key: `refresh:{token_hash}`, TTL matches token expiry)
- Video listing cache (invalidated on new upload)
- Rate limiting counters (max uploads per user per hour)

## Consequences

- Both services (uploader-api, watcher-api) need access to PostgreSQL and Redis
- Drogon's built-in async clients are used for both — no external ORM or driver
- File paths stored in PostgreSQL must use the same env-var-based root as the actual files on disk
- Redis is not a source of truth — it is always reconstructable from PostgreSQL
