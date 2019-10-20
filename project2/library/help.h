

#include <stdlib.h>		// defines  size_t

//get the pathname individuly
void getnamefrompath(const char *pathname, int* n, char*** name);

//free the memeroy
void freenames(char **entrynames, uint32_t nentries);
