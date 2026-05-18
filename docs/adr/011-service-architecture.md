# 011 — Service Architecture (3 C++ Backend Services)

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: Developer's idea. While mapping Nginx routing rules, the developer independently identified that `uploader-api` was accumulating unrelated concerns (auth, user metadata, search, favorites) and proposed adding a third service to enforce proper separation. AI validated the instinct and confirmed it as a standard service decomposition pattern.

## Context

The initial design had two C++ backend services: `uploader-api` and `watcher-api`. As the routing rules were being defined, it became clear that `uploader-api` would need to own authentication, user profiles, video metadata, search, and favorites — none of which are "upload" concerns. Mixing these into an uploader violates single responsibility and produces a God Service.

## Options Considered

**Option A — Rename, keep 2 services**
Rename `uploader-api` → `core-api`, making it responsible for everything except HLS streaming. Fewer containers and routing entries, but `core-api` would still be a broad service.

**Option B — True 3-service split**
Extract a dedicated `core-api` for user-facing application logic, keeping `uploader-api` focused solely on file ingestion:

```
core-api      ← auth, users, video metadata, search, favorites
uploader-api  ← file ingestion + transcoding trigger only
watcher-api   ← HLS file serving only
```

Each service has a single, clear reason to exist. Boundaries are enforceable and independently deployable.

## Decision

**Option B — 3 C++ backend services**

```
streawman-ui   ← Next.js (SSR) — all browser-facing pages
core-api       ← C++ Drogon — auth, users, video catalog, search, favorites
uploader-api   ← C++ Drogon — file ingestion, transcoding trigger, job status
watcher-api    ← C++ Drogon — HLS manifest and segment serving
```

This split was made before any implementation, which keeps the refactoring cost at zero. Service boundaries drawn this way are a natural fit for future extraction of the `transcoder` service (RabbitMQ discussion pending — would slot between `uploader-api` and the FFmpeg worker).

## Service Responsibilities

### core-api
- `POST /auth/login` — issue access token + refresh token
- `POST /auth/refresh` — rotate refresh token
- `POST /auth/logout` — revoke refresh token
- `GET/PUT /users/me` — user profile
- `GET /videos` — video catalog listing (with search/filter)
- `GET /videos/{id}` — video metadata
- `POST /videos/{id}/favorites` — add to favorites
- `DELETE /videos/{id}/favorites` — remove from favorites

### uploader-api
- `POST /upload` — receive file, save to disk, enqueue transcode job, respond 202
- `GET /upload/status/{jobId}` — check transcode job status

### watcher-api
- `GET /stream/{videoId}/master.m3u8` — serve HLS master playlist
- `GET /stream/{videoId}/{resolution}/{resolution}.m3u8` — serve resolution playlist
- `GET /stream/{videoId}/{resolution}/{filename}.ts` — serve segment file

## Nginx Routing

```nginx
location /api/auth/    { proxy_pass http://core-api:8080; }
location /api/users/   { proxy_pass http://core-api:8080; }
location /api/videos/  { proxy_pass http://core-api:8080; }
location /api/upload/  { proxy_pass http://uploader-api:8081; }
location /stream/      { proxy_pass http://watcher-api:8082; }
location /             { proxy_pass http://streawman-ui:3000; }
```

## Consequences

- Three Docker containers, three internal ports, slightly more Nginx config
- `core-api` owns the PostgreSQL schema for users and video metadata
- `uploader-api` writes video records to PostgreSQL via `core-api`'s API, or directly to shared DB — [FILL: decide whether services share the same DB or each owns its tables]
- All three C++ services validate the JWT access token independently (Drogon auth filter)
- The `transcoder` service (future) will consume from `uploader-api`'s job queue without touching `core-api` or `watcher-api`
