# Current Task

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
**Status**: in-progress
**Started**: 2026-05-18
**QA verdict**: (pending)

### Notes
First real endpoint. Goal: learn how Drogon routes requests to handlers. Also sets up the gtest framework for all future tests.
