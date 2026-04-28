# bbb.nozzle

Max/MSP externals for GPU texture sharing via [nozzle](https://github.com/2bbb/nozzle) — a cross-platform alternative to Syphon (macOS) and Spout (Windows).

Shares textures between processes on the same machine. Built as universal binary externals (x86_64 + arm64) using the [min-api](https://github.com/Cycling74/min-api).

## Externals

### bbb.nozzle.send

Publishes GPU textures to named shared streams.

```
[bang]  ──►  bbb.nozzle.send  ──  (publishes frame on bang)
[list w h]   @name "myStream"
              @width 640
              @height 480
```

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | symbol | `"nozzle_sender"` | Sender name for discovery |
| `width` | int | `640` | Texture width |
| `height` | int | `480` | Texture height |

| Message | Description |
|---------|-------------|
| `bang` | Publish one frame |
| `list w h` | Set dimensions and reinitialize |
| `dump` | Print current status to console |

### bbb.nozzle.receive

Receives GPU textures from a named sender. Outputs frame info on the left outlet, connection events on the right outlet.

```
[bang]  ──►  bbb.nozzle.receive  ──►  [width height frame_index ...]
              @name "myStream"    ──►  [sender info events]
              @timeout 0
```

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | symbol | `"nozzle_sender"` | Sender name to connect to |
| `timeout` | int | `0` | Frame acquisition timeout in ms (0 = non-blocking) |

| Message | Description |
|---------|-------------|
| `bang` | Poll for new frame. Outputs `width height frame_index` on left outlet if available |
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
- [nozzle](https://github.com/2bbb/nozzle) — Prebuilt static library at `deps/nozzle/`

## Architecture

The externals use nozzle's C ABI (`nozzle_c.h`) to avoid exception/RTTI conflicts with Max's runtime. The sender creates a named shared texture stream, the receiver connects by name and polls for frames via bang-driven polling.

```
bbb.nozzle.send:   nozzle_sender_create → acquire_writable_frame → commit_frame (on bang)
bbb.nozzle.receive: nozzle_receiver_create → acquire_frame → output dimensions (on bang)
```

## Installation

Copy `externals/bbb.nozzle.send.mxo` and `externals/bbb.nozzle.receive.mxo` to your Max packages folder, or place alongside your Max patcher.

## License

MIT
