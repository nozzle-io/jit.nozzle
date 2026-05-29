# jit.nozzle

> This codebase is currently in its AI-slob prototyping phase: the code runs on momentum, vibes, and plausible intent.
> Proper debugging will be introduced once demand graduates from hypothetical to measurable.

Max/MSP externals for inter-process matrix sharing via [nozzle](https://github.com/nozzle-io/nozzle) — a cross-platform alternative to Syphon (macOS) and Spout (Windows).

Shares jit.matrix and OpenGL texture data between processes on the same machine. Built as universal binary externals (x86_64 + arm64) using the [min-api](https://github.com/Cycling74/min-api).

## Disclaimer / Notice

This library is currently a work in progress and contains many incomplete features and unverified implementations.
Although it may appear usable at first glance, it may not function correctly.

Please use it with the understanding that no guarantees are made regarding its behavior, and perform debugging, validation, and review as needed.
If you encounter problems, please do not become angry; instead, contributions in the form of Issues or Pull Requests would be greatly appreciated.

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

## Matrix Format Support

### jit.nozzle.send (Sender)

Jitter matrix type and planecount are mapped to nozzle format as follows:

| Matrix Type | Planes | Nozzle Format |
|-------------|--------|---------------|
| char | 1 | R8 UNORM |
| char | 2 | RG8 UNORM |
| char | 3,4 | RGBA8 UNORM (3-plane padded to 4) |
| float32 | 1 | R32 Float |
| float32 | 2 | RG32 Float |
| float32 | 3 | RGB32 Float requested semantic format; storage may fall back to RGBA32 Float with alpha fill |
| float32 | 4 | RGBA32 Float |
| long | 1 | R32 Uint |
| long | 2 | RGBA32 Uint (2-plane expanded to 4) |
| long | 3 | RGB32 Uint requested semantic format; storage may fall back to RGBA32 Uint with alpha fill |
| long | 4 | RGBA32 Uint |

`jit.nozzle.send` matrix input publishes Jitter `float32` matrices as full-precision 32-bit float nozzle formats. It does not emit R16/RG16/RGBA16_FLOAT from `jit.matrix float32` sources, and there is currently no `@half`, `@float16`, or `@output_format` attribute for matrix send down-conversion. Receive-side 16F widening is a separate policy and does not imply send-side half output. Any future matrix half-output mode must perform explicit numeric float32-to-float16 conversion, define precision-loss/rounding behavior, and must not raw-copy float32 memory into 16F storage.

Note: long type uses nozzle uint formats (nozzle has no RG32 Uint, so 2-plane is expanded to RGBA32 Uint). 16-bit unorm/float publishing from `jit.nozzle.send` matrix input is not supported; this limitation does not apply to `jit.gl.nozzle.send`, which can publish native 16F GL textures.

### jit.nozzle.receive (Receiver)

Nozzle format is mapped back to Jitter matrix type as follows:

| Nozzle Format | Matrix Type | Planes | Notes |
|---------------|-------------|--------|-------|
| R8/RG8/RGBA8/BGRA8/RGBA8_SRGB/BGRA8_SRGB UNORM | char | 1-4 | BGRA formats swizzled to RGBA |
| R32/RG32/RGBA32 Float | float32 | 1-4 | Direct mapping |
| R16/RG16/RGBA16 Float | float32 | 1-4 | Half to float expansion |
| R16/RG16/RGBA16 UNORM | long | 1-4 | 16-bit integer |
| R32/RGBA32 Uint | long | 1-4 | 32-bit integer |

### jit.gl.nozzle.send/receive

GL externals automatically detect and preserve the source texture's format.

**Sender**: queries the GL texture's internal format via `glGetTexLevelParameteriv(GL_TEXTURE_INTERNAL_FORMAT)` and maps only explicitly supported internal formats to nozzle formats. Supported sender internal formats are R8/RG8/RGB8/RGBA8, BGRA8_EXT, SRGB8_ALPHA8, R16/RG16/RGB16/RGBA16, R16F/RG16F/RGB16F/RGBA16F, R32F/RG32F/RGB32F/RGBA32F, R32UI/RGB32UI/RGBA32UI, and DEPTH_COMPONENT32F. Unsupported or unknown internal formats are rejected; the sender does not guess RGBA8_UNORM.

**Receiver**: reads the frame's nozzle format from `NozzleFrameInfo.format` and passes an explicit copy format to `nozzle_frame_copy_to_gl_texture`. Supported receive copy formats are explicitly mapped; unsupported or unknown nozzle formats are rejected rather than coerced to RGBA8_UNORM.

| Frame Format | Copy Format | Notes |
|---|---|---|
| R8/RG8/RGB8/RGBA8/BGRA8 unorm | Same format | Direct |
| R16/RG16/RGB16/RGBA16 float | Same format | Direct |
| R32/RG32/RGB32/RGBA32 float | Same format | Direct |
| R16/RG16/RGB16/RGBA16 unorm | Same format | Direct |
| R32/RGB32/RGBA32 uint | Same format | Direct |
| RGBA8_SRGB/BGRA8_SRGB | Corresponding unorm | Copy-compatible GL target policy |
| DEPTH32_FLOAT | R32_FLOAT | Copy-compatible GL target only; not a semantic conversion from depth to color |

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
jit.gl.nozzle.send:   jit_gl_texture → get GL name → query internal format → nozzle_sender_publish_gl_texture
jit.gl.nozzle.receive: acquire_frame → get frame format → nozzle_frame_copy_to_gl_texture → output jit_gl_texture
```

## Installation

Copy the `externals/` and `help/` folders into your Max packages directory, or download the latest release from [GitHub Releases](https://github.com/nozzle-io/jit.nozzle/releases).

## License

MIT

Third-party dependencies:

- [nozzle](https://github.com/nozzle-io/nozzle) — MIT
- [min-api](https://github.com/Cycling74/min-api) — MIT
