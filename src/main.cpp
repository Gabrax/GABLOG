#include "gablog.h"

int main()
{
    Logger::Init();

    GABGL_TRACE("");
    GABGL_INFO("");
    GABGL_WARN("");
    GABGL_ERROR("");
    GABGL_CRITICAL("");
    bool test = false;
    GABGL_ASSERT(test, "Test");
}