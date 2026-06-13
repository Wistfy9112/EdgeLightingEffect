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
│   ├── Effect.h                              # Abstract base class
│   ├── EffectManager.h                       # Effect lifecycle manager
│   ├── EdgePulseEffect.h                     # Notification edge pulse
│   ├── MagneticWaveEffect.h                  # Horizontal sine waves
│   ├── MagneticRectangleEffect.h             # Displaced perimeter wave
│   ├── NeonWaveRingEffect.h                  # Neon glowing wave ring
│   ├── AudioReactiveEdgeLightingEffect.h     # Audio-reactive edge lighting
│   └── EnergyStreamEffect.h                  # Futuristic energy stream
└── src/
    ├── main.cpp                              # Demo entry point
    ├── EffectManager.cpp
    ├── core/Shader.h / Shader.cpp            # OpenGL shader wrapper
    ├── effects/
    │   ├── EdgePulseEffect.cpp
    │   ├── MagneticWaveEffect.cpp
    │   ├── MagneticRectangleEffect.cpp
    │   ├── NeonWaveRingEffect.cpp
    │   ├── AudioReactiveEdgeLightingEffect.cpp
    │   └── EnergyStreamEffect.cpp
    └── perimeter/
        └── Perimeter.h / Perimeter.cpp       # Rounded-rectangle path
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
|---|---|---|
| `setCornerRadius(radius)` | Corner radius of the rounded rectangle (default 40). |
| `setCoreWidth(width)` | Base core line width in pixels (default 10). |
| `setPosition(x, y)` | Offset the rectangle center from screen center in pixels (default 0, 0). |
| `setSize(width, height)` | Explicit rectangle size in pixels (default 0 = auto 90% of framebuffer). |
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

### NeonWaveRingEffect

A neon glowing wave ring: a circular ribbon with sine-wave radial displacement, rendered in 3 layered passes (outer glow, inner glow, core) with additive blending for a neon glow effect.

| Method | Description |
|---|---|
| `setRadius(radius)` | Base radius of the ring in pixels (default 200). |
| `setAmplitude(amp)` | Wave displacement amplitude in pixels (default 30). |
| `setSpeed(speed)` | Wave rotation speed (default 2.0). |
| `setWaveCount(count)` | Number of wave cycles around the ring (default 5). |
| `setLineWidth(width)` | Core line width in pixels (default 6). |
| `setGlowWidth(width)` | Glow layer width in pixels (default 35). Outer glow is 2x this. |
| `setGlowIntensity(intensity)` | Glow brightness multiplier (default 1.0). Scales both alpha and shader uAlphaScale of glow layers. |

**Visual behaviour:**
- 3-layer rendering: outer glow (`alphaScale=0.4`), inner glow (`alphaScale=0.6`), core line (`alphaScale=1.0`).
- Outer and inner glow use additive blending (`GL_ONE`), core uses standard alpha (`GL_ONE_MINUS_SRC_ALPHA`).
- 5-stop gradient cycling cyan → purple → pink → orange → cyan, flowing along the ring over time.
- Sine wave displacement creates the undulating "energy ring" appearance.

### EnergyStreamEffect

Two luminous electric-blue energy streams traveling horizontally along the upper and lower edges of the frame, composed of glowing particles with bloom halos and luminous trails.

| Method | Description |
|---|---|
| `setSpeed(speed)` | Stream flow speed (default 1.0). |
| `setIntensity(intensity)` | Emission rate multiplier (default 1.0). |
| `setParticleCount(count)` | Max particles per stream (default 600). |
| `setGlowIntensity(intensity)` | Bloom brightness multiplier (default 1.0). |

**Visual behaviour:**
- 3-layer additive rendering: trails (line shader), outer glow (4× size bloom), core particles.
- Particles emerge from left edge, oscillate vertically with turbulence, and fade out.
- Random color variation: electric blue, neon cyan, white-hot highlights.
- Trailing geometry (14-point history) creates smooth luminous streaks.

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

## API Usage Flow

### Integration into your application

```
1. Include headers
   └── #include <EdgeLightingEffect/EdgePulseEffect.h>
   └── #include <EdgeLightingEffect/EffectManager.h>

2. Create instances (after OpenGL context is ready)
   └── EdgePulseEffect edgePulse;
   └── EffectManager manager;

3. Configure parameters (before init)
   └── edgePulse.setCoreWidth(12.0f);
   └── edgePulse.setCornerRadius(30.0f);

4. Register with manager
   └── manager.addEffect(&edgePulse);

5. One-time init
   └── manager.init(framebufferWidth, framebufferHeight);
   └── [per effect] → init(fbW, fbH) → load shaders, create VAO/VBO

6. Per frame — update then render
   └── manager.update(deltaTime);
   │     └── [per effect] → update(dt)
   │           ├── Advance elapsed time
   │           ├── Build vertex data on CPU
   │           └── Upload to VBO (glBufferData)
   └── manager.render();
         └── [per effect] → render()
               ├── glEnable(GL_BLEND), set blend func
               ├── Shader::use(), set uniforms
               └── glDrawArrays(GL_TRIANGLE_STRIP)

7. On framebuffer resize
   └── manager.onResize(newWidth, newHeight);
   └── [per effect] → onResize(w, h) → update projection matrix

8. Trigger notification (EdgePulseEffect only)
   └── edgePulse.triggerNotification();
   └── Internally: shakeIntensity = 15, pulseIntensity = 1.5
   └── Both decay each frame over ~2–4 seconds
```

