/*
Copyright 2010 Aiko Barz

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

#ifndef MAIN_H
#define MAIN_H

#include "main.h"
#include "ben.h"
#include "tksrc.h"

int str_isValidHostname( const char *hostname, int size );
int torrentkino_lookup( const char *hostname, int size );
void torrenkino_print6( struct sockaddr_in6 *sin );
void torrenkino_print( struct sockaddr_in *sin );


#endif
