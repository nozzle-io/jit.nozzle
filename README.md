# jit.nozzle

> This codebase is currently in its AI-slob prototyping phase: the code runs on momentum, vibes, and plausible intent.
> Proper debugging will be introduced once demand graduates from hypothetical to measurable.

Max/MSP externals for inter-process matrix sharing via [nozzle](https://github.com/nozzle-io/nozzle) — a cross-platform alternative to Syphon (macOS) and Spout (Windows).

Shares jit.matrix and OpenGL texture data between processes on the same machine. Built as universal binary externals (x86_64 + arm64) using the [min-api](https://github.com/Cycling74/min-api).

## Externals

### jit.nozzle.send

Accepts jit.matrix input and publishes pixel data to named shared streams.

```
[jit.matrix]  ──►  jit.nozzle.send  ──►  [width height frame_index]
                 @name "myStream"
```

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | symbol | `"nozzle_sender"` | Sender name for discovery |

| Message | Description |
|---------|-------------|
| `jit_matrix` | Receive a jit.matrix and publish its pixel data |
| `dump` | Print current status to console |

### jit.gl.nozzle.send

Accepts jit.gl.texture input (via `jit_gl_texture` message) and publishes the OpenGL texture to named shared streams.

```
[jit_gl_texture name]  ──►  jit.gl.nozzle.send  ──►  [width height frame_index]
                          @name "myStream"
```

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | symbol | `"nozzle_sender"` | Sender name for discovery |

| Message | Description |
|---------|-------------|
| `jit_gl_texture` | Receive a jit.gl.texture name and publish its GL texture |
| `bang` | Re-publish the last cached texture |
| `dump` | Print current status to console |

### jit.gl.nozzle.receive

Receives GL texture data from a named sender. Outputs `jit_gl_texture` on the left outlet, frame info on the right outlet.

```
[draw]  ──►  jit.gl.nozzle.receive  ──►  [jit_gl_texture output]
             @name "myStream"             ──►  [frame info events]
             @out_name "nozzle_recv_tex"
             @timeout 0
```

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | symbol | `"nozzle_sender"` | Sender name to connect to |
| `out_name` | symbol | `"nozzle_recv_tex"` | Name of the internal jit.gl.texture output to downstream objects |
| `timeout` | int | `0` | Frame acquisition timeout in ms (0 = non-blocking) |

| Message | Description |
|---------|-------------|
| `bang` | Poll for new frame (outputs frame info, no GL texture copy) |
| `draw` | Acquire frame, copy to internal GL texture, output `jit_gl_texture` (use in render context) |
| `connect` | Reconnect to sender |
| `info` | Print connected sender info to console |

## Build

```bash
git clone --recursive https://github.com/nozzle-io/jit.nozzle.git
cd jit.nozzle
cmake -B build
cmake --build build
```

Built externals appear in `externals/` as `.mxo` bundles (macOS) or `.mxe64` files (Windows).

### Requirements

- CMake 3.19+
- C++17 compiler
- macOS 12.0+ (Metal/IOSurface frameworks) or Windows 10+ (D3D11)
- Max 8.0+ (for loading the externals)

### Dependencies (git submodules)

- [min-api](https://github.com/Cycling74/min-api) — Max external development kit
- [max-sdk-base](https://github.com/Cycling74/max-sdk-base) — Max SDK headers and resources
- [nozzle](https://github.com/nozzle-io/nozzle) — Shared texture library at `deps/nozzle/`

## Architecture

The externals use nozzle's C ABI (`nozzle_c.h`) to avoid exception/RTTI conflicts with Max's runtime. The sender accepts jit.matrix input and copies pixel data to an IOSurface-backed shared texture. The receiver polls for frames and outputs the data as jit.matrix.

```
jit.nozzle.send:   jit_matrix → lock_pixels → memcpy to IOSurface → commit_frame
jit.nozzle.receive: acquire_frame → lock_pixels → memcpy to jit.matrix → output
jit.gl.nozzle.send:   jit_gl_texture → get GL name → nozzle_sender_publish_gl_texture
jit.gl.nozzle.receive: acquire_frame → nozzle_frame_copy_to_gl_texture → output jit_gl_texture
```

## Installation

Copy the `externals/` and `help/` folders into your Max packages directory, or download the latest release from [GitHub Releases](https://github.com/nozzle-io/jit.nozzle/releases).

## License

MIT

Third-party dependencies:

- [nozzle](https://github.com/nozzle-io/nozzle) — MIT
- [min-api](https://github.com/Cycling74/min-api) — MIT
