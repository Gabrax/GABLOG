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

#if GABLOG_ENABLE

typedef enum
{
    LOG_TRACE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
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

    #define GABLOG_TRACE(...)    gablog_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
    #define GABLOG_INFO(...)     gablog_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
    #define GABLOG_WARN(...)     gablog_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
    #define GABLOG_ERROR(...)    gablog_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
    #define GABLOG_ASSERT(x, fmt, ...)                                      \
    do {                                                                    \
        if (!(x)) {                                                         \
            gablog_log(LOG_ASSERT, __FILE__, __LINE__,                      \
            fmt, ##__VA_ARGS__);                                            \
            DEBUG_BREAK();                                                  \
        }                                                                   \
    } while(0)

    #ifdef GABLOG_STRIP_PREFIX
        #define LOG_TRACE(...)    GABLOG_TRACE(__VA_ARGS__)
        #define LOG_INFO(...)     GABLOG_INFO(__VA_ARGS__)
        #define LOG_WARN(...)     GABLOG_WARN(__VA_ARGS__)
        #define LOG_ERROR(...)    GABLOG_ERROR(__VA_ARGS__)
        #define LOG_ASSERT(...)   GABLOG_ASSERT(__VA_ARGS__)
    #endif
#else
    #define GABLOG_TRACE(...)    ((void)0)
    #define GABLOG_INFO(...)     ((void)0)
    #define GABLOG_WARN(...)     ((void)0)
    #define GABLOG_ERROR(...)    ((void)0)
    #define GABLOG_ASSERT(...)   ((void)0)

    #ifdef GABLOG_STRIP_PREFIX
        #define LOG_TRACE(...)    ((void)0)
        #define LOG_INFO(...)     ((void)0)
        #define LOG_WARN(...)     ((void)0)
        #define LOG_ERROR(...)    ((void)0)
        #define LOG_ASSERT(...)   ((void)0)
    #endif
#endif

#if defined(GABLOG_IMPLEMENTATION) && GABLOG_ENABLE

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

static const char* LevelToString(const LogLevel level)
{
    switch (level)
    {
        case LOG_TRACE:    return "TRACE";
        case LOG_INFO:     return "INFO";
        case LOG_WARN:     return "WARN";
        case LOG_ERROR:    return "ERROR";
        case LOG_ASSERT:   return "ASSERT";
        default:           return "UNKWN";
    }
}

static const char* LevelColor(const LogLevel level)
{
    if (!g_UseColor)
        return "";

    switch (level)
    {
        case LOG_TRACE:    return "\033[37m";
        case LOG_INFO:     return "\033[32m";
        case LOG_WARN:     return "\033[33m";
        case LOG_ERROR:    return "\033[31m";
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

#ifndef GABPROFILER_ENABLE
    #define GABPROFILER_ENABLE 1
#endif

#if GABPROFILER_ENABLE

typedef struct GABProfileNode
{
    const char* name;
    float cpuTime;

    struct GABProfileNode* parent;
    struct GABProfileNode* firstChild;
    struct GABProfileNode* lastChild;
    struct GABProfileNode* nextSibling;
} GABProfileNode;

#define GAB_MAX_NODES 2048
#define GAB_MAX_THREADS 64

typedef struct
{
    GABProfileNode nodes[GAB_MAX_NODES];
    unsigned int nodeCount;

    GABProfileNode* current;
    GABProfileNode* firstRoot;
    GABProfileNode* lastRoot;
} GABThreadContext;

void gabprofiler_begin_frame(void);
GABProfileNode* gabprofiler_get_root(void);

#define GAB_CONCAT_IMPL(x,y) x##y
#define GAB_CONCAT(x,y) GAB_CONCAT_IMPL(x,y)

typedef struct
{
    GABProfileNode* node;
    double start;
} GABProfilerScope;

GABProfilerScope gabprofiler_begin(const char* name);
void gabprofiler_end(GABProfilerScope* scope);
void gabprofiler_print(void);

#define GABPROFILER_CLEAR() gabprofiler_begin_frame()
#define GABPROFILE_SCOPE(name) \
    GABProfilerScope GAB_CONCAT(scope_, __LINE__) = gabprofiler_begin(name); \
    for (int GAB_CONCAT(_once_, __LINE__) = 1; \
        GAB_CONCAT(_once_, __LINE__); \
        gabprofiler_end(&GAB_CONCAT(scope_, __LINE__)), \
        GAB_CONCAT(_once_, __LINE__) = 0)

#define GABPROFILER_PRINT() gabprofiler_print()

#else
    #define GABPROFILER_CLEAR()
    #define GABPROFILE_SCOPE(name)
    #define GABPROFILER_PRINT()
#endif

#if defined(GABPROFILER_IMPLEMENTATION) && GABPROFILER_ENABLE

#include <stdio.h>

#if defined(_WIN32)
    #include <windows.h>
    static LARGE_INTEGER s_Frequency;
    static INIT_ONCE g_once = INIT_ONCE_STATIC_INIT;
    static CRITICAL_SECTION g_mutex;
#elif defined(__APPLE__) || defined(__linux__)
    #include <pthread.h>
    #include <time.h>
    static pthread_once_t g_once = PTHREAD_ONCE_INIT;
    static pthread_mutex_t g_mutex;
#endif

static GABThreadContext* g_threads[GAB_MAX_THREADS];
static int g_threadCount = 0;

#if defined(_MSC_VER)
    #define GAB_THREAD_LOCAL __declspec(thread)
#else
    #define GAB_THREAD_LOCAL __thread
#endif

GAB_THREAD_LOCAL static GABThreadContext g_ctx;
GAB_THREAD_LOCAL static int g_registered = 0;

static double gab_time_ms(void)
{
#if defined(_WIN32)
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / (double)s_Frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
#endif
}

#if defined(_WIN32)
BOOL CALLBACK gabprofiler_init_once(PINIT_ONCE InitOnce, PVOID Param, PVOID* Context)
{
    QueryPerformanceFrequency(&s_Frequency);
    InitializeCriticalSection(&g_mutex);
    return TRUE;
}
#else
void gabprofiler_init_once(void)
{
    pthread_mutex_init(&g_mutex, NULL);
}
#endif

static void gabprofiler_init(void)
{
#if defined(_WIN32)
    InitOnceExecuteOnce(&g_once, gabprofiler_init_once, NULL, NULL);
#else
    pthread_once(&g_once, gabprofiler_init_once);
#endif
}

static void gabprofiler_register_thread(void)
{
    if (g_registered) return;
    g_registered = 1;

    gabprofiler_init();

#if defined(_WIN32)
    EnterCriticalSection(&g_mutex);
#else
    pthread_mutex_lock(&g_mutex);
#endif

    if (g_threadCount < GAB_MAX_THREADS)
    {
        g_threads[g_threadCount++] = &g_ctx;
    }

#if defined(_WIN32)
    LeaveCriticalSection(&g_mutex);
#else
    pthread_mutex_unlock(&g_mutex);
#endif
}

void gabprofiler_begin_frame(void)
{
    gabprofiler_register_thread();

    g_ctx.nodeCount = 0;
    g_ctx.current = NULL;
    g_ctx.firstRoot = NULL;
    g_ctx.lastRoot = NULL;
}

GABProfilerScope gabprofiler_begin(const char* name)
{
    GABProfilerScope scope;

    GABProfileNode* parent = g_ctx.current;
    GABProfileNode* node = NULL;

    if (parent)
    {
        for (GABProfileNode* c = parent->firstChild; c; c = c->nextSibling)
            if (c->name == name) { node = c; break; }
    }
    else
    {
        for (GABProfileNode* r = g_ctx.firstRoot; r; r = r->nextSibling)
            if (r->name == name) { node = r; break; }
    }

    if (!node)
    {
        if (g_ctx.nodeCount >= GAB_MAX_NODES)
        {
            scope.node = NULL;
            scope.start = 0;
            return scope;
        }

        node = &g_ctx.nodes[g_ctx.nodeCount++];
        node->name = name;
        node->cpuTime = 0.0f;
        node->firstChild = NULL;
        node->lastChild = NULL;
        node->nextSibling = NULL;
        node->parent = parent;

        if (parent)
        {
            if (!parent->firstChild)
                parent->firstChild = parent->lastChild = node;
            else
            {
                parent->lastChild->nextSibling = node;
                parent->lastChild = node;
            }
        }
        else
        {
            if (!g_ctx.firstRoot)
                g_ctx.firstRoot = g_ctx.lastRoot = node;
            else
            {
                g_ctx.lastRoot->nextSibling = node;
                g_ctx.lastRoot = node;
            }
        }
    }

    g_ctx.current = node;

    scope.node = node;
    scope.start = gab_time_ms();
    return scope;
}

void gabprofiler_end(GABProfilerScope* scope)
{
    if (!scope->node) return;

    double end = gab_time_ms();
    scope->node->cpuTime += (float)(end - scope->start);

    g_ctx.current = scope->node->parent;
}

static void gabprofiler_print_node(GABProfileNode* node, int depth)
{
    for (int i = 0; i < depth; i++)
        printf("  ");

    printf("\033[96m[%s]\033[0m: %.3f ms\n", node->name, node->cpuTime);

    for (GABProfileNode* c = node->firstChild; c; c = c->nextSibling)
        gabprofiler_print_node(c, depth + 1);
}

void gabprofiler_print(void)
{
#if defined(_WIN32)
    EnterCriticalSection(&g_mutex);
#else
    pthread_mutex_lock(&g_mutex);
#endif

    for (int i = 0; i < g_threadCount; i++)
    {
        printf("\n=== Thread %d ===\n", i);

        GABThreadContext* ctx = g_threads[i];
        for (GABProfileNode* n = ctx->firstRoot; n; n = n->nextSibling)
            gabprofiler_print_node(n, 0);
    }

#if defined(_WIN32)
    LeaveCriticalSection(&g_mutex);
#else
    pthread_mutex_unlock(&g_mutex);
#endif
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* GABLOG_H */