#include <common.h>
#include <stdlib.h>
#include <math.h>

float random_float(float low, float high) {
    return (float)(rand()) / (float)(RAND_MAX / (high - low));
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}