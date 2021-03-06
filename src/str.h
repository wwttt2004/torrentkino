/*
Copyright 2006 Aiko Barz

This file is part of torrentkino.

torrentkino is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

torrentkino is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with torrentkino.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STR_H
#define STR_H

#include "main.h"
#include "fail.h"

int str_isValidUTF8( char *string );
int str_isNumber( char *string );
int str_isSafePort( char *string );
/* int str_isHex( char *string ); */
int str_isValidFilename( char *string );
int str_isValidHostname( const char *hostname, int size );

/*void str_htmlDisarmInput( char *string );*/

int str_count( char *buffer, const char *search );

void str_GMTtime( char *buffer, int size );
void str_gmttime( char *buffer, int size, time_t timestamp );
void str_prettySize( char *buffer, int size, unsigned long filesize );

/* char *str_append( char *buf1, long int size1, char *buf2, long int size2 ); */

#endif
