#include "particles.h"

#include <stdlib.h>
#include <strings.h>

#include "strutil.h"

ParticleParser *detscat_particles_parser_init(const char *particles_file_path)
{
    ParticleParser *parser = malloc(sizeof(ParticleParser));
    if (!parser)
    {
        return NULL;
    }

    parser->file = fopen(particles_file_path, "r");
    if (!parser->file)
    {
        free(parser);
        return NULL;
    }

    parser->line_number = 0;
    parser->eof = false;
    parser->error_code = DETSCAT_PARTICLE_PARSER_OK;
    parser->line[0] = '\0';
    parser->error_message[0] = '\0';

    return parser;
}

void detscat_particles_parser_free(ParticleParser *parser)
{
    if (!parser)
        return;

    if (parser->file)
    {
        fclose(parser->file);
        parser->file = NULL;
    }

    free(parser);
}


static void free_types(Particles *particles, size_t count)
{
    if (!particles || !particles->types) return;
    for (size_t i = 0; i < count; ++i)
    {
        free(particles->types[i].type);
        free(particles->types[i].data_path);
    }
    free(particles->types);
    particles->types = NULL;
}

static void free_particles(Particles *particles, size_t count)
{
    if (!particles || !particles->particles) return;
    for (size_t i = 0; i < count; ++i)
    {
        free(particles->particles[i].type);
    }
    free(particles->particles);
    particles->particles = NULL;
}



