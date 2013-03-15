/*
 *  (c) Copyright 2004, 2008, 2011, 2013 Christopher J. McKenzie
 */
#if !defined _PARSE_H_
#define _PARSE_H_

#include "common.h"
#define TRUE  1
#define FALSE 0

void unparse(char** in);
void parse(char* in, char** out, size_t maxTokens);
int endofline(char**out, int start);
int nextline(char**out, int start);
int combine(char**out, int start, char**_res);
#endif // _PASE_H_

