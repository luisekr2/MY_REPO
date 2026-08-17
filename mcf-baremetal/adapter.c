#include "mcf.h"

extern network_t net;
int spec_main(int argc, char **argv);
int runtime_has_failed(void);

#ifndef EXPECTED_CHECKSUM
#define EXPECTED_CHECKSUM 2997477L
#endif

int benchmark_main(void)
{
    char name[] = "inp.in";
    char *argv[] = { "mcf", name, 0 };
    int rc = spec_main(2, argv);

    if (rc != 0)
        return 1;
    if (runtime_has_failed())
        return 2;
    return net.checksum == EXPECTED_CHECKSUM ? 0 : 3;
}
