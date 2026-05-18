# 013 — Job Queue Strategy

**Date**: 2026-05-18
**Status**: Accepted
**Origin**: Developer's decision. After understanding the RabbitMQ vs Redis tradeoff, developer chose to keep Redis and build the Dead Letter Queue manually — reasoning that implementing DLQ behavior by hand adds more learning value than delegating it to a message broker. RabbitMQ deferred to Phase 2 as a deliberate upgrade, not an oversight.

## Context

The async transcoding pipeline (ADR 008) needs a job queue: `uploader-api` publishes transcode jobs, a thread pool worker consumes them. The queue must handle failures gracefully — a crashed worker or failed FFmpeg process must not silently drop jobs.

## Options Considered

**Redis List (basic)**
- `RPUSH` to enqueue, `BLPOP` to dequeue
- Simple, already in the stack
- No built-in failure handling — if a worker crashes after popping a job, the job is gone

**Redis List + manual DLQ**
- Same as above, but failure handling is implemented explicitly in application code
- Failed jobs pushed to a separate `transcode_failed` list with retry metadata
- Retry worker periodically processes the failed list up to a max retry count
- Jobs exceeding max retries moved to a `transcode_dead` list and status updated in PostgreSQL
- Everything is visible and auditable in Redis directly

**RabbitMQ**
- Purpose-built message broker (AMQP protocol)
- Built-in Dead Letter Exchange, retry with backoff, management UI, crash-safe ack/nack
- Strong CV signal as a named technology
- Requires: one extra Docker container, an AMQP client library for C++ (`SimpleAmqpClient` or `librabbitmq`), learning AMQP concepts (exchanges, bindings, routing keys)
- Deferred to Phase 2: after home server migration, extracting the transcoder into a dedicated service is the natural trigger for introducing RabbitMQ properly

## Decision

**Redis List + manually implemented DLQ**

Redis is already in the stack. Building the failure handling by hand means understanding exactly what a message broker provides — retry logic, dead letter routing, visibility into failed jobs — because you'll have written each piece yourself. This is a stronger learning outcome than adding RabbitMQ as a black box.

## Redis Queue Layout

```
transcode_queue   ← active jobs (RPUSH / BLPOP)
transcode_failed  ← jobs that failed, pending retry (RPUSH / BLPOP)
transcode_dead    ← jobs that exceeded max retries (RPUSH only, manual review)
```

## Job Payload Shape

```json
{
  "jobId": "uuid",
  "videoId": "uuid",
  "filePath": "/data/raw/{videoId}.mp4",
  "retryCount": 0,
  "maxRetries": 3,
  "enqueuedAt": "ISO8601"
}
```

## Failure Handling Flow

```
Worker pops job from transcode_queue
  └─→ runs FFmpeg processes
  └─→ on success: update PostgreSQL status "ready", notify user
  └─→ on failure:
        retryCount < maxRetries → increment retryCount, RPUSH to transcode_failed
        retryCount >= maxRetries → RPUSH to transcode_dead, update PostgreSQL status "failed"

Retry worker (runs on interval, e.g. every 60s)
  └─→ BLPOP from transcode_failed
  └─→ RPUSH back to transcode_queue
```

## Phase 2 Upgrade Path

When the home server is ready and the transcoder is extracted as a separate service, replace this Redis queue with RabbitMQ:
- `transcode_queue` → RabbitMQ exchange with a work queue
- `transcode_failed` / `transcode_dead` → RabbitMQ Dead Letter Exchange (DLX)
- Worker acknowledgement replaces manual pop-and-track
- Document this migration as ADR 014+

## Consequences

- Manual DLQ requires implementing retry logic, retry counting, and dead letter routing in C++
- `transcode_failed` and `transcode_dead` lists should be monitored — add visibility to an admin endpoint in `core-api`
- Redis key TTLs should not be set on queue lists (jobs must not expire silently)
- PostgreSQL `videos` table needs a `transcode_status` column: `queued` | `transcoding` | `ready` | `failed`
