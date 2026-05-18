# Streawman — AI Agent Briefing

## What is this project?

**Streawman** is a self-hosted video and photo streaming platform built as a portfolio and learning project.

The name is a portmanteau of *streaming* + *straw man*.

## Timeline

**Target completion: end of June 2026 (flexible — personal/learning project, no hard deadline)**
Scope decisions should favor learning and understanding over speed. Features can be layered in after the core works.

## Core goal

Demonstrate production-quality skills across the full stack:
- Frontend (Next.js SSR)
- Backend (C++ with Drogon, async, RBAC)
- System Design (documented ADRs, flows, capacity thinking)
- Infra (Docker, CI/CD, Hetzner VPS → home server)
- Quality (k6 load/stress tests with threshold-based CI reports)

## CRITICAL: How to work with the developer

This is a **learning project**. The developer wants to understand every line and command.

- **Guide and explain** — never write implementation code or run essential commands on their behalf
- **Present options with tradeoffs** — let the developer decide
- **Review and critique** their work honestly
- See `.cursor/rules/03-ai-behavior.mdc` for full AI behavior policy

## Services

| Service | Language | Purpose |
|---|---|---|
| `streawman-ui` | Next.js | Single app: permission-gated upload + public watch/stream |
| `core-api` | C++ (Drogon) | Auth, users, video metadata, search, favorites |
| `uploader-api` | C++ (Drogon) | File ingestion + transcoding trigger only |
| `watcher-api` | C++ (Drogon) | HLS manifest and segment serving only |

## Domain

`streawman.codigor.dev`

## Confirmed decisions

- [x] C++ HTTP framework → **Drogon** (ADR 001)
- [x] Primary database → **PostgreSQL** (ADR 002)
- [x] Cache / session store → **Redis** (ADR 002)
- [x] Auth → **Dual-token JWT** with refresh rotation in Redis (ADR 003)
- [x] Auth roles → `viewer` and `uploader`
- [x] Transcoding → **FFmpeg CLI** now, libav later (ADR 004)
- [x] Transcoding strategy → **async**, parallel per resolution, Redis job queue (ADR 008)
- [x] Streaming protocol → **HLS** (.m3u8 + .ts segments, hls.js in browser) (ADR 006)
- [x] Reverse proxy → **Nginx** + Certbot for TLS (ADR 007)
- [x] Load testing → **k6** with CI threshold reports (ADR 005)
- [x] File storage → local disk (VPS / home server), path from env var

## Still open

- [x] Inter-service communication → **REST (HTTP/JSON)** everywhere (ADR 009)
- [x] Service architecture → **3 C++ backend services**: core-api, uploader-api, watcher-api (ADR 011)
- [x] Chunk storage naming → `{CHUNK_STORAGE_PATH}/{video_id}/{resolution}/seg_%04d.ts` (ADR 010)
- [x] Nginx routing → `/api/*` core-api · `/ws` core-api · `/upload` uploader-api · `/stream` watcher-api (ADR 007)
- [x] Max upload size → `10G` (`client_max_body_size`)
- [x] Message queue → **Redis List** for Phase 1; manual DLQ implementation in Redis (ADR 013). RabbitMQ deferred to Phase 2 (post home server).
- [x] Build system → **CMake + Ninja** (ADR 012)
- [x] CI/CD → **GitHub Actions** + **Monorepo**, path-triggered per service, GitHub-hosted runners (ADR 014)
- [x] C++ unit test framework → **Google Test + gmock** (ADR 015)
- [x] Styling → **Tailwind CSS** (ADR 016)
- [x] DB ownership → **Shared DB, logical ownership** + PgBouncer connection pooler + read replica Phase 2. Sharding: explicit non-decision. (ADR 017)
- [x] VPS specs → Hetzner CX23: 2 vCPU, 4 GB RAM, 40 GB disk, Ubuntu, €4.95/mo (dev only)
- [x] Home server → Lenovo IdeaPad: Ryzen 7 3700U (4c/8t), 12 GB RAM, 512 GB NVMe + 1–2 TB SSD (arriving ~3–4 months)
- [ ] Home server networking → Cloudflare Tunnel vs port forwarding + DDNS (decide when hardware arrives)
