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

        detscat_math_mat3_vec3_mult(&particles[i].orientation.a1, rotmat, &particles[i].orientation.a1);
        detscat_math_mat3_vec3_mult(&particles[i].orientation.a2, rotmat, &particles[i].orientation.a2);

        detscat_math_vec3_orientation_to_angles(
            &particles[i].orientation.a1,
            &particles[i].orientation.a2,
            &particles[i].orientation.theta,
            &particles[i].orientation.beta,
            &particles[i].orientation.phi);
    }
}
