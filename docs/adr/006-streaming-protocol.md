# 006 — Streaming Protocol

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: AI recommended HLS as the starting point due to its direct FFmpeg integration and the availability of hls.js. Developer was new to streaming protocols and accepted the recommendation after understanding the HLS vs DASH tradeoffs.

## Context

The `watcher-api` must deliver video to the browser in a way that supports adaptive quality switching — serving lower resolution chunks when the viewer's connection is slow and upgrading automatically when bandwidth improves. The format also determines how FFmpeg produces output, how files are stored on disk, and which browser player library is used.

## Options Considered

**HLS (HTTP Live Streaming)**
- Developed by Apple, now an industry-wide standard
- Manifest format: `.m3u8` playlist (plain text, human-readable)
- Segment format: `.ts` (MPEG-2 Transport Stream)
- Adaptive quality: master `.m3u8` references per-resolution playlists; player switches between them
- FFmpeg generates HLS natively with `-hls_time`, `-hls_segment_filename` flags — minimal extra work
- Browser support: native in Safari; all other browsers via `hls.js` library
- Segments are plain files served over standard HTTP GET — no special server logic needed

**DASH (Dynamic Adaptive Streaming over HTTP)**
- MPEG industry standard, technically more flexible than HLS
- Manifest format: `.mpd` (XML, less readable)
- Segment format: fragmented `.mp4`
- Used by YouTube and Netflix internally
- No native browser support — always requires `dash.js`
- More complex FFmpeg pipeline and manifest generation

**Custom chunked HTTP**
- Full control, no standard dependency
- Would require building a custom player or low-level `<video>` + fetch integration
- High implementation cost with no portfolio benefit over a known standard

## Decision

**HLS**

The FFmpeg → HLS pipeline is the most direct path from upload to playable video. Segments are static files served with standard HTTP — `watcher-api` has no complex streaming logic beyond file serving. `hls.js` handles all adaptive quality logic in the browser. This is what most self-hosted streaming platforms (Jellyfin, Plex) use.

## File Layout

```
{CHUNK_STORAGE_PATH}/{video_id}/
├── master.m3u8
├── 1080p/
│   ├── 1080p.m3u8
│   ├── seg_001.ts
│   └── seg_002.ts ...
├── 720p/  ...
├── 480p/  ...
└── 360p/  ...
```

## Consequences

- `watcher-api` serves `.m3u8` and `.ts` files; no custom streaming protocol implementation needed
- Browser player library is `hls.js` (Client Component in Next.js)
- FFmpeg must be configured to output HLS segments with consistent naming and duration
- Segment duration of 6 seconds is a standard starting point (`-hls_time 6`)
- Migration to DASH possible later as an ADR — same storage layout, different manifest
