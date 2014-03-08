// Uncanny query

#ifndef _UNCANNY_QUERY_HEADER
#define _UNCANNY_QUERY_HEADER

#define UNCANNY_QUERY_INTERNAL_VERSION_NUMBER 1

#include "uncanny.h"

namespace UncannyQuery {

enum {
	TREE_DISTINGUISHER,
	AUGMENTATION_DISTINGUISHER,
};

Uncanny::Tree* unpack_tree(size_t length, const char* desc);
void unpack_augmentation(Uncanny::Tree* tree, size_t length, const char* desc);

}

#endif

