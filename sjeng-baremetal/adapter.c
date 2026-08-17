#include "runtime.h"

int spec_main(int argc, char **argv);
extern volatile unsigned long spec_sjeng_checksum;

#ifndef EXPECTED_CHECKSUM
#define EXPECTED_CHECKSUM 0UL
#endif

int benchmark_main(void)
{
    char input[] = "test-mini.txt";
    char *argv[] = { "sjeng", input, 0 };
    int rc = spec_main(2, argv);
    if (rc != 0 || runtime_has_failed()) return 1;
    return spec_sjeng_checksum == EXPECTED_CHECKSUM ? 0 : 2;
}
