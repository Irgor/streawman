# 015 — C++ Unit Testing Framework

**Date**: 2026-05-18
**Status**: Accepted
**Origin**: AI presented the tradeoffs between Catch2 and Google Test. Developer chose Google Test for its built-in mocking (gmock) and higher CV recognition.

## Context

All three C++ services (core-api, uploader-api, watcher-api) need a unit testing framework. The choice must integrate with CMake + Ninja (ADR 012) and produce JUnit-compatible XML output for GitHub Actions CI reporting.

## Options Considered

**Catch2**
- Lightweight, expressive natural-language syntax
- Easy CMake integration via `FetchContent`
- BDD-style sections (`GIVEN/WHEN/THEN`) good for documenting intent
- No built-in mocking — requires a separate library (trompeloeil)
- JUnit XML output via command-line flag

**Google Test (gtest + gmock)**
- Most widely used C++ test framework; highest CV recognition
- Built-in mocking via `gmock` — mock FFmpeg calls, Redis client, PostgreSQL queries
- JUnit-compatible XML output natively — works with GitHub Actions test reporters
- More verbose syntax but explicit and well-documented
- CMake integration via `FetchContent` (same as Catch2)

## Decision

**Google Test + gmock**

The built-in mocking story is the deciding factor. Testing `uploader-api`'s transcoding pipeline requires mocking the FFmpeg subprocess call and the Redis client. Testing `core-api`'s auth logic requires mocking the PostgreSQL client. gmock handles all of this without a separate dependency.

## CMake Integration

```cmake
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

target_link_libraries(${SERVICE_NAME}_tests gtest_main gmock)
```

## Test File Convention

```
services/{service-name}/
└── tests/
    ├── unit/
    │   ├── auth_test.cpp        ← core-api: JWT validation logic
    │   ├── transcoder_test.cpp  ← uploader-api: FFmpeg call mocking
    │   └── chunk_server_test.cpp ← watcher-api: file path resolution
    └── integration/
        └── [FILL: add when services are running end-to-end]
```

## Example Pattern (write tests yourself)

```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Mock the FFmpeg subprocess call
class MockFFmpeg : public IFFmpegRunner {
public:
    MOCK_METHOD(int, run, (const std::string& cmd), (override));
};

TEST(TranscoderTest, ReturnsErrorOnMissingFile) {
    MockFFmpeg mock;
    EXPECT_CALL(mock, run).Times(0);  // FFmpeg should never be called

    Transcoder t(&mock);
    EXPECT_EQ(t.transcode("missing.mp4"), TranscodeResult::FileNotFound);
}
```

## CI Integration

Google Test produces JUnit XML with `--gtest_output=xml:report.xml`. GitHub Actions can publish this as a test report using `dorny/test-reporter` action.

## Consequences

- `FetchContent_Declare(googletest ...)` added to each service's `CMakeLists.txt`
- Tests live in `tests/unit/` per service; separate from `src/`
- Interface classes (`IFFmpegRunner`, `IRedisClient`, etc.) must be defined to enable mocking — good practice regardless
- Integration tests added later when services are wired together
