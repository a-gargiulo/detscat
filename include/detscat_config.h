/**
 * @file        detscat_config.h 
 * @brief       Configuration file parser and data container.
 *
 * This module provides functionality for parsing the main user input
 * configuration file. It also defines a structured data container for
 * storing the parsed configuration parameters.
 *
 * @author      Aldo Gargiulo
 * @author      Postdoctoral Research Associate
 * @author      Aerospace Research Laboratory
 * @author      University of Virginia
 * @author      Department of Mechanical and Aerospace Engineering
 * @author      570 Edgemont Rd.
 * @author      Charlottesville, VA 22903
 * @author      bzc6rs@virginia.edu
 *
 * @version     1.0
 * @date        2025-08-02
 */
#ifndef DETSCAT_CONFIG_H
#define DETSCAT_CONFIG_H

#include <stdbool.h>
#include <stdio.h>

#include "mymath.h"

#define DETSCAT_CONFIG_LINE_MAX 1024
#define DETSCAT_CONFIG_PATH_MAX 512
#define DETSCAT_CONFIG_ERR_MSG_MAX 256


/**
 * @brief Structured data container for configuration file parameters.
 *
 * This struct stores all required user input parameters, as defined in the
 * configuration file. It serves as a central container for accessing parsed
 * configuration values throughout the application.
 *
 * See @ref config_file_format for details on the expected configuration format.
 */
typedef struct {

    // Incident light source -- pulsed laser
    /**
     * Boolean value indicating whether the incident light is polarized. If set
     * to `true`, the light is polarized; if `false`, it is unpolarized.
     */
    bool is_polarized;
    /**
     * 3D complex vector specifying the polarization direction of the incident
     * light. 
     */
    ComplexVec3 polarization;
    /**
     *
     */
    double wavelength_nm;
    /**
     *
     */
    double pulse_energy_mj;
    double pulse_width_ns;
    double beam_diameter_mm;

    // Particles
    char particles_file[DETSCAT_CONFIG_PATH_MAX];   // Particles file path

    // Camera
    Vec3 camera_center_position_m;
    Vec3 camera_sensor_normal_vector;
    double sensor_width_mm;
    double sensor_height_mm;
    double focal_length_mm;
    int camera_resolution_x_px;
    int camera_resolution_y_px;

} DetScatConfig;

extern DetScatConfig config;


// CONFIG FILE PARSER 
// -----------------------------------------------------------------------------
typedef enum {
    DETSCAT_CONFIG_PARSER_OK = 0,
    DETSCAT_CONFIG_PARSER_ERR_FILE_NOT_FOUND,
    DETSCAT_CONFIG_PARSER_ERR_ALLOC,
    DETSCAT_CONFIG_PARSER_ERR_INVALID_ARG,
    DETSCAT_CONFIG_PARSER_ERR_FORMAT,
    DETSCAT_CONFIG_PARSER_ERR_UNKNOWN_KEY
} DetScatConfigParserStatus;

typedef struct {
    FILE *file;                                     // Config file path
    int line_number;                                // Current line number
    DetScatConfigParserStatus status;               // Parser status code
    bool eof;                                       // End-of-file flag
    char line[DETSCAT_CONFIG_LINE_MAX];             // Current line buffer
    char err_msg[DETSCAT_CONFIG_ERR_MSG_MAX];       // Last error message
} DetScatConfigParser;


// METHODS 
// -----------------------------------------------------------------------------

DetScatConfigParser *detscat_config_parser_create(const char *file_path);
/**
 * @brief Opens and initializes the configuration file parser.
 *
 * @param file_path Path to the configuration file.
 * @return Pointer to the configuration parser, or 'NULL' if the function fails.
 */

bool detscat_config_parser_parse(DetScatConfigParser *parser,
                                 DetScatConfig *config);
/* Parses file into the config struct.
 *
 * It returns `true` on success, or `false` on error.
 */

void detscat_config_parser_free(DetScatConfigParser *parser);
// Close file and free parser memory.

#endif  // DETSCAT_CONFIG_H

/** 
 * @page config_file_format Configuration File Format
 *
 * ## Overview
 * The configuration file defines the user input parameters for DetScat.
 * It uses a simple key-value format:
 *
 * ```
 * is_polarized               =  true 
 *
 * polarization               =  [[0, 0], [1, 0], [0, 0]]
 *
 * wavelength_nm              =  532.0
 *
 * pulse_energy_mj            =  1.3e-03 
 *
 * particles_definition_file  =  ../resources/particles.dat
 *
 * camera_center_position_m   =  [1.5, 0, 0]
 *
 * ...
 * ```
 *
 * ## Important Notes
 * - Arrays are defined using square brackets (`[[...], ...]`) with
 *   comma-separated values.
 * - Lines starting with `#` are treated as comments and ignored.
 * - All parameter names are case-sensitive.
 * - Arbitrary whitespace is allowed between keys, values, and delimiters.
 * - Blank lines between key-value definitions are allowed.
 * - Do not enclose strings in `"..."` or ```'...'```.
 *
 * ## Required Parameters
 *
 * ### Incident Light Source - Pulsed Laser Source
 * @code
 * - is_polarized                   // Boolean value indicating whether the
 *                                  // incident light is polarized. If set to
 *                                  // `true`, the light is polarized; if
 *                                  // `false`, it is unpolarized.
 *
 * - polarization                   // 3D complex vector specifying the
 *                                  // polarization direction of the incident
 *                                  // light. 
 *
 * - wavelength_nm                  // Wavelength of the incident monochromatic
 *                                  // incident light wave from a pulsed laser
 *                                  // source specified in [nm].
 *
 * - pulse_energy_mj                // Pulse energy of the incident
 *                                  // monochromatic light wave from a pulsed
 *                                  // laser source specified in [mJ].  
 *
 * - pulse_width_ns                 // Pulse width of the incident
 *                                  // monochromatic light wave from a pulsed
 *                                  // laser source specified in [ns].  
 *
 * - beam_diameter_mm               // Beam diameter of the incident
 *                                  // monochromatic light wave from a pulsed
 *                                  // laser source specified in [mm].  
 * @endcode
 *
 * ### Particles
 * @code
 * - particles_definition_file      // System path to the particles definition
 *                                  // file.
 * @endcode
 *
 * ### Camera - Pinhole Camera Model
 * @code
 * - camera_center_position_m       // 3D vector in world coordinate frame,
 *                                  // pointing from the origin to the camera
 *                                  // center, with components specified in [m].
 *
 * - camera_sensor_normal_vector    // 3D vector in world coordinate frame,
 *                                  // normal to and pointing away from the
 *                                  // camera sensor. If not already done, the
 *                                  // vector will be internally normalized to a
 *                                  // unit vector. The components are specified
 *                                  // in [m].
 *    
 * - sensor_width_mm                // Camera sensor width specified in [mm].
 *
 * - sensor_height_mm               // Camera sensor height specified in [mm].
 *
 * - focal_length_mm                // Camera focal length specified in [mm].
 *
 * - camera_resolution_x_px         // Camera sensor resolution in the
 *                                  // x-direction(width) in [px].
 *
 * - camera_resolution_y_px         // Camera sensor resolution in the
 *                                  // y-direction (height) in [px].
 * @endcode
 *
 * @see detscat_config.h
 */
