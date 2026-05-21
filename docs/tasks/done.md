# Done

Completed tasks with QA verdicts. Newest at top.

---

## TASK-004: core-api health check endpoint
**Merged**: 2026-05-21
**PR**: (pending merge)
**QA verdict**: READY — `GET /health` returns 200 with correct JSON body, port configurable via `PORT` env var (defaults to 8080), gtest unit test passes against the controller directly.

## TASK-003: core-api CMake + Ninja build setup
**Merged**: 2026-05-18
**PR**: (pending merge)
**QA verdict**: READY — CMakeLists.txt correct, Drogon fetched via CPM, zero warnings, `./build/core-api` starts and responds with drogon/1.9.10 on port 8080.

## TASK-002: Docker Compose dev environment
**Merged**: 2026-05-18
**PR**: (pending merge)
**QA verdict**: READY — all three containers start healthy, ports correct, named volume persists Postgres data, PgBouncer waits for Postgres healthcheck, `.env.example` documents all required vars.

## TASK-001: Initialize monorepo directory structure
**Merged**: 2026-05-18
**PR**: (first commit, no PR)
**QA verdict**: READY — all directories created, `.gitignore` and `README.md` in place, service dirs tracked with `.gitkeep`.