### Minimal Example

```cpp
#include <EdgeLightingEffect/EdgePulseEffect.h>
#include <EdgeLightingEffect/EffectManager.h>

// Globals
EdgePulseEffect g_edgePulse;
EffectManager g_manager;

void init(int fbW, int fbH) {
    g_edgePulse.setCoreWidth(12.0f);
    g_edgePulse.setCornerRadius(40.0f);
    g_edgePulse.setPosition(100.0f, 50.0f);   // offset from center
    g_edgePulse.setSize(600.0f, 400.0f);      // explicit rectangle size
    g_manager.addEffect(&g_edgePulse);
    g_manager.init(fbW, fbH);
}

void update(float dt) {
    g_manager.update(dt);
}

void render() {
    g_manager.render();
}

void onResize(int fbW, int fbH) {
    g_manager.onResize(fbW, fbH);
}

void onNotification() {
    g_edgePulse.triggerNotification();
}
```

### Lifecycle Contract

| Phase | When | What happens |
|---|---|---|
| **Construction** | Before any GL calls | Create effect instances, set parameters |
| **Init** | After GL context is ready | Shader compilation, VAO/VBO allocation. Can fail → returns false |
| **Update** | Every frame before render | CPU geometry rebuild + VBO upload |
| **Render** | Every frame after update | Bind state, draw calls |
| **Resize** | On framebuffer size change | Update orthographic projection |
| **Destruction** | Before GL context is destroyed | Delete effect instances → destructors free GPU resources |

> **Note**: `EffectManager` does **not** own the effect pointers. You must ensure effects outlive the manager.

## Runtime Flow

### Application Startup
```
main()
  ├── glfwInit()
  ├── glfwCreateWindow("Edge Pulse Demo")
  ├── gladLoadGLLoader()
  ├── Enable GL_BLEND, GL_PROGRAM_POINT_SIZE
  ├── Create DemoEffects (all effect instances)
  ├── Configure effect parameters (speed, amplitude, etc.)
  ├── Create EffectManager
  ├── setupMode()
  │     ├── manager.clearEffects()
  │     ├── manager.addEffect(...)        # 1+ effects depending on mode
  │     ├── manager.init(fbW, fbH)
      │     └── [per effect] init(fbW, fbH)
  │     │           └── Load shaders, create VAO/VBO, set projection
  │     └── glfwSetWindowTitle()
  └── Enter main loop
```

### Main Loop (per frame)
```
while (!glfwWindowShouldClose)
  ├── glfwGetFramebufferSize()
  ├── [if resized] manager.onResize()     # Update projection matrices
  ├── [if mode changed] setupMode()       # Reconfigure effects
  ├── [EdgePulse mode] Check timer → triggerNotification()
  ├── glClear()
  ├── manager.update(dt)
  │     └── [per effect] update(dt)
  │           ├── Advance elapsedTime, decay animation params
  │           ├── Build geometry: CPU → std::vector<LineVertex>
  │           ├── glBindBuffer + glBufferData() upload to VBO
  │           └── (EdgePulse: builds core + glow two VBOs)
  ├── manager.render()
  │     └── [per effect] render()
  │           ├── glEnable(GL_BLEND), set blend func
  │           ├── Shader::use(), set uniforms (uProjection, uAlphaScale)
  │           ├── [EdgePulse] shake → translate projection
  │           ├── [EdgePulse] Draw core: alpha blend
  │           ├── [EdgePulse] If pulsing: draw glow overlay → additive blend
  │           ├── [MagneticWave] Draw N wave lines → each additive blend
  │           └── [MagneticRectangle] Draw perimeter → additive blend
  ├── glfwSwapBuffers()
  └── glfwPollEvents()
```

### Notification Trigger (EdgePulseEffect only)
```
triggerNotification()
  ├── shakeIntensity = 15.0f     # Screen-shake jitter amplitude
  └── pulseIntensity = 1.5f      # Glow overlay brightness & width

Each frame in update():
  ├── shakeIntensity *= (1.0 - 4.0 * dt)    # Decay to 0 in ~4s
  └── pulseIntensity *= (1.0 - 1.2 * dt)    # Decay to 0 in ~2s

In demo mode, notification auto-fires every 4–9 seconds.
```

### Input
| Key | Action |
|---|---|
| ← / → | Switch between effect modes (EdgePulse / MagneticWave / MagneticRectangle / All) |

## Geometry System

`Perimeter` models a rounded rectangle as 8 segments (4 straights + 4 quarter-circle arcs). All geometry is rebuilt on CPU each frame in `update()` and uploaded to the GPU via `glBufferData(GL_DYNAMIC_DRAW)`.

## Shader: line.vert / line.frag

Shared by all effects:

- **Vertex**: passes position (aPos), UV (aUV where `.y = ±1` across strip width), and per-vertex colour (aColor).
- **Fragment**: uses `abs(vUV.y)` with `smoothstep(0, 1, dist)` for a soft edge falloff, multiplied by `uAlphaScale` uniform.
