/*
 * Copyright (C) 2008 Karl Wette
 * Copyright (C) 2005 Reinhard Prix
 *
 *  [partially based on the MSG_LOG class in BOINC:
 *  Copyright (C) 2005 University of California]
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with with program; see the file COPYING. If not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 */

/**
 * \author Reinhard Prix
 * \date 2005
 * \file
 * \brief General-purpose log-message handling, controlled by lalDebugLevel
 *
 * $Id$
 */

#ifndef _LOGPRINTF_H  /* Double-include protection. */
#define _LOGPRINTF_H

/* C++ protection. */
#ifdef  __cplusplus
extern "C" {
#endif

#include <lal/LALRCSID.h>
NRCSID( LOGPRINTFH, "$Id$" );

/*---------- INCLUDES ----------*/
#include <stdarg.h>

#include <gsl/gsl_matrix.h>

/*---------- DEFINES ----------*/
/*---------- TYPES ----------*/

/** Argument-type for LogPrintf(): determines log-level of this message */
typedef enum
  {
    LOG_CRITICAL = -1,
    LOG_NORMAL = 0,
    LOG_DEBUG  = 1,
    LOG_DETAIL = 2,
    LOG_LAST		/* don't use */
  } LogLevel_t;

/*---------- GLOBALs ----------*/

/*---------- PROTOTYPES [API] ----------*/
void LogSetFile(FILE* file);
void LogSetLevel(LogLevel_t level);

void LogPrintf (LogLevel_t, const char* format, ...);
void LogPrintfVerbatim (LogLevel_t, const char* format, ...);

int XLALfprintfGSLmatrix ( FILE *fp, const char *fmt, const gsl_matrix *gij );
int XLALfprintfGSLvector ( FILE *fp, const char *fmt, const gsl_vector *vect );
int XLALfprintfGSLvector_int ( FILE *fp, const char *fmt, const gsl_vector_int *vect );

char * XLALClearLinebreaks ( const char *str );

#ifdef  __cplusplus
}
#endif
/* C++ protection. */

#endif  /* Double-include protection. */