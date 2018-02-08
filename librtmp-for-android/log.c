/*
 *  Copyright (C) 2008-2009 Andrej Stepanchuk
 *  Copyright (C) 2009-2010 Howard Chu
 *
 *  This file is part of librtmp.
 *
 *  librtmp is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1,
 *  or (at your option) any later version.
 *
 *  librtmp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with librtmp see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/lgpl.html
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "log.h"

static const char hexdig[ ] = "0123456789abcdef";
static RTMP_LogCallback *cb = 0;
static RTMP_LogLevel RTMP_debuglevel = 0;

void RTMP_LogSetCallback(int level, RTMP_LogCallback *cbp)
{
	RTMP_debuglevel = level;
	cb = cbp;
}

void RTMP_Log(int level, const char *format, ...)
{
	if( cb == 0 ||
		RTMP_debuglevel > level ) return;

	va_list args;

	va_start(args, format);
	cb(level, format, args);
	va_end(args);
}

void RTMP_LogHex(int level, const uint8_t *data, unsigned long len)
{
	if( cb == 0 ||
		RTMP_debuglevel > level ) return;
	
	unsigned long i;
	char line[50], *ptr;

	ptr = line;

	for(i=0; i<len; i++) {
		*ptr++ = hexdig[0x0f & (data[i] >> 4)];
		*ptr++ = hexdig[0x0f & data[i]];
		if ((i & 0x0f) == 0x0f) {
			*ptr = '\0';
			ptr = line;
			RTMP_Log(level, "%s", line);
		} else {
			*ptr++ = ' ';
		}
	}
	if (i & 0x0f) {
		*ptr = '\0';
		RTMP_Log(level, "%s", line);
	}
}

void RTMP_LogHexString(int level, const uint8_t *data, unsigned long len)
{
	if( cb == 0 ||
		RTMP_debuglevel > level ) return;
	
#define BP_OFFSET 9
#define BP_GRAPH 60
#define BP_LEN	80
	char	line[BP_LEN];
	unsigned long i;

	/* in case len is zero */
	line[0] = '\0';

	for ( i = 0 ; i < len ; i++ ) {
		int n = i % 16;
		unsigned off;

		if( !n ) {
			if( i ) RTMP_Log( level, "%s", line );
			memset( line, ' ', sizeof(line)-2 );
			line[sizeof(line)-2] = '\0';

			off = i % 0x0ffffU;

			line[2] = hexdig[0x0f & (off >> 12)];
			line[3] = hexdig[0x0f & (off >>  8)];
			line[4] = hexdig[0x0f & (off >>  4)];
			line[5] = hexdig[0x0f & off];
			line[6] = ':';
		}

		off = BP_OFFSET + n*3 + ((n >= 8)?1:0);
		line[off] = hexdig[0x0f & ( data[i] >> 4 )];
		line[off+1] = hexdig[0x0f & data[i]];

		off = BP_GRAPH + n + ((n >= 8)?1:0);

		if ( isprint( data[i] )) {
			line[BP_GRAPH + n] = data[i];
		} else {
			line[BP_GRAPH + n] = '.';
		}
	}

	RTMP_Log( level, "%s", line );
}
