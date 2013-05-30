#ifndef UTIL_H
#define UTIL_H

typedef struct {
	int len;
	char ** items;
} list;

typedef struct {
	char * method;
	char * url;
} http_req_t;

char * readFile(char * filename);
char * readFileS(char * filename, int * size_out);
int startsWith(char * haystack, char * needle);
list ls(const char * dir);
void freelist(list l);
int endsWith(char * haystack, char * needle);
char * withoutPath(char * filename);
int equal(char * l, char * r);

#endif