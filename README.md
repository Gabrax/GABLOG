## 📝 Logging

* Multiple log levels (TRACE → CRITICAL)
* Colored output (Windows/Linux/macOS)
* Thread-safe
* File + line reporting
* Assertions with debugger break

## ⏱ Profiling

* High-resolution CPU timing (cross-platform)
* Optional GPU timing (OpenGL)
* Scoped profiling macros
* Frame-based system (no GPU stalls)
* Automatic lazy initialization

# 📦 Installation

1. Copy `GABDEBUG.h` into your project

2. In **one** source file:

```c
#define GABDEBUG_IMPLEMENTATION
#define GABPROFILER_IMPLEMENTATION
#include "GABDEBUG.h"
```

3. In all other files:

```c
#include "GABDEBUG.h"
```

## Enable / Disable systems

```c
#define GABDEBUG_ENABLE 0
#define GABPROFILER_ENABLE 0
```

* Completely removes code at compile time
* Zero runtime cost

## Prefix Stripping (Optional)

Enable shorter macros:

```c
#define GABDEBUG_STRIP_PREFIX
#define GABPROFILER_STRIP_PREFIX
```

# Logging Usage

```c
GAB_TRACE("Trace message");
GAB_INFO("Hello %s", "world");
GAB_WARN("Warning!");
GAB_ERROR("Error occurred");
GAB_ASSERT(!x, "Assertion failed: %d is false, x);
```

### Output

```text
[INFO] Hello world (main.c:10)
[WARN] Warning! (main.c:11)
[ERROR] Error occurred (main.c:12)
```


```c
GABDEBUG_set_level(LOG_WARN);
```

# ⏱ Profiler Usage

## Basic Frame Loop

```c
for (;;)
{
    GABPROFILER_CLEAR_RESULTS();

    GABPROFILE_SCOPE("Frame")
    {
        // your code here
    }

    GABPROFILER_PRINT_RESULTS();
}
```

Enable OpenGL support:

```c
#define GAB_GL
#include <glad/glad.h>
```

Requirements:

* OpenGL context must be initialized
* Uses `GL_TIME_ELAPSED` queries
* Results appear after a few frames (latency buffering)


## Example Output

```text
Frame: CPU 16.42 ms | GPU 14.87 ms
Update: CPU 5.10 ms | GPU 0.00 ms
Render: CPU 11.20 ms | GPU 14.80 ms
```

# 🔁 Profiler Architecture

The profiler is **frame-based**:

```text
Frame N     → record timings
Frame N+1   → GPU processing
Frame N+2   → resolve results
```

This avoids GPU stalls and keeps performance consistent.

```c
#define gabMAX_QUERIES_PER_FRAME 256
#define gabQUERY_LATENCY 3
```

# 🔒 Thread Safety

* Logging: ✅ thread-safe
* Profiler: ⚠️ single-threaded

# ⚠️ Notes

* First few frames may have no GPU data (expected)
* CPU work must not be optimized away (use `volatile`)
* Always profile in **Release mode**
* GPU timing requires actual draw calls

# 🛠 Future Improvements

* [ ] Hierarchical profiler (tree view)
* [ ] Async logging
* [ ] Thread-safe profiler
* [ ] More GPU API support