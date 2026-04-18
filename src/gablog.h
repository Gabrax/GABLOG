#ifndef GABLOG_H
#define GABLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>

#ifndef GABLOG_ENABLE
    #define GABLOG_ENABLE 1
#endif

#ifndef GABLOG_UNSTRIP_PREFIX
    #define GABLOG_STRIP_PREFIX
#endif

typedef enum
{
    LOG_TRACE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_CRITICAL,
    LOG_ASSERT
} LogLevel;

void gablog_set_level(LogLevel level);
void gablog_log(LogLevel level, const char* file, int line, const char* fmt, ...);

#if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
#else
    #include <signal.h>
    #define DEBUG_BREAK() raise(SIGTRAP)
#endif

#if GABLOG_ENABLE
    #define GABLOG_TRACE(...)    gablog_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
    #define GABLOG_INFO(...)     gablog_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
    #define GABLOG_WARN(...)     gablog_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
    #define GABLOG_ERROR(...)    gablog_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
    #define GABLOG_CRITICAL(...) gablog_log(LOG_CRITICAL, __FILE__, __LINE__, __VA_ARGS__)
    #define GABLOG_ASSERT(x, fmt, ...)                                      \
    do {                                                                    \
        if (!(x)) {                                                         \
            gablog_log(LOG_ASSERT, __FILE__, __LINE__,                      \
            fmt, ##__VA_ARGS__);                                            \
            DEBUG_BREAK();                                                  \
        }                                                                   \
    } while(0)

    #ifdef GABLOG_STRIP_PREFIX
        #define TRACE(...)    GABLOG_TRACE(__VA_ARGS__)
        #define INFO(...)     GABLOG_INFO(__VA_ARGS__)
        #define WARN(...)     GABLOG_WARN(__VA_ARGS__)
        #define ERROR(...)    GABLOG_ERROR(__VA_ARGS__)
        #define CRITICAL(...) GABLOG_CRITICAL(__VA_ARGS__)
        #define ASSERT(...) GABLOG_ASSERT(__VA_ARGS__)
    #endif
#else
    #define GABLOG_TRACE(...)    ((void)0)
    #define GABLOG_INFO(...)     ((void)0)
    #define GABLOG_WARN(...)     ((void)0)
    #define GABLOG_ERROR(...)    ((void)0)
    #define GABLOG_CRITICAL(...) ((void)0)
    #define GABLOG_ASSERT(...)   ((void)0)

    #ifdef GABLOG_STRIP_PREFIX
        #define TRACE(...)    ((void)0)
        #define INFO(...)     ((void)0)
        #define WARN(...)     ((void)0)
        #define ERROR(...)    ((void)0)
        #define CRITICAL(...) ((void)0)
        #define ASSERT(...)   ((void)0)
    #endif
#endif

#ifdef GABLOG_IMPLEMENTATION

#include <string.h>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <pthread.h>
    #include <unistd.h>
#endif

static LogLevel g_LogLevel = LOG_TRACE;
static int g_UseColor = 1;
static int g_TerminalInitialized = 0;

#if defined(_WIN32)
static CRITICAL_SECTION g_LogMutex;
static int g_MutexInitialized = 0;
#else
static pthread_mutex_t g_LogMutex = PTHREAD_MUTEX_INITIALIZER;
#endif


static void gablog_init_mutex(void)
{
#if defined(_WIN32)
    if (!g_MutexInitialized)
    {
        InitializeCriticalSection(&g_LogMutex);
        g_MutexInitialized = 1;
    }
#endif
}

static void gablog_lock(void)
{
#if defined(_WIN32)
    EnterCriticalSection(&g_LogMutex);
#else
    pthread_mutex_lock(&g_LogMutex);
#endif
}

static void gablog_unlock(void)
{
#if defined(_WIN32)
    LeaveCriticalSection(&g_LogMutex);
#else
    pthread_mutex_unlock(&g_LogMutex);
#endif
}

static void gablog_init_terminal(void)
{
#if defined(_WIN32)

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        g_UseColor = 0;
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode))
    {
        g_UseColor = 0;
        return;
    }

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    if (!SetConsoleMode(hOut, mode))
        g_UseColor = 0;

#else
    if (!isatty(fileno(stdout)))
        g_UseColor = 0;
#endif
}

static inline const char* gablog_strip_filename(const char* path)
{
    const char* slash = strrchr(path, '/');

#if defined(_WIN32)
    const char* backslash = strrchr(path, '\\');
    if (!slash || (backslash && backslash > slash))
        slash = backslash;
#endif

    return slash ? slash + 1 : path;
}

