/* Array
 *	array management for shell scripts
 *
 * (c) Copyright 2005, 2008 Christopher J. McKenzie under the terms of the
 *  GNU Public License, incorporated herein by reference
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parse.h"

#define VERSION /*BEGIN_VERSIONATE*/"2.0.1307"/*END_VERSIONATE*/
#define MAXSIZE	128
#define MINSIZE	10
#define DBASE	"arrays"
#define VOID	"{void}"
#define PUTIN(n)\
{\
	write(arrays, n, strlen((const char*)n)); \
	write(arrays, "\n", 1); \
}
#define DUMP		0x04	//DUMP DATA
#define LOAD		0x05	//LOAD DATA
#define PRINT		0x07	//PRINT DATA	//DEBUG
#define ITOA_LEN	32

enum
{
	ERR_OOB = 1, 
	ERR_VOID, 
	ERR_BADRAY, 
	ERR_BADCMD, 
	ERR_BADFILE, 
	ERR_NODATA, 
	ERR_CREAT, 
	ERR_DELETE, 
	ERR_ASSIGN, 
	ERR_UNKNOWN, 

	ERR_LAST
};

char*errors[] = 
{
	"SUCCESS", 
	"OUT_OF_BOUNDS", 
	"VOID", 
	"BAD_ARRAY", 
	"BAD_CMD", 
	"BAD_FILE", 
	"NO_DATA", 
	"CREATED", 
	"DELETED", 
	"ASSIGNED", 
	"UNKNOWN", 
	0
};

struct ray*creatray(unsigned char*);
int assign(struct ray*, unsigned long, unsigned char*);

struct ray
{	
	unsigned char*name;	//array name
	unsigned long nmemb, size;	//members in the array
	unsigned char**values;
};

struct
{	
	unsigned long nmemb;	//members in the array	
	struct ray*array;
}table;

struct stat g_dp;
char	*fbuf, 
	**parsed, 
	g_explicit = 0;
char error = 0;

//Finds the closest power of 2 rounding up
int closepower(unsigned long n)
{
	int power = 0;

	if(n < 10)
	{
		return(10);
	}
	while(n > 0)
	{
		n >>= 1;
		power++;
	}

	return (1 << power);
}

unsigned char*itoa(unsigned int in)
{	
	static char 
		toreturn[ITOA_LEN], 
		*ptr;

	ptr = toreturn + ITOA_LEN - 1;

	memset(toreturn, 0, ITOA_LEN);

	do
	{
		ptr--;	
		*ptr = (in % 10) + '0';
		in /= 10;
	}while(in);

	return(ptr);
}

unsigned int m_atoi(unsigned char*in)
{	
	int toret = 0;

	while(*in >= '0' && *in <= '9')
	{
		toret *= 10;
		toret += *in - '0';
		in++;
	}
	return(toret);
}

//Assumes we are in the correct directory
int data(char command)
{
	int 	arrays, 
		ix, 
		array_ix, 
		table_ix, 
		arraysize, 
		tablesize;
	char*ptr;
	struct ray*cur;	

	stat(DBASE, &g_dp);
	fbuf = (char*)malloc(g_dp.st_size+1);	

	if(command == LOAD)
	{
		arrays = open(DBASE, 0);
		if(read(arrays, fbuf, g_dp.st_size) > 0)	
		{
			ix = 1;	//First entry
			parse(fbuf, parsed, 65535);
			tablesize = m_atoi(parsed[0]);
			table.array = (struct ray*)malloc(sizeof(struct ray)*closepower(tablesize));
			memset(table.array, 0, closepower(tablesize)*sizeof(struct ray));
		
			for(table_ix = 0;table_ix < tablesize;table_ix++)
			{
				if(!parsed[ix])
				{
					error = ERR_BADFILE;
					return(0);
				}
				cur = creatray(parsed[ix]);
				ix++;
				arraysize = m_atoi(parsed[ix]);	//size of array
				ix++;	//First dataset
				for(array_ix = 0;array_ix < arraysize;array_ix++)	//array_ixate through the data
				{
					if(parsed[ix])	
					{
						assign(cur, array_ix, parsed[ix++]);
					}
					else	//corrupt file
					{
						break;
					}
				}//Hopefully after this we will be at the next array name
			}
		}
		else
		{
			table.nmemb = 0;
			table.array = (struct ray*)malloc(sizeof(struct ray)*closepower(table.nmemb));
			memset(table.array, 0, closepower(table.nmemb)*sizeof(struct ray));
		}
//		printf("LOAD\n");
//		data(PRINT);
	}
	else if(command == DUMP)
	{//	printf("SAVE\n");
	//	data(PRINT);
		arrays = creat(DBASE, O_CREAT | O_WRONLY | O_TRUNC);
		ptr = itoa(table.nmemb);
		PUTIN(ptr);

		for(table_ix = 0; table_ix < table.nmemb; table_ix++)
		{
			cur = &table.array[table_ix];
			PUTIN(cur->name);
			ptr = itoa(cur->nmemb);
			PUTIN(ptr);

			for(ix = 0; ix < cur->nmemb;ix++)
			{
				PUTIN(cur->values[ix]);
			}
		}
	}
	else if(command == PRINT)
	{
		printf("\t%d\n", table.nmemb);	

		for(table_ix = 0; table_ix < table.nmemb; table_ix++)
		{
			cur = &table.array[table_ix];
			printf("\t%s ", cur->name);
			printf("%d %d\n", cur->nmemb, cur->size);

			for(ix = 0; ix < cur->nmemb; ix++)
			{
				printf("\t\t%s\n", cur->values[ix]);
			}
		}
	}
	else
	{
		return 0;	//wha happen'd?
	}
	return 1;
}

