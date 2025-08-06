#ifndef DETSCAT_CONFIG_H
#define DETSCAT_CONFIG_H

#include <stdbool.h>
#include <stdio.h>

#include "mymath.h"

/**
 * @brief Specifies the maximum length, in bytes, of a line in the configuration file.
 * 
 * Specifies the maximum length, in bytes, of a line in the configuration file.
 */
#define DETSCAT_CONFIG_LINE_MAX 1024
/**
 * @brief Specifies the maximum length, in bytes, of a system path definition
 * in the configuration file.
 *
 * Specifies the maximum length, in bytes, of a system path definition in the
 * configuration file.
 */
#define DETSCAT_CONFIG_PATH_MAX 512
/**
 * @brief Specifies the maximum length, in bytes, of an error message produced
 * by the configuration file parser.
 *
 * Specifies the maximum length, in bytes, of an error message produced by the
 * configuration file parser.
 */
#define DETSCAT_CONFIG_ERR_MSG_MAX 256


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
     * Wavelength of the incident monochromatic light wave from a pulsed laser
     * source specified in [nm].
     */
    double wavelength_nm;
    /**
     * Pulse energy of the incident monochromatic light wave from a pulsed laser
     * source specified in [mJ].
     */
    double pulse_energy_mj;
    /**
     * Pulse width of the incident monochromatic light wave from a pulsed laser
     * source specified in [ns].
     */
    double pulse_width_ns;
    /**
     * Beam diameter of the incident monochromatic light wave from a pulsed
     * laser source specified in [mm].
     */
    double beam_diameter_mm;

    // Particles
    /**
     * System path to the particles definition file.
     */
    char particles_definition_file[DETSCAT_CONFIG_PATH_MAX];

    // Camera
    /**
     * 3D vector in world coordinate frame, pointing from the origin to the
     * camera center, with components specified in [m].
     */
    Vec3 camera_center_position_m;
    /**
     * 3D vector in world coordinate frame, normal to and pointing away from the
     * camera sensor. If not already done, the vector will be internally
     * normalized to a unit vector. The components are specified in [m].
     */
    Vec3 camera_sensor_normal_vector;
    /**
     * Camera sensor width specified in [mm].
     */
    double sensor_width_mm;
    /**
     * Camera sensor height specified in [mm].
     */
    double sensor_height_mm;
    /**
     * Camera focal length specified in [mm].
     */
    double focal_length_mm;
    /**
     * Camera sensor resolution in the x-direction (width) in [px].
     */
    int camera_resolution_x_px;
    /**
     * Camera sensor resolution in the y-direction (height) in [px].
     */
    int camera_resolution_y_px;

} DetScatConfig;

/**
 * `extern` declaration of configuration file struct instance to allow global access. 
 */
extern DetScatConfig config;

/**
 * @brief Configuration file parser status enum.
 *
 * This enum provides status codes for the configuration file parser.
 */
typedef enum {
    /**
     * The status is OK.
     */
    DETSCAT_CONFIG_PARSER_OK = 0,
    /**
     * The configuration file format is wrong. 
     */
    DETSCAT_CONFIG_PARSER_ERR_FORMAT,
    /**
     * Unknown configuration file parameter. 
     */
    DETSCAT_CONFIG_PARSER_ERR_UNKNOWN_KEY,
    /**
     * The value from a key-value pair is out of range.
     */
    DETSCAT_CONFIG_PARSER_ERR_VAL_RANGE
} DetScatConfigParserStatus;

/**
 * @brief Structure representing the configuration file parser.
 *
 * This struct defines a parser to parse the configuration file data.
 */
typedef struct {
    /**
     * Pointer to the onfiguration file. 
     */
    FILE *file;
    /**
     * Current line number of the configuration file that the configuration file
     * parser is processing. 
     */
    int line_number;
    /**
     * Current status of the configuration file parser.
     */
    DetScatConfigParserStatus status;
    /**
     * End-of-file flag, which is set to `true` if the parser successfully
     * processes the entire file.
     */
    bool eof;
    /**
     * Buffer holding the current line processed by the configuration file
     * parser.
     */
    char line[DETSCAT_CONFIG_LINE_MAX];
    /**
     * Error message providing details in case that the configuration file
     * parser fails to process a line.
     */
    char err_msg[DETSCAT_CONFIG_ERR_MSG_MAX];
} DetScatConfigParser;

/**
 * @brief Initializes the configuration file parser.
 *
 * This function opens the configuration file and initializes the
 * configuration file parser.
 *
 * @param file_path
 *   Path to the configuration file.
 * @return 
 *   Pointer to the configuration file parser, or 'NULL' if the function
 *   fails.
 */
DetScatConfigParser *detscat_config_parser_create(const char *cfg_file_path);

/** 
 * @brief Parses the configuration file.
 *
 * This function parses the configuration file content and save extracted data
 * into a data struct. It returns `true` on success, or `false` on error.
 *
 * @param parser 
 *   Pointer to the configuration file parser
 * @param config
 *   Pointer to the data struct storing the configuration file
 *   content.
 */
bool detscat_config_parser_parse(DetScatConfigParser *parser,
                                 DetScatConfig *config);

/** 
 * @brief Closes the configuration file and frees parser resources. 
 *
 * This function closes the configuration file and frees any unused
 * configuration file parser resources.
 * 
 * @param parser
 *   Pointer to the configuration file parser
 */
void detscat_config_parser_free(DetScatConfigParser *parser);

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
 *                                  // light wave from a pulsed laser source
 *                                  // specified in [nm].
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
 *                                  // x-direction (width) in [px].
 *
 * - camera_resolution_y_px         // Camera sensor resolution in the
 *                                  // y-direction (height) in [px].
 * @endcode
 *
 * @see detscat_config.h
 */
