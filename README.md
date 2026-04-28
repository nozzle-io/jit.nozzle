# bbb.nozzle

Max/MSP externals for inter-process matrix sharing via [nozzle](https://github.com/2bbb/nozzle) — a cross-platform alternative to Syphon (macOS) and Spout (Windows).

Shares jit.matrix pixel data between processes on the same machine. Built as universal binary externals (x86_64 + arm64) using the [min-api](https://github.com/Cycling74/min-api).

## Externals

### jit.bbb.nozzle.send

Accepts jit.matrix input and publishes pixel data to named shared streams.

```
[jit.matrix]  ──►  jit.bbb.nozzle.send  ──►  [width height frame_index]
                 @name "myStream"
```

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | symbol | `"nozzle_sender"` | Sender name for discovery |

| Message | Description |
|---------|-------------|
| `jit_matrix` | Receive a jit.matrix and publish its pixel data |
| `dump` | Print current status to console |

### jit.bbb.nozzle.receive

Receives matrix data from a named sender. Outputs jit.matrix on the left outlet, connection events on the right outlet.

```
[bang]  ──►  jit.bbb.nozzle.receive  ──►  [jit_matrix output]
              @name "myStream"         ──►  [sender info events]
              @timeout 0
```

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | symbol | `"nozzle_sender"` | Sender name to connect to |
| `timeout` | int | `0` | Frame acquisition timeout in ms (0 = non-blocking) |

| Message | Description |
|---------|-------------|
| `bang` | Poll for new frame. Outputs jit_matrix on left outlet if available |
| `connect` | Reconnect to sender |
| `info` | Print connected sender info to console |

## Build

```bash
git clone --recursive https://github.com/2bbb/bbb.nozzle.git
cd bbb.nozzle
cmake -B build
cmake --build build
```

Built externals appear in `externals/` as `.mxo` bundles.

### Requirements

- CMake 3.19+
- C++17 compiler
- macOS 12.0+ (Metal/IOSurface frameworks)
- Max 8.0+ (for loading the externals)

### Dependencies (git submodules)

- [min-api](https://github.com/Cycling74/min-api) — Max external development kit
- [max-sdk-base](https://github.com/Cycling74/max-sdk-base) — Max SDK headers and resources
- [nozzle](https://github.com/2bbb/nozzle) — Shared texture library at `deps/nozzle/`

## Architecture

The externals use nozzle's C ABI (`nozzle_c.h`) to avoid exception/RTTI conflicts with Max's runtime. The sender accepts jit.matrix input and copies pixel data to an IOSurface-backed shared texture. The receiver polls for frames and outputs the data as jit.matrix.

```
jit.bbb.nozzle.send:   jit_matrix → lock_pixels → memcpy to IOSurface → commit_frame
jit.bbb.nozzle.receive: acquire_frame → lock_pixels → memcpy to jit.matrix → output
```

## Installation

Copy `externals/jit.bbb.nozzle.send.mxo` and `externals/jit.bbb.nozzle.receive.mxo` to your Max packages folder, or place alongside your Max patcher.

## License

MIT
