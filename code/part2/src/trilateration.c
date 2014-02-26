/*#include "test_tri/test.c"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>*/
#include "trilateration.h"
#include <float.h>

#define PI (4.0 * atan(1.0))
#define IX(a,b) (3 * (a) + (b))

float angled(int a, int b, int c)
{
    float da = (float) a;
    float db = (float) b;
    float dc = (float) c;
    float gamma = (da*da + db*db - dc*dc)/(2.0*da*db);
    return (float) acos(gamma);// * 180.0 / PI;
}

int anglei(int a, int b, int c)
{
    return (int) angled(a,b,c);
}

static float vnorm(const float *v)
{
    float result = 0.0f;
    for(int i = 0; i < 3; ++i)
        result += v[i] * v[i];
    return sqrt(result);
}

static float vnormc(const struct coordinates *c)
{
    float result = c->x *c->x + c->y * c->y;
    return sqrt(result);
}

static void vnormalize(float *v)
{
    float norm_v = vnorm(v);
    for(int i = 0; i < 3 ; ++i)
        v[i] /= norm_v;
}

static void cross_prod(float *a, float *b, float *r)
{
    r[0] = (a[1] * b[2]) - (a[2] * b[1]);
    r[1] = (a[2] * b[0]) - (a[0] * b[2]);
    r[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

static void calcR(float *s1, float *m1, float *s2, float *m2, float *r)
{
    float sm1[3];
    float sm2[3];
    cross_prod(s1, m1, sm1);
    cross_prod(s2, m2, sm2);
    r[IX(0,0)] = s1[0] * s2[0] + m1[0] * m2[0] + sm1[0] * sm2[0];
    r[IX(0,1)] = s1[0] * s2[1] + m1[0] * m2[1] + sm1[0] * sm2[1];
    r[IX(0,2)] = s1[0] * s2[2] + m1[0] * m2[2] + sm1[0] * sm2[2];
    r[IX(1,0)] = s1[1] * s2[0] + m1[1] * m2[0] + sm1[1] * sm2[0];
    r[IX(1,1)] = s1[1] * s2[1] + m1[1] * m2[1] + sm1[1] * sm2[1];
    r[IX(1,2)] = s1[1] * s2[2] + m1[1] * m2[2] + sm1[1] * sm2[2];
    r[IX(2,0)] = s1[2] * s2[0] + m1[2] * m2[0] + sm1[2] * sm2[0];
    r[IX(2,1)] = s1[2] * s2[1] + m1[2] * m2[1] + sm1[2] * sm2[1];
    r[IX(2,2)] = s1[2] * s2[2] + m1[2] * m2[2] + sm1[2] * sm2[2];
}

void trilaterate(struct local_bots *lb)
{
    if(lb->fl->c.y == 0 && lb->el->c.y != 0) {
        struct location *templ = lb->fl;
        lb->fl = lb->el;
        lb->el = templ;
        struct neighbour *tempn = lb->e;
        lb->f = lb->e;
        lb->e = tempn;
    } else if(lb->fl->c.y == COORD_UINIT || lb->el->c.y == COORD_UINIT) {
        error_state(TRI_LBS_NOT_LOC);
    }
    float d_az2;
    float d_ez2;
    float d_ae;
    float d_ae2;
    float d_fz2;
    float ex = (float) lb->el->c.x;
    float ey = (float) lb->el->c.y;
    float fx = (float) lb->fl->c.x;
    float fy = (float) lb->fl->c.y;

    float theta = 0.0f;
    if(ex < 0.0f) {
        theta = acos(ex / vnormc(&lb->el->c));
        float x_old = ex;
        ex = ex * cos(theta) - ey * sin(theta);
        ey = x_old * sin(theta) - ey * cos(theta);
    }
    d_az2 = (float) (lb->s->n->dist * lb->s->n->dist);
    d_ez2 = (float) (lb->e->dist * lb->e->dist);
    d_fz2 = (float) (lb->f->dist * lb->f->dist);

    d_ae = vnormc(&lb->el->c);
    d_ae2 = d_ae * d_ae;
    float x = ((d_az2 - d_ez2 + d_ae2) / (2.0f * d_ae));
    float y = (d_az2 - d_fz2 + (fx *fx) + (fy * fy)) / (2.0f * fy);
    y -= (fx * ((d_az2 - d_ez2 + d_ae2))) / (fy * 2.0f * d_ae);

    if(theta != 0.0f) {
        float x_old = x;
        x = x * cos(theta) - y * sin(theta);
        y = x_old * sin(theta) - y * cos(theta);
    }
    struct list_node *ln = exists(get_locations(), lb->s, &comp_seeder_location);
    if(ln == 0) {
        error_state(TRI_SEEDER_NO_MATCH);
        return;
    }
    struct location *l = (struct location *) ln->data;
    l->c.x = (int8_t) round(x);
    l->c.y = (int8_t) round(y);
}

static void r_to_q(float *r, float *q)
{
    q[0] = (1.0 + r[IX(0,0)] + r[IX(1,1)] + r[IX(2,2)]) / 4.0;
    if(q[0] > DBL_EPSILON) {
        q[0] = sqrt(q[0]);
        float wt4 = q[0] * 4.0;
        q[1] = (r[IX(2,1)] - r[IX(1,2)]) / wt4;
        q[2] = (r[IX(0,2)] - r[IX(2,0)]) / wt4;
        q[3] = (r[IX(1,0)] - r[IX(0,1)]) / wt4;
    } else {
        q[0] = 0.0;
        q[1] = (r[IX(1,1)] + r[IX(2,2)]) / (-2.0);
        if(q[1] > DBL_EPSILON) {
            q[1] = sqrt(q[1]);
            float xt2 = q[1] * 2.0;
            q[2] = r[IX(0,1)] / xt2;
            q[3] = r[IX(0,2)] / xt2;
        } else {
            q[1] = 0.0;
            q[2] = (1 - r[IX(2,2)]) / 2.0;
            if(q[2] > DBL_EPSILON) {
                q[2] = sqrt(q[2]);
                q[3] = r[IX(1,2)] / (2.0 * q[2]);
            } else {
                q[2] = 0.0;
                q[3] = 1.0;
            }
        }
    }
}

static void slerp(float *q, union quaternion *rq)
{
    float r[4];
    if(1.0 - q[0] <= DBL_EPSILON) {
        for(int i = 0; i < 4; ++i)
            r[i] = q[i] * 0.5;
        r[0] += 0.5;
    } else if(1.0 + q[0] <= DBL_EPSILON) {
        for(int i = 0; i < 4; ++i)
            r[i] = q[i] * sin(PI / 2);
        r[3] -= (sin(PI / 2));
    } else {
        float theta = acos(q[0]);
        float s = sin(theta / 2)/sin(theta);
        for(int i = 0; i < 4; ++i)
            r[i] = q[i] * s;
        r[0] += s;
    }

    rq->q16b[0] = (int16_t) round((r[0] * 1000));
    rq->q16b[1] = (int16_t) round((r[1] * 1000));
    rq->q16b[2] = (int16_t) round((r[2] * 1000));
    rq->q16b[3] = (int16_t) round((r[3] * 1000));
}

void triad2(float *ab1, float *ac1, float *ab2, float *ac2, float *rq)
{
    float m1[3];
    float m2[3];
    float r[3 * 3];
    cross_prod(ab1, ac1, m1);
    cross_prod(ab2, ac2, m2);
    vnormalize(m1);
    vnormalize(m2);
    vnormalize(ab1);
    vnormalize(ab2);
    free(ac1);
    free(ac2);
    calcR(ab1, m1, ab2, m2, r);
    r_to_q(r, rq);
}

void rotate_w_quat(float *v, union quaternion *q)
{
    float temp[4];
    //multiply with q
    temp[0] = (-v[0]*q->q16b[1] - v[1]*q->q16b[2] - v[2]*q->q16b[3]) / 1000;
    temp[1] =  (v[0]*q->q16b[0] + v[1]*q->q16b[3] - v[2]*q->q16b[2]) / 1000;
    temp[2] = (-v[0]*q->q16b[3] + v[1]*q->q16b[0] + v[2]*q->q16b[1]) / 1000;
    temp[3] =  (v[0]*q->q16b[2] - v[1]*q->q16b[1] + v[2]*q->q16b[0]) / 1000;
    //multiply with q^-1
    v[0] = (temp[0]*(-q->q16b[1]) + temp[1]*q->q16b[0] + temp[2]*(-q->q16b[3]) - temp[3]*(-q->q16b[2])) / 1000;
    v[1] = (temp[0]*(-q->q16b[2]) - temp[1]*(-q->q16b[3]) + temp[2]*q->q16b[0] + temp[3]*(-q->q16b[1])) / 1000;
    v[2] = (temp[0]*(-q->q16b[3]) + temp[1]*(-q->q16b[2]) - temp[2]*(-q->q16b[1]) + temp[3]*q->q16b[0]) / 1000;
}

void get_vector(struct location *a, struct coordinates *b, float *v)
{
    v[0] = (float) (b->x - a->c.x);
    v[1] = (float) (b->y - a->c.y);
    v[2] = 0.0;
    rotate_w_quat(v, &a->s->q);
}

void calculate_new_v_q(struct location *mergee, struct location *s2,
                       struct coordinates *c1_a, struct coordinates *c2_a,
                       struct coordinates *c1_b, struct coordinates *c2_b)
{
    float v1az[3];
    float v2az[3];
    float v1bz[3];
    float v2bz[3];
    float rq[4];
    get_vector(mergee, c1_a, v1az);
    get_vector(s2, c2_a, v2az);
    get_vector(mergee, c1_b, v1bz);
    get_vector(s2, c2_b, v2bz);
    triad2(v1az, v1bz, v2az, v2bz, rq);
    slerp(rq, &mergee->s->q);

}

