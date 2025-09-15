#ifndef DETSCAT_LIMITS_H
#define DETSCAT_LIMITS_H

#include <stddef.h>

static const size_t DETSCAT_CFG_PATH_INIT = 512;
static const size_t DETSCAT_CFG_PATH_MAX = 4096;
static const size_t DETSCAT_CFG_STRVAR_INIT = 16;
static const size_t DETSCAT_CFG_STRVAR_MAX = 128;

static const size_t DETSCAT_PARSER_LINE_INIT = 1024;
static const size_t DETSCAT_PARSER_LINE_MAX = 65536;

static const size_t DETSCAT_PRT_PATH_INIT = 512;
static const size_t DETSCAT_PRT_PATH_MAX = 4096;
static const size_t DETSCAT_PRT_TYPEID_INIT = 16;
static const size_t DETSCAT_PRT_TYPEID_MAX = 128;

static const size_t DETSCAT_DDSCAT_PATH_INIT = 512;
static const size_t DETSCAT_DDSCAT_PATH_MAX = 4096;

#endif  // DETSCAT_LIMITS_H
