# 004 — Transcoding Pipeline

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: AI recommended FFmpeg CLI as the starting point (lower barrier, same output quality). Developer accepted the phased approach: learn with CLI, migrate to libav after the full system works end-to-end.

## Context

Uploaded videos must be transcoded into multiple resolutions so the `watcher-api` can serve the appropriate quality based on viewer connection speed. Transcoding is CPU-bound and slow — it must not block the HTTP event loop.

## Options Considered

**FFmpeg CLI via subprocess (`popen` / `system`)**
- Call the `ffmpeg` binary from C++ code as a child process
- All encoding logic written as shell command arguments — well-documented
- No C++ complexity beyond process management and output parsing
- Progress reporting requires parsing stderr line-by-line
- Spawns a new OS process per job — acceptable for low upload volume

**libav (FFmpeg as a C library)**
- Link `libavcodec`, `libavformat`, `libavfilter` directly
- Maximum control: frame-level access, memory management, no process overhead
- Can stream frames directly without temp files
- API is notoriously complex — weeks of learning before a single frame transcodes
- The right long-term choice for a systems-programming portfolio

**External transcoding service (cloud API)**
- No self-hosting of FFmpeg
- Introduces external dependency and cost
- Defeats the purpose of a self-hosted portfolio project

## Decision

**Phase 1: FFmpeg CLI** — implemented now
**Phase 2: libav** — migrate after the full system works end-to-end

This mirrors the cpp-httplib → Drogon learning path: understand the problem with the simpler tool, then go deeper once the domain is understood.

## Implementation Notes

- FFmpeg binary must be installed on the VPS and available in `PATH`
- Transcode calls happen in a Drogon async task or separate thread — never in the request handler directly
- Output resolutions: [FILL: e.g. 1080p, 720p, 480p, 360p — decide before implementing]
- Output format: HLS — segments `seg_%04d.ts`, playlist `{resolution}.m3u8` per resolution — see ADR 010
- Chunk storage path root from env var `CHUNK_STORAGE_PATH`

## Consequences

- Phase 1 creates a hard dependency on the `ffmpeg` binary in the Docker image
- Phase 2 migration will require linking libav libraries in CMakeLists.txt and rewriting the transcoding core — business logic above it stays the same
- Async vs sync transcoding (blocking the upload response vs queuing) is a separate open decision
