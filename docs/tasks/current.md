# Current Task

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
**Status**: in-progress
**Started**: 2026-05-18
**QA verdict**: (pending)

### Notes
First task — no code, just structure. Goal is a clean repo skeleton that matches the monorepo layout defined in architecture docs.
