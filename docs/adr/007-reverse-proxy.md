# 007 — Reverse Proxy

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: Developer chose Nginx over Caddy explicitly for CV impact, aware that Caddy would be simpler operationally. AI presented both options honestly; developer made the call with full understanding of the tradeoff.

## Context

All services (streawman-ui, core-api, uploader-api, watcher-api) run in Docker containers on the same host. A reverse proxy must sit in front, handle TLS termination, and route incoming requests to the correct service. Only ports 80 and 443 are exposed publicly.

## Options Considered

**Nginx**
- Industry standard since 2004; ~34% of all web servers globally
- Extremely battle-tested with well-documented configuration
- High CV recognition — widely expected in production infrastructure roles
- TLS: requires Certbot + cron job for Let's Encrypt cert renewal (standard practice)
- Config is verbose but explicit and highly controllable
- Strong support for serving static files and large payloads (relevant for `.ts` segment delivery)

**Caddy**
- Modern reverse proxy written in Go (2015)
- Automatic HTTPS out of the box — zero-config Let's Encrypt issuance and renewal
- Much simpler Caddyfile syntax
- Excellent fit for self-hosted and homelab environments
- Growing CV recognition but less universal than Nginx

**Traefik**
- Docker-native, auto-discovers services via container labels
- Good for dynamic microservice environments
- More operational complexity; less relevant for a static service layout like this project

## Decision

**Nginx**

The primary driver is CV impact — Nginx is the name interviewers and reviewers expect to see in a production infra stack. The Certbot TLS setup, while more manual than Caddy, is also a well-understood skill to demonstrate. The operational overhead is low and well-documented.

## Routing Rules

| Path prefix | Backend | Port |
|---|---|---|
| `/api/*` | core-api | 8080 |
| `/ws` | core-api | 8080 |
| `/upload` | uploader-api | 8081 |
| `/stream` | watcher-api | 8082 |
| `/` | streawman-ui | 3000 |

Nginx matches most-specific prefix first — no conflicts between routes.

## Config Template (write this yourself during infra setup)

```nginx
# HTTP → HTTPS redirect
server {
    listen 80;
    server_name streawman.codigor.dev;
    return 301 https://$host$request_uri;
}

server {
    listen 443 ssl;
    server_name streawman.codigor.dev;

    ssl_certificate     /etc/letsencrypt/live/streawman.codigor.dev/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/streawman.codigor.dev/privkey.pem;

    # Max video upload size (VPS: 40GB disk; home server: ~2TB)
    client_max_body_size 10G;

    # core-api — auth, users, video metadata, search, favorites
    location /api/ {
        proxy_pass http://core-api:8080;
        proxy_set_header Host              $host;
        proxy_set_header X-Real-IP         $remote_addr;
        proxy_set_header X-Forwarded-For   $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    # core-api — WebSocket notifications
    location /ws {
        proxy_pass         http://core-api:8080;
        proxy_http_version 1.1;
        proxy_set_header   Upgrade    $http_upgrade;
        proxy_set_header   Connection "upgrade";
        proxy_read_timeout 3600s;
    }

    # uploader-api — stream upload body directly, no nginx buffering
    location /upload {
        proxy_pass              http://uploader-api:8081;
        proxy_request_buffering off;
        proxy_read_timeout      300s;
        proxy_send_timeout      300s;
        proxy_set_header Host              $host;
        proxy_set_header X-Real-IP         $remote_addr;
        proxy_set_header X-Forwarded-For   $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    # watcher-api — HLS: no response buffering, cache immutable segments
    location /stream {
        proxy_pass      http://watcher-api:8082;
        proxy_buffering off;
        proxy_set_header Host      $host;
        proxy_set_header X-Real-IP $remote_addr;
        add_header Cache-Control "public, max-age=86400";
    }

    # streawman-ui — catch all
    location / {
        proxy_pass http://streawman-ui:3000;
        proxy_set_header Host              $host;
        proxy_set_header X-Real-IP         $remote_addr;
        proxy_set_header X-Forwarded-For   $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

## Consequences

- Certbot installed on the host (not in a container); cron job auto-renews Let's Encrypt cert
- All inter-service traffic stays on the Docker bridge network — only ports 80/443 exposed publicly
- `proxy_request_buffering off` on `/upload` — upload bytes flow directly to `uploader-api`; progress tracking is real
- `proxy_buffering off` on `/stream` — `.ts` segments flow directly to the browser; no added latency per chunk
- `client_max_body_size 10G` — sufficient for VPS dev (40GB disk) and home server (2TB); test files during dev will be <10MB
- WebSocket at `/ws` requires `proxy_http_version 1.1` and the `Upgrade` header — standard WS proxy pattern
- `.m3u8` playlist files are served under `/stream/` and inherit the same `Cache-Control` — acceptable since VOD playlists are also immutable after transcoding
