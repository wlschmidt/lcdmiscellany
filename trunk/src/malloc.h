#ifndef MY_MALLOC_H
#define MY_MALLOC_H
#include <malloc.h>
/*
#ifdef _DEBUG
#include <string.h>
#endif
//*/

inline int _srealloc(void **v, size_t size) {
	void *temp = realloc(*v, size);
	if (!temp && size) {
		//*v = 0;
		return 0;
	}
	/*
#ifdef _DEBUG
	if (size) {
		void *temp2 = malloc(size);
		memcpy(temp2, temp, size);
		free(temp);
		v = (T*) temp2;
		return 1;
	}
#endif
	//*/
	*v = temp;
	return 1;
}

#define srealloc(x,y) _srealloc((void**)&(x),(y))


#endif