struct ray*findray(unsigned char*name)
{	
	int 	n, 
		m = strlen((const char*)name);

	for(n = 0; n < table.nmemb; n++)
	{	
		if(!strncmp((char*)name, (const char*)table.array[n].name, m))	//Yay, found it
		{
			if(m == strlen((const char*)table.array[n].name))
			{
				return(&table.array[n]);
			}
		}
	}
	return 0;	//boo hoo
}

struct ray*creatray(unsigned char*name)
{
	struct ray*toret;	

	if((table.nmemb + 1) > closepower(table.nmemb))//realloc is needed
	{ 	
		table.array = realloc(table.array, closepower(table.nmemb)*sizeof(struct ray));
	}
	toret = &table.array[table.nmemb];
	toret->name = malloc(strlen((const char*)name)+1);
	strcpy((char*)toret->name, (const char*)name);
	toret->nmemb = 0;
	toret->size = closepower(toret->nmemb);
	toret->values = malloc(toret->size*sizeof(unsigned char**));
	memset(toret->values, 0, toret->size*sizeof(unsigned char**));
	table.nmemb++;
	return(toret);
}

// Assign value to index of toassign
int assign(struct ray*toassign, unsigned long index, unsigned char*value)
{ 	
	if(index >= toassign->size)//out of bounds
	{
		int old = toassign->size;	

		toassign->size = closepower(index);	
		toassign->values = (unsigned char**)realloc(toassign->values, (toassign->size)*sizeof(unsigned char**));
		//memset(toassign->values+old*sizeof(unsigned char*), 0, ((toassign->size-1)-old)*sizeof(unsigned char*));
	} 
	if((index + 1) > toassign->nmemb)
	{	
		int ix = toassign->nmemb;

		toassign->nmemb = index+1;

		for(ix; ix < index; ix++)
		{	
			assign(toassign, (unsigned long)ix, (unsigned char*)VOID);
		}
	}
//	if(toassign->values[index])
//	{
//		free(toassign->values[index]);
//	}
	toassign->values[index] = malloc(strlen((const char*)value)+2);
	memset(toassign->values[index], 0, strlen((const char*)value)+1);
	strcpy((char*)toassign->values[index], (char*)value);
	return 1;
}

unsigned char*getvalue(struct ray*toget, unsigned long index)
{ 	
	if(!toget)
	{
		error = ERR_BADRAY;
		return(0);	
	}
	if(index >= toget->nmemb)	//Oops
	{
		error = ERR_OOB;
		return(0);
	}
	if(toget->values[index] == 0)	//Oops again
	{
		error = ERR_VOID;
		return(0);	
	}
	return(toget->values[index]);
}

void printarray(unsigned char*cin)
{
	int ix;
	struct ray*toprint = 0;

	if(cin)
	{
		toprint = findray(cin);
	}
	else
	{
		if(table.nmemb == 0)
		{
			error = ERR_NODATA;	
		}
		else
		{
			for(ix = 0; ix < table.nmemb; ix++)
			{
				printf("%d\t%s\n", 
					table.array[ix].nmemb, 
					table.array[ix].name);
			}
		}
		return;
	}
	if(toprint)
	{
		if(toprint->nmemb == 0)
		{
			error = ERR_NODATA;
		}
		else	
		{	
			for(ix = 0;ix < toprint->nmemb;ix++)
			{
				printf("%d\t%s\n", ix, getvalue(toprint, ix));
			}
		}
	}
	else
	{
		if(!g_explicit)	
		{
			creatray(cin);
			error = ERR_CREAT;
		}
		else
		{
			error = ERR_BADRAY;
		}
	}
}

