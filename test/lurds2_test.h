
#define ASSERT_THAT(condition) assert_that((condition), #condition, __FUNCTION__, __FILE__, __LINE__)

#include <stdio.h>
#include <stdlib.h>

int sTotalTestCount = 0;

void assert_that(int condition, const char * expression, const char* function_name, const char* file_name, int line_number)
{
  sTotalTestCount++;
  if (!condition)
  {
    printf("Expected %s to equal true, but it was false at line %d of %s() in %s\n", expression, line_number, function_name, file_name);
    fflush(stdout);
    exit(1);
  }
}
