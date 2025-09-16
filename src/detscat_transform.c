#include "detscat_math.h"
#include "detscat_prt.h"


void detscat_prt_transform(DetScatPrtParticle *particles, size_t n,
                           const Vec3 *origin, const Mat3 *rotmat)
{
    assert(particles && origin && rotmat);

    for (size_t i = 0; i < n; ++i) {
        Vec3 tmp;
        detscat_math_vec3_sub(&tmp, &particles[i].position, origin);
        detscat_math_mat3_vec3_mult(&particles[i].position, rotmat, &tmp);
    }
}
