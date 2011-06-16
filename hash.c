/* Hash.c
 * 	Arbitrary depth, arbitrary size, arbitrary hash
 *
 *  (c) 2008, 2011, Christopher J. McKenzie under the terms of the
 *      GNU Public License, incorporated herein by reference.
 */
#include "common.h"
#include "hash.h"
#include "parse.h"

extern int errno;
typedef int	Severity;
typedef char	bool;

enum
{
	NONFATAL=0,
	FATAL=1,
};
enum
{
	false,
	true
};

Node *g_head = 0;
char *g_fname = "hash";
bool g_fnameflag = false;
struct stat g_dp;
const HashTable hash_prototype =
{
	0,
	64,	// Default to 64 entries
	63,	// The mask
	0	// Needs to be allocated
};

// Local functions
static inline void 	setError(char *file, int lineno, char *message, char*varname, Severity s);
static inline size_t	hashResize(Node*toResize);
static inline Node*	hashInsert(Node*toSearch, Node**toInsert, bool*created, bool force);
static inline size_t	hashFind(Node*toSearch, char*key);
static inline size_t	hash(char*tohash);
static inline Node*	nodeCreate(NodeType type, char*key, char*value);
static int 		dataLoadHead();
static int 		dataSaveHead();
static void 		printUsage(char*progname );
void 			cleanup(Node*toClean);

// A couple macros
#ifdef _DEBUG
#define mmalloc(bytes) 			malloc(bytes);printf("malloc: %d\t<%s@%d>\n", bytes, __FILE__, __LINE__);
#define mrealloc(pointer, bytes) 	realloc(pointer, bytes);printf("realloc: %d\t<%s@%d>\n", bytes, __FILE__, __LINE__);
#define BAILIFNOT(Node, Check) 	if(Node->type != Check) return 0;
#define NULLCHECK(var)		if(var == NULL) setError(__FILE__, __LINE__, "Null exception", " var ", FATAL);
#define ASSERT(n) 		if( ! ( n ) ) { printf("<ASSERT FAILURE@%s:%d>", __FILE__, __LINE__); fflush(0); __asm("int $0x3"); }
#define TRACE(n)		printf("trace: %s <%s@%d>\n", n, __FILE__, __LINE__);fflush(0);
#else //_DEBUG
#define mmalloc(bytes) 			malloc(bytes)
#define mrealloc(pointer, bytes)	realloc(pointer, bytes)
#define BAILIFNOT(Node, Check) 	{}
#define NULLCHECK(var)		{}
#define ASSERT(n)		{}
#define TRACE(n)		{}
#endif //_DEBUG
#define TABS	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
#define TABOUT	(depth) ? (strlen(TABS) + TABS - depth + 1) : ""
// RESIZE policy
#define RESIZEIF(n)		((n * 3) / 5)

static inline void setError(char *file, int lineno, char *message, char*varname, Severity s)
{
	printf("%s@%d: %s %c%s%c\n",
			file? file : "(unknown)",
			lineno? lineno : -1,
			message? message : "An error occured",
			varname? '(' : ' ',
			varname? varname : " ",
			varname? ')' : ' ');
	switch(s)
	{
		case FATAL:
			exit -1;
			break;
		case NONFATAL:
		default:
			break;
	}
}

void dumpNode(Node*nd, size_t depth, size_t maxdepth)
{
	switch(nd->type)
	{
		case HASH:
			{
				size_t ix;
				if(depth < maxdepth)
				{
					if(depth > 0)
					{
						printf("%s@\t%s\n", TABOUT, nd->key);
					}	
					for(ix = 0; ix < nd->hash->size_maximum; ix++)
					{
						if(nd->hash->table[ix])
						{
							dumpNode(nd->hash->table[ix], depth + 1, maxdepth);
						}
					}
				}
				else
				{
					printf("%s@\t%s\n", TABOUT, nd->key);
				}
			}
			break;

		case TUPLE:
			printf("%s%s\t%s\n", TABOUT, nd->key, nd->value? nd->value: "<NULL>");
			break;
	}
}

