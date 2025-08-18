/** @file detscat_ddscat.h
 *  @brief DDSCAT data parser and structures.
 *
 *  This module provides data structures and functions for reading,
 *  storing, and managing parameters and results from DDSCAT simulation files.
 */
#ifndef DETSCAT_DDSCAT_H
#define DETSCAT_DDSCAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "mymath.h"

/** Maximum (expected) length of a line in DDSCAT files.
 *  @warning Lines longer than this will be truncated.
 */
#define DETSCAT_DDSCAT_LINE_MAX 1024

/** Expected maximum length of a parser error message.
 *  @warning Lines longer than this will be truncated.
 */
#define DETSCAT_DDSCAT_PARSER_ERR_MSG_MAX 256

/** Expected maximum component name length in a DDSCAT .par file.
 *  @warning Lines longer than this will be truncated.
 */
#define DETSCAT_DDSCAT_COMPONENTS_MAX 512

/** Number of scattering plane parameters in a DDSCAT .par file.
 *  Warning: Lines longer than this will be truncated.
 */
#define DETSCAT_DDSCAT_SCAT_PLANE_PARAMS 4

/**
 * @brief Identifier for a DDSCAT case
 *
 * A DDSCAT simulation case is defined by:
 * - Wavelength index
 * - Target size index
 * - Target orientation index
 */
typedef struct {
    int w;  /**< Wavelength index */
    int r;  /**< Target size index */
    int k;  /**< Target orientation index */
} DetScatDdscatCaseId;

/**
 * @brief Status codes for the DDSCAT parser
 */
typedef enum {
    DETSCAT_DDSCAT_PARSER_OK = 0,           /**< No error */
    DETSCAT_DDSCAT_PARSER_ERR_ALLOC,        /**< Memory allocation failed */
    DETSCAT_DDSCAT_PARSER_ERR_FORMAT,       /**< File format error */
    DETSCAT_DDSCAT_PARSER_ERR_RESET         /**< Parser reset failed */
} DetScatDdscatParserStatus;

/**
 * @brief DDSCAT file parser structure
 *
 * Maintains the state of the parser, current line, error messages, and file
 * handle.
 */
typedef struct {
    FILE *file;                          /**< File handle */
    int line_number;                     /**< Current line number */
    DetScatDdscatParserStatus status;    /**< Last parser status */
    bool eof;                            /**< End-of-file flag */
    char line[DETSCAT_DDSCAT_LINE_MAX];  /**< Current line buffer */
    /** Error message buffer */
    char err_msg[DETSCAT_DDSCAT_PARSER_ERR_MSG_MAX];
} DetScatDdscatParser;

/**
 * @brief DDSCAT .par file parameters
 *
 * Holds the parameters specified in a DDSCAT .par file.
 */
typedef struct {
    ComplexVec3 e01;       /**< Incident light polarization basis vector */
    size_t n_components;   /**< Number of components forming the target */
    size_t n_scat_planes;  /**< Number of scattering planes */
    char **components;     /**< Component names */
    /** Scattering plane parameters */
    double (*scat_planes)[DETSCAT_DDSCAT_SCAT_PLANE_PARAMS];
} DetScatDdscatParams;

/**
 * @brief DDSCAT scattering matrix (F-matrix)
 *
 * Represents the scattering matrix for a single DDSCAT case (uniqe case ID)
 * and scattering plane at an azimuthal angle \f$\phi\f$.
 */
typedef struct {
    double phi;      /**< Azimuthal angle */
    size_t n_theta;  /**< Number of scattering angles, theta */
    Complex *f11;    /**< Scattering matrix component f11 */
    Complex *f21;    /**< Scattering matrix component f21 */ 
    Complex *f12;    /**< Scattering matrix component f12 */
    Complex *f22;    /**< Scattering matrix component f22 */
    double *theta;   /**< Scattering angles */
} DetScatDdscatFmatrix;

/**
 * @brief DDSCAT .fml file content
 *
 * Stores the F-matrices corresponding to each azimuthal scattering plane for a
 * given DDSCAT case, as specified in a DDSCAT .fml file.
 */
typedef struct {
    size_t n_fmats;
    DetScatDdscatFmatrix *fmats;
} DetScatDdscatFml;

/**
* @brief Complete DDSCAT dataset for DetScat
*
* Holds all DDSCAT parameter sets and F-matrices for every DDSCAT target and
* each specified DetScat particle. Includes an index map that links each
* parameter file (specific to a given target) to its corresponding particle
* instance (with specific size and orientation). 
*/
typedef struct {
    size_t n_pars;
    size_t n_fmls;
    size_t n_par_idx;
    DetScatDdscatParams *pars;
    DetScatDdscatFml *fmls;
    size_t *par_idx;
} DetScatDdscatData;


DetScatDdscatParser *detscat_ddscat_parser_create(const char *ddscat_file_path);

void detscat_ddscat_parser_free(DetScatDdscatParser *ddscat_parser);

bool detscat_ddscat_parser_reset(DetScatDdscatParser *ddscat_parser,
                                 const char *ddscat_file_path);

/**
 * @brief Parse a DDSCAT `.par` parameter file.
 *
 * Reads the DDSCAT `.par` file from the parser and populates
 * a `DetScatDdscatParams` structure.
 *
 * @param ddscat_parser Parser instance.
 * @param par Output `.par` file structure.
 * @return `true` on success, `false` on failure.
 */
bool detscat_ddscat_parser_parse_par(DetScatDdscatParser *ddscat_parser,
                                     DetScatDdscatParams *par);

/**
 * @brief Parse a `.fml` scattering matrix file.
 *
 * Reads the F-matrix list from the DDSCAT file and populates
 * a `DetScatDdscatFml` structure.
 *
 * @param ddscat_parser Parser instance.
 * @param fml Output `.fml` file structure.
 * @param par Associated `.par` file parameters.
 * @return `true` on success, `false` on failure.
 */
bool detscat_ddscat_parser_parse_fml(DetScatDdscatParser *ddscat_parser,
                                     DetScatDdscatFml *fml,
                                     DetScatDdscatParams *par);

/**
 * @brief Free memory for a DDSCAT parameter structure.
 *
 * Releases all dynamically allocated arrays inside the parameter set.
 *
 * @param par Parameter set to free.
 */
void detscat_ddscat_par_free(DetScatDdscatParams *par);

void detscat_ddscat_fml_free(DetScatDdscatFml *fml);

void detscat_ddscat_data_free(DetScatDdscatData *ddscat);

#endif  // DETSCAT_DDSCAT_H
