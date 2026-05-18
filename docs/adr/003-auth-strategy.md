# 003 — Authentication and Authorization Strategy

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: Developer identified the need for role-based tokens (viewer vs uploader) and raised the JWT theft concern proactively. AI introduced the dual-token + rotation pattern in response. Developer chose this over simpler session-only auth after understanding the security tradeoffs.

## Context

The system has two user roles:
- **viewer** — can browse and stream content
- **uploader** — can do everything a viewer can, plus upload new content

Security requirements: token theft must have a limited blast radius. Every request to both APIs must be authenticated without introducing a per-request database call.

## Options Considered

**Single long-lived JWT**
- Simple to implement
- If stolen, attacker has access until expiry (could be hours or days)
- No revocation mechanism without a blocklist (which reintroduces statefulness)

**Session cookies (server-side sessions in Redis)**
- Stateful — server can revoke immediately
- Requires Redis lookup on every request
- CSRF risk if not handled carefully
- Logout is clean and immediate

**OAuth 2.0 (external identity provider)**
- Delegates identity to Google/GitHub etc.
- Adds external dependency; overkill for a personal-use system
- Useful if public user registration is ever added

**Dual-token JWT with refresh rotation**
- Short-lived access token (15 min): stateless JWT, validated by signature alone
- Long-lived refresh token (30 days): opaque, stored in Redis with TTL
- Access token stored in memory only (not localStorage, not a cookie)
- Refresh token stored in HttpOnly + Secure + SameSite=Strict cookie
- Refresh token rotation on every use: old token invalidated, new token issued
- Reuse detection: if an already-used refresh token is presented, all tokens for that user are revoked immediately

## Decision

**Dual-token JWT with refresh rotation**

This pattern gives stateless per-request validation (no Redis call per request) while preserving revocability via refresh token control. The short access token window (15 min) limits the theft window. The rotation + reuse detection strategy handles stolen refresh tokens. This is the pattern used by production streaming and SaaS platforms.

## Implementation Notes

- JWT payload: `{ sub: userId, role: "viewer"|"uploader", iat, exp }`
- JWT secret stored in environment variable `JWT_SECRET` — never in code
- Refresh token stored in Redis: key `refresh:{sha256(token)}`, value `{ userId, role, usedAt: null }`
- On reuse detection: set `revoked: true` on all refresh tokens for `userId`, return 401
- Auth filter in Drogon validates JWT signature and expiry on every protected route
- Role check is performed in the handler after the filter passes

## Consequences

- streawman-ui must implement a silent refresh mechanism (refresh the access token before it expires)
- No persistent login across browser restarts unless refresh cookie survives (it will, via cookie TTL)
- Logout = delete refresh token from Redis + clear cookie
- Forced logout of all sessions = delete all `refresh:{hash}` keys for a userId in Redis
