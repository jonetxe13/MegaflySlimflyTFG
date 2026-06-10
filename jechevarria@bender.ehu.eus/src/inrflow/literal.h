/**
* @file
* @brief	Definition of a literal that links a string to a number.
*
* This tools are used for read the fsin.conf file & the command line parameters(arguments)
* of the simulation & print them in the final brief.
*
* @see get_conf.c

FSIN Functional Simulator of Interconnection Networks
Copyright (2003-2011) J. Miguel-Alonso, A. Gonzalez

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "misc.h"

#ifndef _literal
#define _literal

/**
* Structure to contain a literal.
* This is a bijective union between a string & a number.
*
* @see get_conf.c
* @see print_results.c
*/
typedef struct literal_t {
	int value;
	char * name;
} literal_t;

#define LITERAL_END { 0, NULL } ///< This must be the last literal in an array.

// Some declarations
bool_t literal_value(literal_t * literal, char *  name, int * value);

bool_t literal_name (literal_t * literal, char ** name, int   value);

void panic(char * msg);

void * alloc(long);
#endif /* _literal */
