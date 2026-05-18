# 005 — Load and Stress Testing

**Date**: 2026-05-17
**Status**: Accepted
**Origin**: Developer explicitly wanted automated load and stress test reports as portfolio evidence of production-readiness. AI recommended k6 over alternatives (JMeter, Locust, Artillery) based on CI integration and threshold-based pass/fail reports.

## Context

The project must demonstrate it is prepared for real load, even if the actual user count is small. Load and stress test reports serve as quantitative portfolio evidence that the system was designed and validated for production conditions.

## Options Considered

**wrk / wrk2**
- Ultra-minimal C-based HTTP benchmarking
- No scripting, no scenarios, no reports — output is just req/s and latency percentiles
- Good for raw baseline numbers, not for complex scenarios

**Apache JMeter**
- Java-based, GUI-driven, legacy enterprise tooling
- XML test definitions — difficult to version control cleanly
- Cumbersome CI/CD integration

**Locust**
- Python-based, good for complex user behavior simulation
- Readable code, but slower than k6 under high concurrency

**Artillery**
- Node.js-based, YAML scenario definitions
- Good CI integration but less widely adopted than k6

**k6**
- JavaScript test scripts — familiar syntax for a Node.js-experienced developer
- Native CI/CD integration (GitHub Actions plugin, thresholds as pass/fail)
- Generates JSON reports; Grafana dashboard integration available
- Scenarios support: constant load, ramping VUs, stress, spike, soak
- `thresholds` block makes tests binary (pass/fail) like unit tests

## Decision

**k6**

The combination of JavaScript syntax, CI-native threshold-based pass/fail, and Grafana integration makes k6 reports the strongest portfolio artifact. A committed test suite with green threshold reports in CI is direct evidence of production-readiness thinking.

## Test Structure

```
tests/load/
├── scenarios/
│   ├── baseline.js         ← normal expected load
│   ├── stress.js           ← ramp to breaking point
│   ├── spike.js            ← sudden traffic burst
│   └── soak.js             ← sustained load over hours
├── utils/
│   └── auth.js             ← shared JWT acquisition helper
└── thresholds.js           ← shared threshold definitions
```

## Key Thresholds to Define (fill as you benchmark)

```javascript
thresholds: {
  http_req_duration: ['p(99)<200'],   // 99% of requests under 200ms
  http_req_failed:   ['rate<0.01'],   // less than 1% error rate
  // [FILL: chunk delivery specific thresholds for watcher-api]
}
```

## Consequences

- k6 must be installed in CI environment (Docker image or GitHub Actions runner)
- Tests run against a staging environment — never production
- Reports committed to the repo or published as CI artifacts
- [FILL: decide if k6 Cloud is used for distributed load generation, or local runner is sufficient]
