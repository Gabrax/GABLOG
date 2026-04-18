#include <algorithm>

#include "gablog.h"

int main()
{
    GABLOG_TRACE("Test");
    GABLOG_INFO("Test");
    GABLOG_WARN("Test");
    GABLOG_ERROR("Test");
    GABLOG_CRITICAL("Test");
    bool test = false;
    //GABLOG_ASSERT(test, "Test");

    TRACE("Test");
    //ASSERT(test, "Test");

    int count1 = 0;
    int count2 = 0;

    gabprofiler_init();

    for (int frame = 0; frame < 100; frame++)
    {
        GABPROFILER_CLEAR_RESULTS();

        GABPROFILE_SCOPE("Frame")
        {
            volatile double result = 0.0;

            for (uint32_t i = 1; i < 10000; i++)
                for (uint32_t j = 1; j < 10000; j++)
                {
                    result += (i * 0.5) / (j + 1.0);
                }
        }

        uint32_t count;
        const GABProfileResult* results = gabprofiler_get_results(&count);

        for (uint32_t i = 0; i < count; i++)
        {
            printf("%s: CPU %.6f ms | GPU %.6f ms\n",
                results[i].name,
                results[i].cpuTime,
                results[i].gpuTime);
        }
    }
}