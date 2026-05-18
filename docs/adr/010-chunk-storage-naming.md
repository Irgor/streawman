# 010 — Chunk Storage Naming Convention

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: AI presented the options and recommended Option B (resolution subdirectory). Developer chose `%04d` segment padding (over AI's initial `%03d` suggestion) after independently calculating ~16 hours of headroom at 6s segments — correctly identifying that `%03d` was too tight for longer content.

## Context

HLS streaming requires that segment filenames written inside `.m3u8` playlists by FFmpeg exactly match the actual files on disk. `watcher-api` serves both the playlist and the segments, constructing file paths from the video ID, resolution, and segment filename. The naming convention is therefore a hard contract between FFmpeg output, disk layout, and `watcher-api` routing logic.

## Options Considered

**Option A — Flat directory per video**
All segments for all resolutions in one directory, prefixed by resolution:
`/data/{video_id}/seg_1080p_0001.ts`
- Simple path structure but produces 2,000–3,000+ files in one directory for long videos
- Filesystems degrade above ~1,000 files per directory
- Eliminated.

**Option B — Resolution subdirectory**
```
{CHUNK_STORAGE_PATH}/{video_id}/{resolution}/seg_%04d.ts
```
- Matches FFmpeg's natural output when given a directory target
- Each directory holds only segments for one resolution — manageable file count
- Deleting or reprocessing a single resolution is a single `rm -rf` on one directory
- Human-readable at a glance
- `watcher-api` path construction: `{root}/{videoId}/{resolution}/{filename}`

**Option C — Hash-based**
- Content-addressed paths for inode distribution at massive scale
- Unreadable, adds lookup complexity, overkill for a personal server
- Eliminated.

**Option D — Date-based hierarchy**
- Date is metadata — belongs in PostgreSQL, not the filesystem path
- Forces the date into every path construction call
- Eliminated.

## Decision

**Option B — Resolution subdirectory with `seg_%04d.ts` segment format**

Full layout:
```
{CHUNK_STORAGE_PATH}/
└── {video_id}/
    ├── master.m3u8
    ├── 1080p/
    │   ├── 1080p.m3u8
    │   ├── seg_0001.ts
    │   └── seg_0002.ts ...
    ├── 720p/
    ├── 480p/
    └── 360p/
```

## Segment Format Rationale

`%04d` = 4-digit zero-padded integer → supports `seg_0001.ts` through `seg_9999.ts`.

At 6 seconds per segment: 9,999 × 6s = 59,994s ≈ **16.6 hours** per video. Sufficient headroom for any realistic content.

`%03d` (max 999 segments ≈ 1.5 hours) was ruled out as too tight for longer videos.
`%06d` adds visual noise with no benefit at this scale.

## FFmpeg Flag

```
-hls_segment_filename "{CHUNK_STORAGE_PATH}/{video_id}/{resolution}/seg_%04d.ts"
```

FFmpeg writes this pattern into the `.m3u8` playlist automatically — no manual playlist editing needed.

## Consequences

- `watcher-api` serves files at: `GET /watch/{videoId}/{resolution}/{filename}`
- Nginx must not strip path segments when proxying to `watcher-api`
- `CHUNK_STORAGE_PATH` env var must be identical in both `uploader-api` (writes) and `watcher-api` (reads) — use a shared Docker volume
- Cleanup on failed transcode: `rm -rf {CHUNK_STORAGE_PATH}/{video_id}/` removes all artifacts