static inline const char* LevelToString(LogLevel level)
{
    switch (level)
    {
        case LOG_TRACE:    return "TRACE";
        case LOG_INFO:     return "INFO";
        case LOG_WARN:     return "WARN";
        case LOG_ERROR:    return "ERROR";
        case LOG_CRITICAL: return "CRIT";
        case LOG_ASSERT:   return "ASSERT";
        default:           return "UNKWN";
    }
}

static inline const char* LevelColor(LogLevel level)
{
    if (!g_UseColor)
        return "";

    switch (level)
    {
        case LOG_TRACE:    return "\033[37m";
        case LOG_INFO:     return "\033[32m";
        case LOG_WARN:     return "\033[33m";
        case LOG_ERROR:    return "\033[31m";
        case LOG_CRITICAL: return "\033[41m";
        case LOG_ASSERT:   return "\033[41m";
        default:           return "\033[0m";
    }
}

void gablog_set_level(const LogLevel level)
{
    g_LogLevel = level;
}

void gablog_log(const LogLevel level, const char* file, int line, const char* fmt, ...)
{
#if GABLOG_ENABLE

    if (level < g_LogLevel)
        return;

    if (!g_TerminalInitialized)
    {
        gablog_init_mutex();
        gablog_init_terminal();
        g_TerminalInitialized = 1;
    }

    gablog_lock();

    char message[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    fprintf(stdout,
            "%s[%s] %s%s (%s:%d)\n",
            LevelColor(level),
            LevelToString(level),
            message,
            g_UseColor ? "\033[0m" : "",
            gablog_strip_filename(file),
            line);

    fflush(stdout);

    gablog_unlock();

#endif
}
#endif /* GABLOG_IMPLEMENTATION */

#include <stdint.h>

#define gabMAX_QUERIES_PER_FRAME 256
#define gabQUERY_LATENCY 3

#ifdef GAB_GL
#include <glad/glad.h>
#endif

typedef struct
{
    const char* name;
    float cpuTime;
    float gpuTime; /* 0 if GPU unavailable */
} GABProfileResult;

typedef struct
{
    const char* name;
    unsigned int query;
    int hasQuery;
    double start;
} GABProfilerScope;

void gabprofiler_init(void);
void gabprofiler_shutdown(void);
void gabprofiler_begin_frame(void);

GABProfilerScope gabprofiler_begin(const char* name);
void gabprofiler_end(GABProfilerScope* scope);

const GABProfileResult* gabprofiler_get_results(uint32_t* count);

#ifndef GABPROFILER_ENABLE
    #define GABPROFILER_ENABLE 1
#endif

#ifndef GABPROFILER_UNSTRIP_PREFIX
    #define GABPROFILER_STRIP_PREFIX 1
#endif

#define GAB_CONCAT_IMPL(x, y) x##y
#define GAB_CONCAT(x, y) GAB_CONCAT_IMPL(x, y)

#if GABPROFILER_ENABLE
    #define GABPROFILE_SCOPE(name) \
        GABProfilerScope GAB_CONCAT(scope_, __LINE__) = gabprofiler_begin(name); \
        for (int GAB_CONCAT(_once_, __LINE__) = 1; \
            GAB_CONCAT(_once_, __LINE__); \
            gabprofiler_end(&GAB_CONCAT(scope_, __LINE__)), \
            GAB_CONCAT(_once_, __LINE__) = 0)

    #define GABPROFILER_CLEAR_RESULTS() gabprofiler_begin_frame()

    #if GABPROFILER_STRIP_PREFIX
        #define PROFILE_SCOPE(name) GABPROFILE_SCOPE(name)
        #define PROFILER_CLEAR_RESULTS() GABPROFILER_CLEAR_RESULTS()
    #endif
#else
    #define GABPROFILE_SCOPE(name)
    #define GABPROFILER_CLEAR_RESULTS()

    #define PROFILE_SCOPE(name)
    #define PROFILER_CLEAR_RESULTS()
#endif

#ifdef GABPROFILER_IMPLEMENTATION

#ifdef _WIN32
#include <windows.h>
static LARGE_INTEGER s_Frequency;
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#include <time.h>

#if !defined(_WIN32) && !defined(__APPLE__)
#define _POSIX_C_SOURCE 199309L
#endif

static double gab_time_ms(void)
{
#ifdef _WIN32
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / (double)s_Frequency.QuadPart;

#elif defined(__APPLE__)
    static mach_timebase_info_data_t timebase;
    static int init = 0;

    if (!init)
    {
        mach_timebase_info(&timebase);
        init = 1;
    }

    uint64_t t = mach_absolute_time();
    return (double)t * timebase.numer / timebase.denom / 1e6;

#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
#endif
}

typedef struct
{
    const char* name;
    float cpuTime;
#ifdef GAB_GL
    GLuint query;
#endif
    int hasQuery;
} GPUQueryData;

static uint32_t s_CurrentFrame = 0;
static uint32_t s_QueryIndex = 0;

#ifdef GAB_GL
static GLuint s_QueryPool[gabQUERY_LATENCY][gabMAX_QUERIES_PER_FRAME];
#endif

static GPUQueryData s_FrameQueries[gabQUERY_LATENCY][gabMAX_QUERIES_PER_FRAME];
static uint32_t s_FrameQueryCounts[gabQUERY_LATENCY];

static GABProfileResult s_Results[gabMAX_QUERIES_PER_FRAME];
static uint32_t s_ResultCount = 0;

static void resolve_frame(uint32_t frameIndex)
{
    uint32_t count = s_FrameQueryCounts[frameIndex];

    for (uint32_t i = 0; i < count; i++)
    {
        GPUQueryData* q = &s_FrameQueries[frameIndex][i];

        float gpuTime = 0.0f;

#ifdef GAB_GL
        if (q->hasQuery)
        {
            GLuint64 timeElapsed = 0;
            glGetQueryObjectui64v(q->query, GL_QUERY_RESULT, &timeElapsed);
            gpuTime = (float)(timeElapsed / 1000000.0);
        }
#endif

        if (s_ResultCount < gabMAX_QUERIES_PER_FRAME)
        {
            s_Results[s_ResultCount++] = (GABProfileResult){
                q->name,
                q->cpuTime,
                gpuTime
            };
        }
    }

    s_FrameQueryCounts[frameIndex] = 0;
}

void gabprofiler_init(void)
{
#ifdef _WIN32
    QueryPerformanceFrequency(&s_Frequency);
#endif

#ifdef GAB_GL
    for (uint32_t i = 0; i < gabQUERY_LATENCY; i++)
    {
        glGenQueries(gabMAX_QUERIES_PER_FRAME, s_QueryPool[i]);
    }
#endif

    for (uint32_t i = 0; i < gabQUERY_LATENCY; i++)
    {
        s_FrameQueryCounts[i] = 0;
    }
}

void gabprofiler_shutdown(void)
{
#ifdef GAB_GL
    for (uint32_t i = 0; i < gabQUERY_LATENCY; i++)
    {
        glDeleteQueries(gabMAX_QUERIES_PER_FRAME, s_QueryPool[i]);
    }
#endif
}

void gabprofiler_begin_frame(void)
{
    s_ResultCount = 0;

    s_CurrentFrame = (s_CurrentFrame + 1) % gabQUERY_LATENCY;
    s_QueryIndex = 0;

    uint32_t resolveIndex = (s_CurrentFrame + 1) % gabQUERY_LATENCY;
    resolve_frame(resolveIndex);
}

#ifdef GAB_GL
static GLuint acquire_query(void)
{
    if (s_QueryIndex >= gabMAX_QUERIES_PER_FRAME)
        return 0;

    return s_QueryPool[s_CurrentFrame][s_QueryIndex++];
}
#endif

GABProfilerScope gabprofiler_begin(const char* name)
{
    GABProfilerScope scope;
    scope.name = name;
    scope.query = 0;
    scope.hasQuery = 0;
    scope.start = gab_time_ms();

#ifdef GAB_GL
    GLuint q = acquire_query();
    if (q)
    {
        scope.query = q;
        scope.hasQuery = 1;
        glBeginQuery(GL_TIME_ELAPSED, q);
    }
#endif

    return scope;
}

void gabprofiler_end(GABProfilerScope* scope)
{
    if (!scope->name)
        return;

#ifdef GAB_GL
    if (scope->hasQuery)
        glEndQuery(GL_TIME_ELAPSED);
#endif

    double end = gab_time_ms();
    float cpuTime = (float)(end - scope->start);

    uint32_t index = s_FrameQueryCounts[s_CurrentFrame];

    if (index < gabMAX_QUERIES_PER_FRAME)
    {
        s_FrameQueries[s_CurrentFrame][index].name = scope->name;
        s_FrameQueries[s_CurrentFrame][index].cpuTime = cpuTime;
        s_FrameQueries[s_CurrentFrame][index].hasQuery = scope->hasQuery;

#ifdef GAB_GL
        s_FrameQueries[s_CurrentFrame][index].query = scope->query;
#endif
        s_FrameQueryCounts[s_CurrentFrame]++;
    }
}

const GABProfileResult* gabprofiler_get_results(uint32_t* count)
{
    *count = s_ResultCount;
    return s_Results;
}

#endif /* GABPROFILER_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* GABLOG_H */