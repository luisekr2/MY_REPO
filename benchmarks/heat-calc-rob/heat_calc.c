typedef unsigned long u64;

#define N 100
#ifndef STEPS
#define STEPS 32
#endif

static volatile double u[N];
static volatile double u_new[N];
static volatile u64 q[N];
static volatile u64 q_new[N];
static volatile u64 result_sink;

__attribute__((noinline, aligned(64)))
static void native_step(void)
{
  for (int i = 1; i < N - 1; ++i) {
    double laplacian = u[i - 1] - 2.0 * u[i] + u[i + 1];
    u_new[i] = u[i] + 0.1 * laplacian;
  }

  u_new[0] = u[0];
  u_new[N - 1] = u[N - 1];

  for (int i = 0; i < N; ++i)
    u[i] = u_new[i];
}

/*
 * Heat-like fixed-point phase for ROB/TAGE stress.  The independent integer
 * mixing operations are intentionally kept in the loop so they remain
 * ROB-compressible, while the two predictable loop branches are revisited at
 * nearly the same cadence as the native heat-calc loops.
 */
__attribute__((noinline, aligned(64)))
static u64 tage_stress_step(u64 salt)
{
  u64 mix0 = salt ^ 0x9e3779b97f4a7c15UL;
  u64 mix1 = salt + 0x243f6a8885a308d3UL;
  u64 mix2 = salt ^ 0xb7e151628aed2a6bUL;
  u64 mix3 = salt + 0x94d049bb133111ebUL;

  for (int i = 1; i < N - 1; ++i) {
    u64 left = q[i - 1];
    u64 center = q[i];
    u64 right = q[i + 1];
    q_new[i] = center + ((left + right - (center << 1)) >> 3);

    mix0 = (mix0 + left) ^ (mix1 >> 7);
    mix1 = (mix1 + center) ^ (mix2 << 9);
    mix2 = (mix2 + right) ^ (mix3 >> 11);
    mix3 = (mix3 + (u64)i) ^ (mix0 << 13);
  }

  q_new[0] = q[0];
  q_new[N - 1] = q[N - 1];

  for (int i = 0; i < N; ++i) {
    q[i] = q_new[i];
    mix0 ^= q[i] + (u64)i;
    mix1 += (q[i] << 3) ^ mix0;
  }

  return mix0 ^ mix1 ^ mix2 ^ mix3;
}

int benchmark_main(void)
{
  for (int i = 0; i < N; ++i) {
    u[i] = i == N / 2 ? 100.0 : 0.0;
    q[i] = i == N / 2 ? (100UL << 32) : 0;
    u_new[i] = 0.0;
    q_new[i] = 0;
  }

#ifdef TAGE_STRESS
  u64 signature = 1;
  for (int step = 0; step < STEPS; ++step)
    signature = tage_stress_step(signature + (u64)step);
  result_sink = signature ^ q[N / 2];
  return result_sink == 0;
#else
  for (int step = 0; step < STEPS; ++step)
    native_step();

  double checksum = 0.0;
  for (int i = 0; i < N; ++i)
    checksum += u[i];

  union {
    double d;
    u64 u;
  } checksum_bits = { checksum };
  result_sink = checksum_bits.u;
  return !(checksum > 99.999 && checksum < 100.001);
#endif
}
