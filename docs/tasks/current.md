# Current Task

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
**Status**: done
**Started**: 2026-05-18
**QA verdict**: READY — all three containers (postgres, redis, pgbouncer) start healthy. Ports correct, named volume persists data, PgBouncer waits for Postgres healthcheck.

### Notes
First task — no code, just structure. Goal is a clean repo skeleton that matches the monorepo layout defined in architecture docs.
