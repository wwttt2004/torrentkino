/*
Copyright 2006 Aiko Barz

This file is part of masala/vinegar.

masala/vinegar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/vinegar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/vinegar.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef VINEGAR
void log_complex(struct obj_nodeItem *nodeItem, int code, const char *buffer);
void log_info(int code, const char *buffer);
#endif

#ifdef MASALA
void log_udp(CIPV6 *c_addr, const char *buffer);
void log_info(const char *buffer);
#endif

void log_simple(const char *buffer);
void log_fail(const char *buffer);
void log_memfail(const char *buffer, const char *caller);