# 009 — Inter-Service Communication

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: Developer chose REST over gRPC after AI presented the full tradeoff. The decision was primarily driven by the end-of-June deadline and the fact that HLS delivery already forces HTTP — REST keeps the stack consistent. Developer noted gRPC as a future candidate if a dedicated transcoder service is added.

## Context

`streawman-ui` (Next.js) needs to call `uploader-api` and `watcher-api` for metadata, auth, and video listings. A protocol must be chosen for these server-to-server calls.

## Options Considered

**REST (HTTP/JSON)**
- Standard HTTP requests with JSON payloads
- No tooling beyond a fetch client — no codegen, no schema compilation step
- Human-readable, trivially debuggable with curl or browser DevTools
- Already required for browser-facing endpoints (hls.js fetches `.m3u8` and `.ts` directly over HTTP)
- Consistent protocol across the entire stack

**gRPC (HTTP/2 + protobuf)**
- Binary protocol, 5–10x higher internal throughput than REST
- Strongly typed `.proto` contracts with auto-generated client/server code
- Browser cannot speak gRPC natively — gRPC-Web required for any browser-facing call
- Strong CV signal for internal microservice communication patterns
- Significant additional tooling: protobuf compiler, codegen for C++ and TypeScript

**Hybrid (REST browser-facing, gRPC server-to-server)**
- Architecturally correct — mirrors what Netflix, Lyft, and others do in production
- Requires maintaining two protocol layers simultaneously
- Best suited as a future addition if a dedicated internal service (e.g. transcoder) is extracted

## Decision

**REST everywhere**

The primary drivers:
1. `watcher-api` must already speak HTTP for HLS delivery — REST is already in the stack
2. All services run on the same host; intra-host latency is ~0.1ms, eliminating the performance case for gRPC
3. Target completion is end of June 2026 — REST removes the protobuf + codegen setup cost
4. The ADR reasoning demonstrates awareness of gRPC and a deliberate choice, which is the actual CV signal

## Future gRPC Path

If a dedicated `transcoder` service is extracted (planned for the RabbitMQ discussion), `uploader-api` → `transcoder` is the natural first gRPC candidate: high-frequency internal calls, no browser involvement, typed job status contracts. That would be ADR 010+.

## Consequences

- All API endpoints in `uploader-api` and `watcher-api` are HTTP/JSON
- `streawman-ui` server-side calls use standard `fetch` with typed response wrappers
- API response shape must be consistent: `{ data: T }` for success, `{ error: string, code: string }` for errors
- OpenAPI specs for both services should be written before implementation — they serve as the REST contract
