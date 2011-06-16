/*
 *  (c) Copyright 2004, 2008, 2011 Christopher J. McKenzie under the terms of the
 *      GNU Public License, incorporated herein by reference
 */
#include "parse.h"

char *g_chrs[]= {
	"\000", "\001", "\002", "\003", "\004", "\005", "\006", "\007",
	"\010", "\011", "\012", "\013", "\014", "\015", "\016", "\017",
	"\020", "\021", "\022", "\023", "\024", "\025", "\026", "\027",
	"\030", "\031", "\032", "\033", "\034", "\035", "\036", "\037",
	"\040", "\041", "\042", "\043", "\044", "\045", "\046", "\047",
	"\050", "\051", "\052", "\053", "\054", "\055", "\056", "\057",
	"\060", "\061", "\062", "\063", "\064", "\065", "\066", "\067",
	"\070", "\071", "\072", "\073", "\074", "\075", "\076", "\077",
	"\100", "\101", "\102", "\103", "\104", "\105", "\106", "\107",
	"\110", "\111", "\112", "\113", "\114", "\115", "\116", "\117",
	"\120", "\121", "\122", "\123", "\124", "\125", "\126", "\127",
	"\130", "\131", "\132", "\133", "\134", "\135", "\136", "\137",
	"\140", "\141", "\142", "\143", "\144", "\145", "\146", "\147",
	"\150", "\151", "\152", "\153", "\154", "\155", "\156", "\157",
	"\160", "\161", "\162", "\163", "\164", "\165", "\166", "\167",
	"\170", "\171", "\172", "\173", "\174", "\175", "\176", "\177"
};

void unparse(char** in) {	
	static int n;
	n = 1;

	loop1:
		if(!in[n]) {
			return;

		} else if(*in[n]<32) {
			in[n-1][strlen((const char*)(in[n-1]))]=*in[n];
		}
		else if(in[n][-1]);
		else {
			in[n][-1]=' ';
		}

		n++;
		goto loop1;
}

void parse(char* in, char** out, size_t maxTokens) {

	char quote = 0;
	static char	*end = 0;
	end = in + strlen(in);
	int tokenCurrent = 0;
	char *ptr = in;

	if(end == in) {

		out[tokenCurrent] = NULL;
		return;
	}	

	for(;*ptr <= ' ';ptr++);

	out[tokenCurrent] = in;

	while(ptr != end) {	

		if(ptr[0] > ' ' || quote == 1)	//not a control sequence
		{	

			if(ptr[0] == '\"') {

				// If not in quotes this is the beginning
				if(quote == 0) {
					ptr[0] = 0;
					quote = 1;
					out[tokenCurrent] = ptr + 1;
					ptr++;
					continue;

				} else {
					// If the last character wasn't an escape sequence
					if (ptr[-1] != '\\') {
						quote = 0;
					}

					// It's escaped ... this is ok if it's an escaped escape
					else if(ptr [-2] == '\\') {	
						quote = 0;
					} else {

						ptr++;
						continue;
					}
				}
			} else {
				ptr++;
				continue;	
			}
		}

		
		ptr[0] = 0;

		for(;*ptr <= ' ' && ptr != end;ptr++);

		tokenCurrent++;

		if(tokenCurrent >= maxTokens) {
			return;
		}

		out[tokenCurrent] = ptr;
	}
	return;
}	

int endofline(char**out, int start) {
	loop:
		if(!out[start]) {
			return(start);
		}

		if(out[start] == (char*)(g_chrs[10])) {
			return(start);
		}

		start++;
		goto loop;
}

int nextline(char**out, int start) { 
	loop:
		if(!out[start]) {
			return(0);
		}

		if(out[start] == (char*)g_chrs[10]) {

			if(out[start + 1]) {
				return(start + 1);
			}
		}

		start++;
		goto loop;
}

int combine(char**out, int start, char**_res) {
	int end = endofline(out, start);
	char*res = (*_res);

	if(start == end) {
		return(FALSE);
	}

	res = (char*)malloc(256);	
	bzero(res, 256);
	sprintf((char*)res, "%s", out[start]);
	start++;

	while(start != end) {	
		sprintf((char*)res,"%s %s",res,out[start]);
		start++;
	}

	(*_res) = res;
	return(TRUE);
}

int doerror() {
  perror(strerror(errno));
  return 0;
}
