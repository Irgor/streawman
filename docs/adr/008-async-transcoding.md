# 008 — Async Transcoding Strategy

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: Developer's idea. Independently proposed async transcoding with parallel FFmpeg processes per resolution, 202 response on upload, and user notification on completion — reasoning from C++'s multi-threading capabilities. AI validated the approach and added the Redis job queue and WebSocket notification details.

## Context

Transcoding a video into multiple HLS resolutions is slow (seconds to minutes depending on length and VPS specs). The upload request must not block waiting for transcoding to complete. Users need feedback on progress and a notification when their video is ready.

## Options Considered

**Synchronous — block the upload response**
- Simplest implementation: one request, one response, done
- Unacceptable UX for any video longer than a few seconds
- Ties up the Drogon worker thread for the entire transcode duration
- Not viable

**Async — single FFmpeg process, all resolutions sequentially**
- Non-blocking upload response
- Transcodes one resolution at a time: 1080p → 720p → 480p → 360p
- Total time = sum of all resolution transcode times
- Simple to implement, wastes available CPU

**Async — parallel FFmpeg processes, one per resolution**
- Non-blocking upload response
- Spawns one FFmpeg process per resolution simultaneously
- Total time ≈ time for the slowest resolution (usually 1080p)
- Requires a thread pool to manage concurrent processes
- CPU usage spikes during transcode — acceptable on a dedicated VPS/home server

**Async — external queue (RabbitMQ) + separate transcoder service**
- Fully decoupled: uploader-api and transcoder are independent services
- Retry logic, dead letter queue for failures, horizontal scaling
- Higher operational complexity
- Under discussion for a future upgrade — not implemented in Phase 1

## Decision

**Async — parallel FFmpeg processes per resolution, Redis List as job queue**

The upload API responds immediately (202 Accepted). A thread pool in `uploader-api` pops jobs from a Redis List and spawns one FFmpeg process per target resolution in parallel. Total transcode time is reduced to the slowest resolution's duration. When all resolutions complete, `master.m3u8` is generated and the video status is updated in PostgreSQL.

## Implementation Flow

```
1. POST /upload
   └─→ save raw file to disk: {CHUNK_STORAGE_PATH}/{video_id}/raw.{ext}
   └─→ insert video record in PostgreSQL: { status: "queued" }
   └─→ RPUSH transcode_queue '{ "videoId": "...", "filePath": "..." }'
   └─→ return 202: { "videoId": "...", "status": "queued" }

2. Thread pool worker (BLPOP transcode_queue)
   └─→ update PostgreSQL status: "transcoding"
   └─→ spawn in parallel:
       ├── ffmpeg → 1080p HLS segments
       ├── ffmpeg → 720p  HLS segments
       ├── ffmpeg → 480p  HLS segments
       └── ffmpeg → 360p  HLS segments
   └─→ wait for all processes to complete
   └─→ generate master.m3u8
   └─→ update PostgreSQL status: "ready"
   └─→ trigger notification (WebSocket if user is online, email otherwise)
   └─→ on any FFmpeg failure: update status "failed", log error
```

## Notification Strategy

- **WebSocket** (Drogon native): pushed to the browser if the user has the site open
- **Email via SMTP**: sent when the transcoding completes regardless of browser state
- [FILL: SMTP server — self-hosted Postfix? external provider like Resend or Mailgun?]

## Future Upgrade Path

When RabbitMQ is added, the Redis List queue is replaced by a RabbitMQ exchange. The thread pool worker becomes a standalone `transcoder` service. The `uploader-api` only publishes jobs — it no longer owns the worker. Dead letter queue handles failures with retry logic. This is documented as a planned ADR.

## Consequences

- `uploader-api` owns both the HTTP layer and the thread pool worker in Phase 1
- Thread pool size: VPS has 2 vCPU — cap at 2 concurrent FFmpeg processes, or spawn all 4 with `-threads 1` each; benchmark both
- Multiple simultaneous uploads will queue — the Redis List serializes jobs naturally
- FFmpeg binary must exist in the `uploader-api` Docker image
