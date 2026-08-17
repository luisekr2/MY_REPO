#ifndef BARE_MATH_H
#define BARE_MATH_H
double sqrt(double); double sin(double); double cos(double); double log(double);
static inline float fabsf(float x) { return x < 0.0f ? -x : x; }
#endif