bool detscat_particles_parser_parse(ParticleParser *parser, Particles *particles)
{
    if (!parser || !particles)
    {
        if (parser)
        {
            parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_ARGUMENT;
            snprintf(parser->error_message, sizeof(parser->error_message),
                     "Internal error: invalid argument to "
                     "'detscat_particles_parser_parse'");
        }
        return false;
    }

    memset(particles, 0, sizeof(*particles));

    bool type_def_sec = false;
    bool particles_sec = false;

    while (fgets(parser->line, sizeof(parser->line), parser->file))
    {
        parser->line_number++;

        char *trimmed = strutil_trim(parser->line);
        if (trimmed[0] == '\0' || trimmed[0] == '#')
            continue;

        if (strcmp(trimmed, "$(StartTypeDef)") == 0)
        {
            type_def_sec = true;
            bool n_types_parsed = false;
            size_t types_allocated = 0;
            bool end_type_def_found = false;

            while (fgets(parser->line, sizeof(parser->line), parser->file))
            {
                parser->line_number++;

                char *trimmed = strutil_trim(parser->line);
                if (trimmed[0] == '\0' || trimmed[0] == '#')
                    continue;

                if (strcmp(trimmed, "$(EndTypeDef)") == 0)
                {
                    end_type_def_found = true;
                    break;
                }

                if (!n_types_parsed)
                {
                    if(sscanf(trimmed, " %zu ", &particles->n_types) != 1)
                    {
                        parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
                        snprintf(parser->error_message, sizeof(parser->error_message),
                                 "Missing number of particle types in $(StartTypeDef) on line %d", parser->line_number);
                        return false;
                    }

                    particles->types = malloc(particles->n_types * sizeof(ParticleType));
                    if (!particles->types)
                    {
                        parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_ALLOCATION_FAILED;
                        snprintf(parser->error_message, sizeof(parser->error_message),
                                 "Could not allocate the particle type array");
                        return false;
                    }
                    memset(particles->types, 0, particles->n_types * sizeof(ParticleType));

                    n_types_parsed = true;
                    continue;
                }

                char tmp_type[64];
                char tmp_path[1024];

                if (sscanf(trimmed, " %63s %1023s ", tmp_type, tmp_path) != 2)
                {
                    parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Invalid particle type definition on line %d", parser->line_number);
                    free_types(particles, types_allocated);
                    return false;
                }

                particles->types[types_allocated].type = strdup(tmp_type);
                particles->types[types_allocated].data_path = strdup(tmp_path);

                if (!particles->types[types_allocated].type || !particles->types[types_allocated].data_path)
                {
                    parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_ALLOCATION_FAILED;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Could not allocate the particle type on line %d", parser->line_number);
                    if (particles->types[types_allocated].type)
                        free(particles->types[types_allocated].type);
                    if (particles->types[types_allocated].data_path)
                        free(particles->types[types_allocated].data_path);
                    free_types(particles, types_allocated);
                    return false;
                }

                types_allocated++;

                if (types_allocated > particles->n_types)
                {
                    parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Expected %zu particle types, got %zu", particles->n_types, types_allocated);
                    free_types(particles, types_allocated);
                    return false;
                }

            }

            if (!end_type_def_found)
            {
                parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Missing $(EndTypeDef) after line %d", parser->line_number);
                free_types(particles, types_allocated);
                return false;
            }

            if (types_allocated != particles->n_types)
            {
                parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Expected %zu particle types, got %zu", particles->n_types, types_allocated);
                free_types(particles, types_allocated);
                return false;
            }
        }

        else if (strcmp(trimmed, "$(StartParticles)") == 0)
        {
            particles_sec = true;
            bool n_particles_parsed = false;
            size_t particles_allocated = 0;
            bool end_particles_found = false;

            while (fgets(parser->line, sizeof(parser->line), parser->file))
            {
                parser->line_number++;

                char *trimmed = strutil_trim(parser->line);
                if (trimmed[0] == '\0' || trimmed[0] == '#')
                    continue;

                if (strcmp(trimmed, "$(EndParticles)") == 0)
                {
                    end_particles_found = true;
                    break;
                }

                if (!n_particles_parsed)
                {
                    if(sscanf(trimmed, " %zu ", &particles->n_particles) != 1)
                    {
                        parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
                        snprintf(parser->error_message, sizeof(parser->error_message),
                                 "Missing number of particles in $(StartParticles) on line %d", parser->line_number);
                        return false;
                    }

                    particles->particles = malloc(particles->n_particles * sizeof(Particle));
                    if (!particles->particles)
                    {
                        parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_ALLOCATION_FAILED;
                        snprintf(parser->error_message, sizeof(parser->error_message),
                                 "Could not allocate the particles array");
                        return false;
                    }
                    memset(particles->particles, 0, particles->n_particles * sizeof(Particle));

                    n_particles_parsed = true;
                    continue;
                }

                char tmp_type[64];
                double tmp_x, tmp_y, tmp_z;

                if (sscanf(trimmed, " %63s %lf %lf %lf ", tmp_type, &tmp_x, &tmp_y, &tmp_z) != 4)
                {
                    parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Invalid particle definition on line %d", parser->line_number);
                    free_particles(particles, particles_allocated);
                    return false;
                }

                particles->particles[particles_allocated].type = strdup(tmp_type);
                particles->particles[particles_allocated].position.x = tmp_x;
                particles->particles[particles_allocated].position.y = tmp_y;
                particles->particles[particles_allocated].position.z = tmp_z;

                if (!particles->particles[particles_allocated].type)
                {
                    parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_ALLOCATION_FAILED;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Could not allocate the particle on line %d", parser->line_number);
                    free_particles(particles, particles_allocated);
                    return false;
                }

                particles_allocated++;

                if (particles_allocated > particles->n_particles)
                {
                    parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Too many particles defined at line %d", parser->line_number);
                    free_particles(particles, particles_allocated);
                    return false;
                }
            }


            if (!end_particles_found)
            {
                parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Missing $(EndParticles) after line %d", parser->line_number);
                free_particles(particles, particles_allocated);
                return false;
            }

            if (particles_allocated != particles->n_particles)
            {
                parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Expected %zu particles, got %zu", particles->n_particles, particles_allocated);
                free_particles(particles, particles_allocated);
                return false;
            }
        }
    }

    parser->eof = true;

    if (!type_def_sec || !particles_sec)
    {
        parser->error_code = DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT;
        snprintf(parser->error_message, sizeof(parser->error_message),
                 "Missing section(s): %s%s",
                 type_def_sec ? "" : "$(StartTypeDef) ",
                 particles_sec ? "" : "$(StartParticles)");
        if (particles->types)
            free_types(particles, particles->n_types);
        if (particles->particles)
            free_particles(particles, particles->n_particles);
        return false;
    }

    parser->error_code = DETSCAT_PARTICLE_PARSER_OK;
    return true;
}


void detscat_particles_free(Particles *particles) {
    if (!particles) return;

    if (particles->types) {
        for (size_t i = 0; i < particles->n_types; ++i) {
            free(particles->types[i].type);
            free(particles->types[i].data_path);
        }
        free(particles->types);
        particles->types = NULL;
    }

    if (particles->particles) {
        for (size_t i = 0; i < particles->n_particles; ++i) {
            free(particles->particles[i].type);
        }
        free(particles->particles);
        particles->particles = NULL;
    }

    particles->n_types = 0;
    particles->n_particles = 0;
}


