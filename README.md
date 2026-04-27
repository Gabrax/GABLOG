<div align="center">

# Logging

</div>

```c
GABLOG_TRACE("Trace message");
GABLOG_INFO("Hello %s", "world");
GABLOG_WARN("Warning!");
GABLOG_ERROR("Error occurred");
GABLOG_ASSERT(!x, "Assertion failed: %d is false, x);
```

Example Output:

```text
[INFO] Hello world (main.c:10)
[WARN] Warning! (main.c:11)
[ERROR] Error occurred (main.c:12)
```

To disable logging less than specified alert level, use this function:

```c
void gablog_set_level(LogLevel level);
```

<div align="center">

# Profiling

</div>

```c
for (;;)
{
    GABPROFILER_CLEAR();

    GABPROFILE_SCOPE("Frame")
    {
        // your code here
    }

    GABPROFILER_PRINT();
}
```

Example Output:

```text
Frame: CPU 16.42 ms | GPU 14.87 ms
Update: CPU 5.10 ms | GPU 0.00 ms
Render: CPU 11.20 ms | GPU 14.80 ms
```


The profiler is frame-based, which avoids GPU stalls and keeps performance consistent.

```text
Frame N     → record timings
Frame N+1   → GPU processing
Frame N+2   → resolve results
```

Adjust these macros to your needs:

```c
#define GABMAX_QUERIES_PER_FRAME 256
#define GABQUERY_LATENCY 3
```

<div align="center">

# Installation

</div>

1. Copy `gabdebug.h` into your project

2. In **one** source file:

```c
#define GABDEBUG_IMPLEMENTATION
#define GABPROFILER_IMPLEMENTATION
#include "gabdebug.h"
```

3. In all other files:

```c
#include "gabdebug.h"
```

Libraries are enabled by default. You can enable or disable them using the following macros.

```c
#define GABDEBUG_ENABLE false
#define GABPROFILER_ENABLE false
```

To enable GPU profiling, select the graphics API you are using:

```c
#define GAB_GL
```

Stripped-prefix versions are enabled by default. In case of naming conflicts, use the following defines:

```c
#define GABDEBUG_UNSTRIP_PREFIX
#define GABPROFILER_UNSTRIP_PREFIX
```


