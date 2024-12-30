#ifndef DYNAMICPTR_H
#define DYNAMICPTR_H

typedef struct
{
	int type;
	void* data;
} dynamicPtr;
#define ERROR -1
#define UNKNOWN 0
#define CHAR 1
#define SHORT 2
#define INT 3
#define LONG 4
#define STRING 5

dynamicPtr dpint(int x)
{
	dynamicPtr ptr;
	ptr.type = INT;
	ptr.data = (void*)(&x);

	return ptr;
}

dynamicPtr dp(void* x)
{
	dynamicPtr ptr;
	ptr.type = UNKNOWN;
	ptr.data = x;

	return ptr;
}

dynamicPtr dperr(int code)
{
	dynamicPtr ptr;
	ptr.type = ERROR;
	ptr.data = (void*)(&code);

	return ptr;
}

#endif