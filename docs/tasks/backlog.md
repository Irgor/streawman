# Backlog

Tasks ordered by dependency — complete top to bottom.
To generate new tasks: "plan tasks for [feature/service]"
Task rules: `.cursor/rules/04-dev-workflow.mdc`

---

## TASK-001: Initialize monorepo directory structure
**Branch**: `infra/TASK-001-monorepo-structure`
**Service**: infra
**Depends on**: none
**Acceptance criteria**:
- [ ] All directories from the monorepo layout in `01-architecture.mdc` exist: `apps/streawman-ui/`, `services/core-api/`, `services/uploader-api/`, `services/watcher-api/`, `infra/docker/`, `infra/compose/`, `.github/workflows/`, `tests/load/`, `docs/system-design/`, `docs/adr/`
- [ ] `.gitignore` at root covers: C++ build artifacts (`build/`, `CMakeFiles/`, `*.o`, `*.a`, `*.so`), node_modules, `.env*`, compiled binaries, media files (`*.mp4`, `*.ts`, `*.jpg`, `*.png`)
- [ ] `README.md` at root describes the project (one paragraph), lists the four services, and has a "How to run" section with a placeholder
- [ ] Each service directory has an empty `.gitkeep` so the folders are tracked by git
**Status**: backlog

---

## TASK-002: Docker Compose dev environment
**Branch**: `infra/TASK-002-docker-compose-dev`
**Service**: infra
**Depends on**: TASK-001
**Acceptance criteria**:
- [ ] `infra/compose/docker-compose.dev.yml` exists with three services: `postgres` (image: postgres:16), `redis` (image: redis:7-alpine), `pgbouncer` (image: edoburu/pgbouncer)
- [ ] Postgres has a named volume for persistence, a healthcheck (`pg_isready`), and reads credentials from a `.env` file (not hardcoded)
- [ ] Redis has a healthcheck (`redis-cli ping`)
- [ ] PgBouncer is configured to pool connections to the postgres service (pool_mode = transaction)
- [ ] `docker compose -f infra/compose/docker-compose.dev.yml up -d` starts all three containers with no errors
- [ ] `docker compose -f infra/compose/docker-compose.dev.yml ps` shows all three as healthy
- [ ] A `.env.example` file at root documents all required env vars with placeholder values
**Status**: backlog

---

## TASK-003: core-api CMake + Ninja build setup
**Branch**: `feat/TASK-003-core-api-build`
**Service**: core-api
**Depends on**: TASK-001
**Acceptance criteria**:
- [ ] `services/core-api/CMakeLists.txt` exists and uses cmake_minimum_required ≥ 3.16 and project() declaration
- [ ] Drogon is declared as a dependency (via find_package or FetchContent)
- [ ] `cmake -B build -G Ninja` configures with no errors from `services/core-api/`
- [ ] `ninja -C build` compiles and produces a `core-api` executable
- [ ] Build produces zero warnings on `-Wall -Wextra`
- [ ] A `services/core-api/src/main.cpp` exists with a minimal Drogon app entrypoint (no endpoints yet, just starts the server)
**Status**: backlog

---

## TASK-004: core-api health check endpoint
**Branch**: `feat/TASK-004-core-api-health`
**Service**: core-api
**Depends on**: TASK-003
**Acceptance criteria**:
- [ ] `GET /health` returns HTTP 200 with body `{"status": "ok", "service": "core-api"}`
- [ ] Service listens on port 8080 (configurable via `PORT` env var)
- [ ] `curl http://localhost:8080/health` returns the expected response
- [ ] One gtest unit test exists that instantiates the handler and asserts the response body and status code
- [ ] Test runs and passes: `./build/tests/core-api-tests` exits 0
**Status**: backlog

---

