#include "../inrflow/globals.h"
#include "../inrflow/applications.h"
#include "../inrflow/gen_trace.h"
#include "trace.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
* The trace reader dispatcher selects the format type and calls to the correct trace read.
*
* The selection reads the first character in the file. This could be:
* 'c', 's' or 'r' for fsin trc, and '-' for alog (in complete trace the header is "-1",
* or in filtered trace could be "-101" / "-102").
*
*@param app            The data structure storing the application information
*
*@see read_fsin_trc
*@see read_alog
*/
void read_trc(application *app) {
	FILE * ftrc;
	char c;

	if((ftrc = fopen(app->pattern_file, "r")) == NULL){
		printf("Trace file not found in current directory");
                exit(-1);
	}
	c=(char)fgetc(ftrc);
	fclose(ftrc);

	switch (c){
		case '-':
			read_alog_trc(app);
			break;
		case 'c':
		case 's':
		case 'r':
			read_fsin_trc(app);
			break;
		case -1:
			printf("Reading empty file\n");
			break;
		default:
			printf("Cannot understand this trace format");
                        exit(-1);
			break;
	}
}

/**
 * Reads a trace from a file in fsin trc format
 * This format only takes into account 'c' CPU, 's' SEND, 'r' RECV, events.
 *
 *@param app            The data structure storing the application information
 */
void read_fsin_trc(application *app) {

    FILE * ftrc;
    char buffer[512];
    char * tok;
    char sep[]=" \t";
    long to, from, tag, length;

    if((ftrc = fopen(app->pattern_file, "r")) == NULL){
        printf("Trace file not found in current directory\n");
        exit(-1);
    }
    while(fgets(buffer, 512, ftrc) != NULL) {
        if(buffer[0] != '\n' && buffer[0] != '#') {
            if(buffer[strlen(buffer) - 1] == '\n')
                buffer[strlen(buffer) - 1] = '\0';

            tok = strtok( buffer, sep);

            if (strcmp(tok, "s") == 0) { // Communication.
                tok = strtok(NULL, sep); // from
                from = atol(tok); // Node to add event
                tok=strtok(NULL, sep);
                to = atol(tok); // event's PID: destiny when we are sending
                if (to != from) {
                    tok = strtok(NULL, sep);
                    tag = atol(tok); // Type of message (tag)
                    tok = strtok(NULL, sep);
                    length = atol(tok); // Length of message
                    if(length == 0)
                        length = 1;
                    if((app->size_tasks <= to) || (app->size_tasks <= from)){
                        printf("WARNING: Number of nodes in the trace larger than application.\n");
                        exit(-1);
                    }
                    if(to != from)
                        send(app, from, to, tag, length);
                }
            }
            else if(strcmp(tok, "r") == 0) { // Communication.
                tok = strtok(NULL, sep); // from
                from = atol(tok); // Node to add event
                tok=strtok(NULL, sep);
                to = atol(tok); // event's PID: destiny when we are sending
                if (to != from) {
                    tok = strtok(NULL, sep);
                    tag = atol(tok); // Type of message (tag)
                    tok = strtok(NULL, sep);
                    length = atol(tok); // Length of message
                    if(length == 0)
                        length = 1;
                    if((app->size_tasks <= to) || (app->size_tasks <= from)){
                        printf("WARNING: Number of nodes in the trace larger than application.\n");
                        exit(-1);
                    }
                    if(to != from)
                        receive(app, from, to, tag, length);
                }
            }
            else if (strcmp(tok, "c")==0){ // Computation.
                tok = strtok(NULL, sep);
                to = atol(tok); // nodeId.
                tok = strtok(NULL, sep);
                length=atol(tok); // Computation time.
                if(app->size_tasks <= to){
                        printf("WARNING: Number of nodes in the trace (%ld) larger than application (%ld).\n", to, app->size_tasks);
                        exit(-1);
                }
                if(length == 0)
                    length = 1;
                insert_computation(app, to, length);
            }
        }
    }
    fclose(ftrc);
}

/**
 * Reads a trace from a file in alog format
 * This format only takes in account "-101" SEND and "-102" RECV, events.
 *
 *@param app            The data structure storing the application information
 */
void read_alog_trc(application *app) {

    FILE * ftrc;
    char buffer[512];
    char * tok;
    char sep[]=" \t";
    long to, from, tag, length;

    if((ftrc = fopen(app->pattern_file, "r")) == NULL){
        printf("Trace file not found in current directory\n");
        exit(-1);
    }
    while(fgets(buffer, 512, ftrc) != NULL) {
        if(buffer[0] != '\n' && buffer[0] != '#') {
            if(buffer[strlen(buffer) - 1] == '\n')
                buffer[strlen(buffer) - 1] = '\0';

            tok = strtok( buffer, sep);

            if (strcmp(tok, "-101") == 0) { // Communication. SEND
                tok = strtok(NULL, sep); // from
                from = atol(tok); // Node to add event
                tok=strtok(NULL, sep);
                tok=strtok(NULL, sep);
                to = atol(tok); // event's PID: destiny when we are sending
                if (to != from) {
                    tok = strtok(NULL, sep);
                    tok = strtok(NULL, sep);
                    tok = strtok(NULL, sep);
                    tag = atol(tok); // Type of message (tag)
                    tok = strtok(NULL, sep);
                    length = atol(tok); // Length of message
                    if(length == 0)
                        length = 1;
                    if((app->size_tasks <= to) || (app->size_tasks <= from)){
                        printf("WARNING: Number of nodes in the trace larger than application.\n");
                        exit(-1);
                    }
                    if(to != from)
                        send(app, from, to, tag, length);
                }
            }
            else if(strcmp(tok, "-102") == 0) { // Communication. RECV
                tok = strtok(NULL, sep); // from
                to = atol(tok); // Node to add event
                tok=strtok(NULL, sep);
                tok=strtok(NULL, sep);
                from = atol(tok); // event's PID: destiny when we are sending
                if (to != from) {
                    tok = strtok(NULL, sep);
                    tok = strtok(NULL, sep);
                    tok = strtok(NULL, sep);
                    tag = atol(tok); // Type of message (tag)
                    tok = strtok(NULL, sep);
                    length = atol(tok); // Length of message
                    if(length == 0)
                        length = 1;
                    if((app->size_tasks <= to) || (app->size_tasks <= from)){
                        printf("WARNING: Number of nodes in the trace larger than application.\n");
                        exit(-1);
                    }
                    if(to != from)
                        receive(app, from, to, tag, length);
                }
            }
        }
    }
    fclose(ftrc);
}