// Resize a hash according to a policy
// If the policy isn't met then this doesn't resize
static inline size_t	hashResize(Node*toResize)
{
	static size_t ix;
	static HashTable *hash;
	Node **tempHash;
	TRACE("hashResize");

	NULLCHECK(toResize);
	BAILIFNOT(toResize, HASH);

       	hash = toResize->hash;

	// Resize
	if( RESIZEIF(hash->size_maximum) < hash->size_current)
	{
		// Create a temporary hash
		tempHash = mmalloc(hash->size_maximum * sizeof(Node*));
		ASSERT(tempHash != NULL);
		// Copy the current hash over to it	(maximum because data can be sparse)
		memcpy(tempHash, hash->table, hash->size_maximum * sizeof(Node*));
		
		// Increase the size of the current hash
		hash->size_maximum <<= 1;
		hash->size_mask = hash->size_maximum - 1; 
		
		hash->table = (Node**)mrealloc(hash->table, hash->size_maximum * sizeof(Node*));
		ASSERT(hash->table != NULL);

		// Clear it
		memset(hash->table, 0, hash->size_maximum * sizeof(Node*));

		// Repopulate it
		for(ix = 0; ix < hash->size_current; ix++)
		{
			if(tempHash[ix])
			{
				hashInsert(toResize, &(tempHash[ix]), 0, false);
			}
		}
		
		// Free up the temporary hash
		free(tempHash);
	}
	
	// Return the (new) maximum size
	return hash->size_maximum;
}

/*
 * Insert something into a hash
 * Returns the place of insert
 * Resizes if necessary.
 */
static inline Node*	hashInsert(Node*toSearch, Node**toInsert, bool*created, bool force)
{
	static size_t insertionPlace;
	static HashTable *hash;

	NULLCHECK(toSearch);
	NULLCHECK(*toInsert);
	BAILIFNOT(toSearch, HASH);
	TRACE("hashInsert");

	// See if it needs a resize
	hashResize(toSearch);

	hash = toSearch->hash;
	insertionPlace = hashFind(toSearch, (*toInsert)->key);
	// See if anything is there already
	if ( (hash->table[insertionPlace] == NULL) || force == true)
	{
		if(hash->table[insertionPlace] != NULL)
		{
			cleanup(hash->table[insertionPlace]);
		}

		if(created)
		{
			*created = true;
		}
		hash->size_current++;
		hash->table[insertionPlace] = (Node*)mmalloc(sizeof(Node));
		memcpy(hash->table[insertionPlace], *toInsert, sizeof(Node));
	}
	else
	{
		if(created)
		{
			*created = false;
		}
		cleanup(*toInsert);
		(*toInsert) = hash->table[insertionPlace];
	}
	switch(hash->table[insertionPlace]->type)
	{
		case HASH:
			return hash->table[insertionPlace];
			break;
		default:
		case TUPLE:
			return toSearch;
			break;
	}
}

/*
 * Find an insertion point in a hash
 * for a given key value
 */
static inline size_t	hashFind(Node*toSearch, char*key)
{
	static size_t index;
	static Node **table;

	NULLCHECK(toSearch);
	NULLCHECK(key);
	BAILIFNOT(toSearch, HASH)
	TRACE("hashFind");

	index = (hash(key) & toSearch->hash->size_mask);
	table = toSearch->hash->table;

	for(;;)
	{
		// See if it matches
		if(!table[index])
		{
			return index;
		}

		if(strcmp(table[index]->key, key))
		{
			index++;
			index &= toSearch->hash->size_mask;
			continue;
		}
		return index;
		break;
	}
}

/*
 * Compute the starting point of a hash search
 */
static inline size_t	hash(char*tohash)
{
	static size_t hash_result = 0;
	static int letterno = 0;

	NULLCHECK(tohash);
	TRACE("Hash");

	// The hash algorithm starts with an size_t of 0
	hash_result = 0;

	// It takes each letter and xors it with the previous hash_result while incrementing a variable
	// And bitshifting the letter left letterno spaces
	for(letterno = 0; (tohash[letterno]) && (letterno < 24); letterno++)
	{
		hash_result = hash_result ^ (tohash[letterno] << letterno);
	}
	// Returns the hash_result;
	return hash_result;
}

/*
 * Create a node for insertion purposes
 */
