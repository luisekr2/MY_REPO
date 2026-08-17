#include <stdio.h>

int spec_main(int argc, char **argv);
extern volatile unsigned long spec_sjeng_checksum;

int main(void)
{
    char input[] = "test-mini.txt";
    char *argv[] = { "sjeng", input, 0 };
    int rc = spec_main(2, argv);
    fprintf(stderr, "NATIVE_RC=%d CHECKSUM=%lu\n", rc,
            spec_sjeng_checksum);
    return rc;
}
