#include "triliteration_small.h"

#define PI (4.0 * atan(1.0))

float angled(int a, int b, int c)
{
    float da = (float) a;
    float db = (float) b;
    float dc = (float) c;
    float gamma = (da*da + db*db - dc*dc)/(2.0*da*db);
    gamma = acos(gamma) * 180.0 / PI;
    return gamma;
}