static inline Node*nodeCreate(NodeType type, char*key, char*value)
{
	Node*newNode = (Node*)mmalloc(sizeof(Node));
	ASSERT(newNode != NULL);

	NULLCHECK(key);
	TRACE("nodeCreate");

	// Assign the type
	newNode->type = type;

	// Assign the key
	newNode->key = (char*)mmalloc(strlen(key) + 1);
	ASSERT(newNode->key);
	strcpy(newNode->key, key);

	// Create the type
	switch(type)
	{
		case HASH:
			newNode->hash = (HashTable*)mmalloc(sizeof(HashTable));
			ASSERT(newNode->hash != NULL);
			memcpy(newNode->hash, &hash_prototype, sizeof(HashTable));

			newNode->hash->table = (Node**)mmalloc(sizeof(Node*) * newNode->hash->size_maximum);
			ASSERT(newNode->hash->table != NULL);
			memset(newNode->hash->table, 0, sizeof(Node*) * newNode->hash->size_maximum);
			break;

		case TUPLE:
			NULLCHECK(value);
			newNode->value = (char*)mmalloc(strlen(value) + 1);
			ASSERT(newNode->value);
			strcpy(newNode->value, value);
			break;
	}

	return newNode;
}

#define BUFFORCE ret = write(fd, buffer, strlen(buffer));\
	ASSERT(ret == strlen(buffer));\
	\
	memset(buffer, 0, bufsize);\

#define BUFCHECK if(strlen(buffer) > ((bufsize * 3) / 4)) { BUFFORCE }
#define BUFMATH	buffer + strlen(buffer), bufsize - strlen(buffer) 
// The dataSave is recursive

#define bufsize 4096
static int dataSave(int fd, Node*node, int depth)
{
	char buffer[bufsize] = {0};
	size_t ix;
	size_t ret;
	TRACE("dataSave");

	for( ix = 0; ix < node->hash->size_maximum; ix++ )
	{
		if(node->hash->table[ix] != NULL)
		{
			switch(node->hash->table[ix]->type)
			{
				case TUPLE:
					snprintf( BUFMATH, "\n%s\"%s\"\t\"%s\"", TABOUT, node->hash->table[ix]->key, node->hash->table[ix]->value);
					BUFCHECK
					break;
				case HASH:
					snprintf( BUFMATH, "\n%s\"%s\"\n%s{", TABOUT, node->hash->table[ix]->key, TABOUT);
					// Force this stuff to be written to disk first!
					BUFFORCE
					// Recurse down
					dataSave(fd, node->hash->table[ix], depth + 1);
					snprintf( BUFMATH, "\n%s}", TABOUT);
					break;
			}
		}
	}
	// Force the buffer to be written before returning
	BUFFORCE
	return node->hash->size_current;
}

static int dataSaveHead()
{
	int fd = open(g_fname, O_TRUNC | O_RDWR);
	int ret;
	char buffer[] = "HEAD\n{";
	char end[] = "\n}\n";
	TRACE("dataSaveHead");
	ret = write(fd, buffer, strlen(buffer));
	ASSERT(ret == strlen(buffer));

	if(g_head->hash)
	{
		dataSave(fd, g_head, 1);
	}
	ret = write(fd, end, strlen(end));
	ASSERT(ret == strlen(end));

	// Close the file descriptor
	close(fd);
}

static char** dataLoadRecursive(char **start, Node*Current)
{
	Node*insertNode;

	TRACE("dataLoadRecursive");
	while(start[1] && strlen(start[1]))
	{
		// It's a hash
		if(start[1][0] == '{' )
		{
			insertNode = nodeCreate(HASH, start[0], 0);
			hashInsert(Current, &insertNode, 0, false);
			start = dataLoadRecursive(start + 2, insertNode);
		
		}
		else if(start[0][0] == '}')
		{
			start ++;
			// End of scope
			return start;
		}
		else
		{
			insertNode = nodeCreate(TUPLE, start[0], start[1]);
			hashInsert(Current, &insertNode, 0, false);
			start += 2;
		}
	}

	return start;
}


#define MAXTOKENS	65536
static int dataLoadHead() {
	int fd,
	    ret;

  size_t sz;

	char	*buffer,
		**parsed;

	TRACE("dataLoadHead");
	umask(0);

	if(g_fnameflag == false) {

		if(! chdir((const char*)getenv("HOME")) ) {
      doerror();
    }

		if(stat(".array", &g_dp)) {

			mkdir(".array",0755);
		}

		if(!chdir(".array")) {        //assuming this is successful now       
      doerror();
    }
	}

	if(stat(g_fname, &g_dp)) {

		creat(g_fname, 0644);

	} else {	
		// Create a buffer for the file
		buffer = (char*)mmalloc(g_dp.st_size);
		NULLCHECK(buffer);

		// Create a buffer for the tokens
		parsed = (char**)mmalloc(MAXTOKENS * sizeof(char*));
		NULLCHECK(parsed);

		// Open the file
		fd = open(g_fname, O_RDONLY);

		// Read the whole file in
		sz = read(fd, buffer, g_dp.st_size);
    if(sz != g_dp.st_size) {
      doerror();
    }

		close(fd);

		parse(buffer, parsed, MAXTOKENS);

		dataLoadRecursive(parsed + 2, g_head);
	}

	return 1;
}

