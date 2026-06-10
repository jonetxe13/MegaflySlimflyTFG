/**
* @file
* @brief	Tools for reading simulation parameter.
*
* This tools are used for read the fsin.conf file & the command line parameters(arguments)
* of the simulation & for printing the final summary.
*
* @see get_conf.c
* @see print_results.c

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

#include "literal.h"
#include "misc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
* Searches for a string in a literal array.
*
* This function takes an array of literals and finds the value corresponding to a string.
*
* @param l array of literals.
* @param name the string we are searching for.
* @param value the corresponding value for the string is returned here.
* @return if 'name' is in 'l' TRUE, else FALSE.
* @see get_conf.c
*/
bool_t literal_value(literal_t * l, char * name, int * value)
{
	while(l->name) {
		if(!strncmp(name, l->name, strlen(name))) {
			*value = l->value;
			return TRUE;
		}
		l += 1;
	}
	return FALSE;
}

/**
* Searches for a value in a literal array.
*
* This function takes an array of literals and finds the string corresponding to a value.
*
* @param l an array of literals.
* @param name the corresponding string is returned here.
* @param value the value we are searching for.
* @return if 'value' is in 'l' TRUE, else FALSE.
* @see print_results.c
*/
bool_t literal_name(literal_t * l, char ** name, int value)
{
	while(l->name) {
		if(value == l->value) {
			*name = l->name;
			return TRUE;
		}
		l += 1;
	}
	return FALSE;
}

/**
* Aborts the simulation & prints an error message.
* Should only be used when an insolvable error is encountered, otherwise should use abort (), which completes the whole execution rather than killing the program.
*
* When the simulation ends here the return code is -1.
*
* @param msg A string containing the error message
*/
void panic(char * msg) {
    fprintf(stderr, "******************************************************************************************************\n");
        fprintf(stderr, "* PANIC: %-91s *\n", msg);
        fprintf(stderr, "******************************************************************************************************\n");
        exit(-1);
}


/**
* Allocates memory.
*
* @param size The size in bytes of the memory allocation.
*/
void * alloc(long size) {

        void * res;
        
        if((res = malloc(size)) == NULL)
                panic("alloc: Unable to allocate memory");
        return res;
}
