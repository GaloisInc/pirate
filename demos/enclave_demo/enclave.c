#include <stdio.h>

#pragma capability declare(red)
#pragma enclave declare(alpha)
#pragma enclave declare(beta)
#pragma enclave declare(gamma)
#pragma enclave capability(alpha, red)

void onalpha(void)
    __attribute__((gaps_enclave_only("alpha")))
    __attribute__((gaps_capability("red")))
{
    printf("running on alpha\n");
}

void alphamain(void)
    __attribute__((gaps_enclave_main("alpha")))
{
    onalpha();
    printf("alpha started\n");
}

void betamain(void)
    __attribute__((gaps_enclave_main("beta")))
{
    printf("beta started\n");
}

void gammamain(void)
    __attribute__((gaps_enclave_main("gamma")))
{
    printf("gamma started\n");
}