static void printUsage(char *progname)
{
	TRACE("printUsage");
	printf("hash %s - General purpose hashing for the masses.\n", VERSION);
	printf("Usage: %s [ option | hash ]* (key) (value)\n", progname);
	printf("\n");
	printf("options:\n");
	printf("  -f --file dbase  User specified database\n");
	printf("  -a --all         Show the entire tree\n");
	printf("  -F --force       Overwrite any existing value\n");
	printf("     --delete      Delete a key or hash table\n");
	printf("  -h --help        This screen\n");
	printf("  -s --stdin       Read from stdin\n");
	printf("     --version     Version\n");
	printf("\n");
	printf("hash	one or more recursive hash names\n");
	printf("key	key\n");
	printf("value	value\n");
	
	return;
}

void cleanup(Node*toClean)
{
	size_t ix;
	TRACE("Cleanup");
	free(toClean->key);
	switch(toClean->type)
	{
		case TUPLE:
			free(toClean->value);
			break;
		case HASH:
			for(ix = 0; ix < toClean->hash->size_maximum; ix++)
			{
				if(toClean->hash->table[ix])
				{
					cleanup(toClean->hash->table[ix]);
				}
			}
			free(toClean->hash->table);
			free(toClean->hash);
			break;
	}
	free(toClean);
}

int main(int argc, char*argv[])
{
	bool 	created = false,
		cmdsearch = true,
	    	usestdin = false,
		firstentry = true,
	    	force = false,
	    	doDelete = false;

	int maxnode = 1;

	char *progname = argv[0];

	Node*insertNode = NULL;
	Node*currentNode = NULL;

	g_head = nodeCreate(HASH, "HEAD", 0);
	currentNode = g_head;

	if(argc <= 1)
	{
		dataLoadHead();
	}

	while(argc > 1)
	{
		if(cmdsearch == true && argv[1][0] == '-')
		{
			if (!strcmp(argv[1], "--help"))
			{
				printUsage(progname);
				goto end;
			}
			else if (!strcmp(argv[1], "--"))
			{
				cmdsearch = false;
			}
			else if (!strcmp(argv[1], "--version"))
			{
				printf("%s\n", VERSION);
				goto end;
			}
			else if (!strcmp(argv[1], "-f") || (!strcmp(argv[1], "--fname" )))
			{
				g_fname = argv[2];
				g_fnameflag = true;
				argc--;
				argv++;
			}

			else if (!strcmp(argv[1], "-s") || (!strcmp(argv[1], "--stdin" )))
			{
				usestdin = true;
			}
			else if (!strcmp(argv[1], "-a") || (!strcmp(argv[1], "--all" )))
			{
				maxnode = 65535;
			}
			else if (!strcmp(argv[1], "-F") || (!strcmp(argv[1], "--force" )))
			{
				force = true;
			}
			else if (!strcmp(argv[1], "--delete"))
			{
				doDelete = true;
			}
		}
		else
		{
			if(firstentry == true)
			{
				dataLoadHead();
				firstentry = false;
			}
			if (argc == 3)
			{
				insertNode = nodeCreate(TUPLE, argv[1], argv[2]);
				currentNode = hashInsert(currentNode, &insertNode, &created, force);
				if(insertNode->type == TUPLE)
				{
					// Go double time (key/value pair)
					argv++;
					argc--;
				}
			}
			else
			{
				insertNode = nodeCreate(HASH, argv[1], 0);
				if(argc < 3)
				{
					currentNode = hashInsert(currentNode, &insertNode, &created, force);
				}
				else
				{
					currentNode = hashInsert(currentNode, &insertNode, &created, false);
				}
			}
		}
		argv++;
		argc--;
	}
	if(insertNode == NULL)
	{
		insertNode = g_head;
	}
	if(doDelete == true)
	{
		cleanup(insertNode);
	}
	else
	{
		if(created == false)
		{
			dumpNode(insertNode, 0, maxnode);
		}
	}
	dataSaveHead();
end:
	cleanup(g_head);
	return(0);
}