## TASK-005: PostgreSQL schema + migration runner
**Branch**: `feat/TASK-005-db-schema`
**Service**: core-api
**Depends on**: TASK-002, TASK-004
**Acceptance criteria**:
- [ ] Migration SQL files live in `services/core-api/db/migrations/` named `001_create_users.sql`, `002_create_refresh_tokens.sql`, `003_create_videos.sql`
- [ ] `users` table: `id` (uuid, pk), `email` (unique), `password_hash`, `role` (enum: viewer/uploader), `created_at`
- [ ] `refresh_tokens` table: `id` (uuid, pk), `user_id` (fk → users), `token_hash`, `expires_at`, `revoked` (bool), `created_at`
- [ ] `videos` table: `id` (uuid, pk), `user_id` (fk → users), `title`, `status` (enum: pending/transcoding/ready/failed), `created_at`
- [ ] A bash script `services/core-api/db/migrate.sh` applies migrations in order and is idempotent (safe to run twice)
- [ ] Running `migrate.sh` against the dev Docker postgres creates all tables with no errors
**Status**: backlog

---

## TASK-006: User registration endpoint
**Branch**: `feat/TASK-006-user-registration`
**Service**: core-api
**Depends on**: TASK-005
**Acceptance criteria**:
- [ ] `POST /api/v1/auth/register` accepts `{"email": "...", "password": "...", "role": "viewer|uploader"}`
- [ ] Password is hashed with bcrypt before storage (never stored as plaintext)
- [ ] Returns `201` with `{"id": "<uuid>"}` on success
- [ ] Returns `409` with `{"error": "email already registered", "code": "EMAIL_CONFLICT"}` if email exists
- [ ] Returns `400` with a descriptive error if any required field is missing or email format is invalid
- [ ] Unit tests exist for all three response cases (201, 409, 400)
- [ ] Tests pass: `./build/tests/core-api-tests` exits 0
**Status**: backlog

---

## TASK-007: Login endpoint + JWT issuance
**Branch**: `feat/TASK-007-login-jwt`
**Service**: core-api
**Depends on**: TASK-006
**Acceptance criteria**:
- [ ] `POST /api/v1/auth/login` accepts `{"email": "...", "password": "..."}`
- [ ] Validates password against bcrypt hash
- [ ] On success: issues a short-lived JWT access token (15min expiry, includes `user_id` and `role` claims) returned in the response body
- [ ] On success: issues a long-lived refresh token (opaque random string, 30d expiry), stored hashed in Redis, set as an HttpOnly cookie named `refresh_token`
- [ ] Returns `200` with `{"access_token": "..."}` on success
- [ ] Returns `401` with `{"error": "invalid credentials", "code": "UNAUTHORIZED"}` on wrong email or password (same message for both — no user enumeration)
- [ ] JWT secret loaded from env var `JWT_SECRET` (not hardcoded)
- [ ] Unit tests for 200 (happy path) and 401 (bad password)
- [ ] Tests pass: `./build/tests/core-api-tests` exits 0
**Status**: backlog

---

## TASK-008: JWT middleware + token refresh endpoint
**Branch**: `feat/TASK-008-jwt-middleware`
**Service**: core-api
**Depends on**: TASK-007
**Acceptance criteria**:
- [ ] A Drogon middleware `JwtMiddleware` exists that validates the `Authorization: Bearer <token>` header on protected routes
- [ ] Middleware returns `401` if the header is missing, the token is expired, or the signature is invalid
- [ ] `POST /api/v1/auth/refresh` reads the `refresh_token` HttpOnly cookie, validates it against Redis
- [ ] On valid refresh: revokes the old token in Redis, issues a new access token + new refresh token (rotation)
- [ ] On reuse detection (token already revoked): revokes all refresh tokens for that user in Redis, returns `401`
- [ ] `GET /api/v1/auth/me` is a protected route (uses the middleware) that returns `{"id": "...", "email": "...", "role": "..."}` for the authenticated user
- [ ] Unit tests: middleware with valid token, expired token, missing token
- [ ] Unit tests: refresh with valid token, reused token
- [ ] Tests pass: `./build/tests/core-api-tests` exits 0
**Status**: backlog
