#include "quantum.h"

#ifndef LQ_WIDTH
#define LQ_WIDTH 8
#endif

#ifndef LQ_ROUNDS
#define LQ_ROUNDS 1
#endif

int runtime_has_failed(void);
static volatile unsigned long libquantum_signature;

/*
 * A bounded libquantum kernel for slow RTL simulation.  It uses the original
 * sparse quantum-register representation and original Walsh, controlled-gate,
 * phase, QFT, and inverse-QFT implementations.  Unlike a synthetic integer
 * loop, the dominant work is still complex-amplitude traversal, hash lookup,
 * state expansion, and quantum gate application from 462.libquantum.
 */
int benchmark_main(void)
{
    quantum_reg reg = quantum_new_qureg(0, LQ_WIDTH);
    double probability = 0.0;
    unsigned long signature = 0;
    int round, bit, i;

    quantum_walsh(LQ_WIDTH, &reg);
    for (round = 0; round < LQ_ROUNDS; ++round) {
        for (bit = 0; bit + 1 < LQ_WIDTH; ++bit) {
            quantum_cnot(bit, bit + 1, &reg);
            quantum_cond_phase(bit + 1, bit, &reg);
        }
        quantum_qft(LQ_WIDTH, &reg);
        quantum_qft_inv(LQ_WIDTH, &reg);
        quantum_sigma_x(round % LQ_WIDTH, &reg);
        quantum_sigma_x(round % LQ_WIDTH, &reg);
    }

    for (i = 0; i < reg.size; ++i) {
        probability += quantum_prob(reg.node[i].amplitude);
        signature ^= (unsigned long)reg.node[i].state +
                     ((unsigned long)i * 0x9e3779b97f4a7c15UL);
    }
    libquantum_signature = signature;
    quantum_delete_qureg(&reg);

    return runtime_has_failed() || probability < 0.90 || probability > 1.10 ||
           signature == 0;
}