int setup()
{	
	chdir((const char*)getenv("HOME"));
	umask(0);

	if(stat(".array", &g_dp))
	{
		mkdir(".array", 0755);
	}
	chdir(".array");	//assuming this is successful now	
	if(stat(DBASE, &g_dp))
	{
		creat(DBASE, 0644);
	}
	return 1;
}

void deletearray(unsigned char*todel)
{	
	int 	ix, 
		len = strlen((const char*)todel);

	for(ix = 0;ix < table.nmemb;ix++)
	{
		if(!strncmp((const char*)table.array[ix].name, (const char*)todel, len))	//Found it
		{
			table.nmemb--;	
			for(;ix < table.nmemb; ix++)
			{
				table.array[ix] = table.array[ix+1];
			}
			error = ERR_DELETE;
			return;
		}
	}
	error = ERR_BADRAY;
}

int main(int argc, unsigned char*argv[])
{
	unsigned char 	*param = 0, 
			*array = 0, 
			*function = 0, 
			*value = 0, 
			*ret, 
			se = 0;
	int ix;
	struct ray*tomod;

	parsed = (char**)malloc(sizeof(char*) * 65536);	//64K entries
	memset(parsed, 0, 65536 << 2);

	if(!setup())
	{
	}
	while(argc>1)
	{
		param = argv[1];	
		if(!strcmp((const char*)param, "--flush"))
		{
			creat(DBASE, 0644);
			return(0);
		}
		else if(!strcmp((const char*)param, "--version"))
		{
			printf("%s\n", VERSION);
			return(0);
		}
		else if(!strcmp((const char*)param, "--help"))
		{
			printf("Array %s - General purpose arrays for the masses.\n", VERSION);
			printf("Usage: %s name (option) [ index (value) | function ]\n", argv[0]);
			printf("\n");
			printf("options:\n");
			printf("  --errors   Display error codes\n");
			printf("  --flush    Delete the database\n");
			printf("  --help     This help screen\n");
			printf("  --version  Print the version\n");
			printf("  --explicit Do nothing implicitly\n");
			printf("  --stderr   Print the return code to stderr\n");
			printf("\n");
			printf("functions:\n");
			printf("  delete     delete the array\n");
			printf("  create     create the array (implicit)\n");
			printf("\n");
			printf("name     Array name to index into\n");
			printf("index    Decimal numbered index for the array\n");
			return(0);
		}
		else if(!strcmp((const char*)param, "--errors"))
		{
			for(ix = 0;ix < ERR_LAST;ix++)
			{
				printf("%d\t%s\n", ix, errors[ix]);
			}
			return(0);
		}
		else if(!strcmp((const char*)param, "--explicit"))
		{
			g_explicit = 1;
		}
		else if(!strcmp((const char*)param, "--stderr"))
		{
			se = 1;
		}
		else if(!strcmp((const char*)param, "--"))
		{
			argv++;
			argc--;
			break;
		}
		else
		{
			param = 0;
			break;
		}
		argv++;
		argc--;
	}
	if(data(LOAD))
	{	
		if(argc < 2)
		{
			printarray(0);
		}
		else
		{
			array = argv[1];
			if(argc>2)
			{	
				function = argv[2];
				if(argc > 3)
				{
					value = argv[3];
				}
				ix = m_atoi(function);
				if(ix == 0 && *function != '0')	//command actually
				{
					if(!strcmp((const char*)function, "delete"))
					{
						deletearray(array);
					}
					else if(!strcmp((const char*)function, "create"))
					{
						creatray(array);
						error = ERR_CREAT;
					}	
					else
					{
						error = ERR_BADCMD;
					}
				}
				else	//really was an index
				{
					tomod = findray(array);
					if(argc > 3)	
					{	
						if(tomod == 0)
						{
							if(!g_explicit)	
							{
								tomod = creatray(array);
								error = ERR_CREAT;
							}
							else
							{
								error = ERR_BADRAY;
							}
						}
						if(tomod)
						{	
							assign(tomod, ix, value);
							error = ERR_ASSIGN;
						}
					}
					else
					{	
						ret = getvalue(tomod, ix);
						if(ret)
						{
							printf("%s\n", ret);
						}
					}
				}
			}
			else
			{
				printarray(array);
			}
			data(DUMP);
		}
	}

	if( (error > 0) && (error < ERR_LAST) && se )
	{
#if (!defined(sun)) && (!defined(__CYGWIN__))
		FILE * stderr = fopen("/dev/stderr", "w");	
		fprintf(stderr, "%s\n", errors[error]);
#endif
	}

	return(error);
}
