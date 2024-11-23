#include "mainEngine.h"

int main()
{
    FATAL_MESSAGE("A test message: %f", 3.14)
    ERROR_MESSAGE("A test message: %f", 3.14)
    WARNING_MESSAGE("A test message: %f", 3.14)
    DEBUG_MESSAGE("A test message: %f", 3.14)
    TRACE_MESSAGE("A test message: %f", 3.14)

    BLIT_ASSERT_DEBUG(0)
}
