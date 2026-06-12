# Edge Pulse Effect

OpenGL 3.3 real-time visual effects framework with notification-style edge pulse, magnetic waves, and magnetic rectangle effects.

## Project Structure

```
├── CMakeLists.txt
├── assets/shaders/
│   ├── line.vert / line.frag         # Active — used by all effects
│   ├── particle.vert / particle.frag # Reserved — not currently used
├── external/
│   ├── src/glad.c                    # OpenGL loader
│   ├── src/glfw/                     # GLFW windowing (built from source)
│   └── include/                      # glad, GLFW, GLM, stb_image
├── include/EdgeLightingEffect/
│   ├── Effect.h                      # Abstract base class
│   ├── EffectManager.h               # Effect lifecycle manager
│   ├── EdgePulseEffect.h             # Notification edge pulse
│   ├── MagneticWaveEffect.h          # Horizontal sine waves
│   └── MagneticRectangleEffect.h     # Displaced perimeter wave
└── src/
    ├── main.cpp                      # Demo entry point
    ├── EffectManager.cpp
    ├── core/Shader.h / Shader.cpp    # OpenGL shader wrapper
    ├── effects/
    │   ├── EdgePulseEffect.cpp
    │   ├── MagneticWaveEffect.cpp
    │   └── MagneticRectangleEffect.cpp
    └── perimeter/
        └── Perimeter.h / Perimeter.cpp  # Rounded-rectangle path
```

## Dependencies

- **CMake** >= 3.10, **C++11**
- Vendored: GLFW 3, GLAD (OpenGL 3.3 core), GLM, stb_image
- System: `opengl32.lib` (Windows), OpenGL.framework (macOS), libGL.so (Linux)

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

The demo copies `assets/` to the output directory automatically.

## Targets

| Target | Type | Description |
|---|---|---|
| `EdgePulseEffectLib` | Static library | All effects + manager + shader + perimeter |
| `EdgePulseEffectDemo` | Executable | Demo app (press ←/→ to switch modes) |

---

## Effect API Reference

### Effect (abstract base)

| Method | Description |
|---|---|
| `init(fbWidth, fbHeight)` | One-time setup, allocate GPU resources. Return `false` on failure. |
| `update(deltaTime)` | Advance simulation by `deltaTime` seconds, rebuild geometry. |
| `render()` | Issue OpenGL draw calls. |
| `onResize(fbWidth, fbHeight)` | Optional; react to framebuffer resize. |

### EdgePulseEffect

Notification-style edge pulse: a thin coloured core line tracing a rounded rectangle, with a wider glow overlay that lights up on notification.

| Method | Description |
|---|---|
| `setCornerRadius(radius)` | Corner radius of the rounded rectangle (default 40). |
| `setCoreWidth(width)` | Base core line width in pixels (default 10). |
| `triggerNotification()` | Manually trigger notification: intensifies glow overlay, screen shake, and colour flash. |

**Visual behaviour:**
- Core line: thin (10px), standard alpha blending (`GL_ONE_MINUS_SRC_ALPHA`), gradient colour cycling cyan→blue→purple→cyan.
- Glow overlay: wider (30px), additive blending (`GL_ONE`), appears on top of core line only during notification.
- Notification: pulse intensity decays at 1.2/s, shake decays at 4/s.
- Normal direction at every perimeter point points radially outward from rectangle center for uniform glow width.

### MagneticWaveEffect

Multiple horizontal sine-wave lines with additive blending.

| Method | Description |
|---|---|
| `setSpeed(speed)` | Wave propagation speed (default 1.5). |
| `setAmplitude(amp)` | Displacement amplitude in pixels (default 30). |
| `setWaveCount(count)` | Number of horizontal wave lines (default 6). |
| `setLineWidth(width)` | Vertical thickness per wave line (default 6). |
| `setColorShift(shift)` | Hue offset [0–1] to stagger colours (default 0). |

### MagneticRectangleEffect

A rounded rectangle whose perimeter points are displaced outward/inward by a composite sine wave.

| Method | Description |
|---|---|
| `setSpeed(speed)` | Wave speed (default 1.2). |
| `setAmplitude(amp)` | Displacement amplitude in pixels (default 15). |
| `setLineWidth(width)` | Ribbon width in pixels (default 4). |
| `setCornerRadius(radius)` | Corner radius (default 40). |
| `setColorShift(shift)` | Hue offset [0–1] for perimeter gradient (default 0). |
| `setScale(scale)` | Scale factor relative to framebuffer (default 0.7). |

### EffectManager

Non-owning aggregator for `Effect` instances.

| Method | Description |
|---|---|
| `addEffect(effect)` | Register an effect. |
| `removeEffect(effect)` | Unregister first match. |
| `clearEffects()` | Unregister all. |
| `init(fbWidth, fbHeight)` | Init all registered effects. |
| `update(deltaTime)` | Update all registered effects. |
| `render()` | Render all registered effects (in registration order). |
| `onResize(fbWidth, fbHeight)` | Resize all registered effects. |

## Geometry System

`Perimeter` models a rounded rectangle as 8 segments (4 straights + 4 quarter-circle arcs). All geometry is rebuilt on CPU each frame in `update()` and uploaded to the GPU via `glBufferData(GL_DYNAMIC_DRAW)`.

## Shader: line.vert / line.frag

Shared by all effects:

- **Vertex**: passes position (aPos), UV (aUV where `.y = ±1` across strip width), and per-vertex colour (aColor).
- **Fragment**: uses `abs(vUV.y)` with `smoothstep(0, 1, dist)` for a soft edge falloff, multiplied by `uAlphaScale` uniform.
