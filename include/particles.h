#ifndef PARTICLES_H
#define PARTICLES_H

#include <stddef.h>


typedef struct {
    size_t n_types;
    size_t n_particles;
    const char **types;
    const char **types_scatdat;

} Particles;



#endif  // PARTICLES_H
