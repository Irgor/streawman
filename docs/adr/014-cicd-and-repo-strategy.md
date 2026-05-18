# 014 — CI/CD Platform and Repository Strategy

**Date**: 2026-05-18
**Status**: Accepted
**Origin**: Developer identified the monorepo question independently while thinking about CI/CD. Solo developer context made monorepo the obvious conclusion. GitHub Actions chosen as the industry-standard CI/CD platform with native GitHub integration.

## Context

The project has four deployable units (streawman-ui, core-api, uploader-api, watcher-api) plus shared infra config, docs, and ADRs. A repository strategy and CI/CD platform must be chosen before setting up the pipeline.

## Repository Strategy

### Options Considered

**Polyrepo** — one Git repository per service
- Independent versioning and deployment cycles
- Natural fit for large teams with separate ownership
- ADRs, system design docs, and Cursor rules don't belong to any single service — would require a 5th repo or duplication
- Coordinating cross-service changes requires multiple commits across multiple repos
- No benefit for a solo developer

**Monorepo** — all services, infra, and docs in one repository
- One portfolio link shows the full system
- Atomic cross-service commits (API contract change + consumer update in one commit)
- ADRs, system design docs, `.cursor/rules/`, `AGENTS.md` live naturally at the root
- CI/CD path filters prevent rebuilding all services on every change
- The only relevant monorepo concern (team ownership) does not apply to a solo developer

### Decision: Monorepo

## CI/CD Platform

### Options Considered

**GitHub Actions**
- YAML workflow files in `.github/workflows/`, versioned with the code
- Most widely used CI/CD platform in the industry — high CV recognition
- Free: unlimited minutes for public repos; 2,000 min/month for private repos
- Official k6 action available — load test reports integrate directly
- Path-based triggers prevent unnecessary builds in a monorepo
- Runner choice: GitHub-hosted (zero setup) or self-hosted on VPS (no minute limits)

**Drone CI / Woodpecker CI** — self-hosted, lightweight
- Less CV recognition; extra maintenance burden on VPS

**Jenkins** — heavy JVM process, XML config, outdated tooling. Eliminated.

**GitLab CI** — requires GitLab. Not applicable.

### Decision: GitHub Actions with GitHub-hosted runners (public repo = unlimited free minutes)

The repository is public for portfolio purposes. **Public repos get unlimited free CI minutes on GitHub-hosted runners** — the 2,000 min/month limit applies only to private repos. Self-hosted runners are therefore unnecessary and actively undesirable for a public repo: a malicious actor could fork the repo, open a PR, and if a self-hosted runner triggers on PRs, their code executes on the VPS. GitHub explicitly warns against this pattern. GitHub-hosted runners are the correct and safe choice.

## Workflow Structure

One workflow file per service, triggered only when that service's files change:

```
.github/
└── workflows/
    ├── ci-streawman-ui.yml     ← triggers on apps/streawman-ui/**
    ├── ci-core-api.yml         ← triggers on services/core-api/**
    ├── ci-uploader-api.yml     ← triggers on services/uploader-api/**
    ├── ci-watcher-api.yml      ← triggers on services/watcher-api/**
    ├── ci-infra.yml            ← triggers on infra/**
    └── deploy.yml              ← triggers on push to main — deploys to VPS via SSH
```

## Pipeline Stages (per service)

```
1. Lint / type-check
   ├── C++ services: clang-tidy or cppcheck
   └── streawman-ui: ESLint + TypeScript compiler

2. Build
   ├── C++ services: cmake -G Ninja + ninja
   └── streawman-ui: next build

3. Test
   ├── C++ services: unit tests (Catch2 or Google Test — ADR pending)
   └── streawman-ui: Jest / Vitest

4. Docker build
   └── Build and tag Docker image for the changed service

5. Deploy (main branch only)
   └── SSH into VPS → docker compose pull && docker compose up -d
```

## Deployment Flow

```
Developer pushes to main on MacBook
    → GitHub Actions triggers
    → lint → build → test
    → docker build (on GitHub-hosted runner)
    → docker push → GitHub Container Registry (GHCR, free for public repos)
    → SSH into Hetzner VPS (using VPS_SSH_KEY secret)
    → docker compose pull (VPS fetches new images from GHCR)
    → docker compose up -d (restarts affected containers)
```

Docker images are built on GitHub's machines (not the VPS), versioned as immutable artifacts in GHCR. The VPS only pulls and restarts — no build CPU used, rollback is possible by pulling a previous image tag.

## SSH Deploy Setup (done once)

1. Generate a dedicated CI SSH key pair on your machine
2. Add the public key to VPS `~/.ssh/authorized_keys`
3. Add the private key to GitHub Secrets as `VPS_SSH_KEY`
4. Add `VPS_HOST` (VPS IP) to GitHub Secrets

## Secrets

| Secret | Value |
|---|---|
| `VPS_SSH_KEY` | Private key for SSH deploy |
| `VPS_HOST` | VPS IP address (89.167.7.198) |
| `JWT_SECRET` | [FILL: strong random string, injected at deploy time] |
| `POSTGRES_PASSWORD` | [FILL: injected at deploy time] |

All stored in: GitHub repo → Settings → Secrets and variables → Actions

## Consequences

- `.github/workflows/` lives at the monorepo root
- Each workflow uses `paths:` filter — only runs when relevant files change
- Deploy workflow only triggers on push to `main` — PRs only run lint + build + test
- Docker images built on GitHub runners — VPS CPU is never used for builds
- GHCR provides image history — roll back by changing the image tag in docker-compose and re-running deploy
- Self-hosted runners must never be used on a public repo (security risk)
