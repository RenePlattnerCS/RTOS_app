#include "unity.h"

/* Example function under test */
uint32_t add(uint32_t a, uint32_t b)
{
    return a + b;
}

void setUp(void)
{
}
void tearDown(void)
{
}

void test_add(void)
{
    TEST_ASSERT_EQUAL_UINT32(5, add(2, 3));
}
