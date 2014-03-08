// Uncanny query.

#include <string.h>
#include <assert.h>
#include <stdint.h>

#include <iostream>
using namespace std;

#include "uncanny_query.h"
using namespace Uncanny;
using namespace UncannyQuery;

#define HEADER_LENGTH 16

static function<int(void*,void*)>* build_cmp_function(size_t& length, const char*& desc) {
	// Figure out how large the payload is.
	uint32_t chunk_size = *(uint32_t*)desc;
	// Copy out just the payload.
	const char* data = new const char[chunk_size];
	memcpy(data, desc+4, chunk_size);
	desc += 4 + chunk_size;
	length -= 4 + chunk_size;
	return new function<int(void*,void*)>([=data](void* a, void* b) -> void {
		return (long long)a > (long long)b;
	});
}

// Guarantees the input is for this version of UncannyQuery.
static bool verify_string(size_t length, const char* s, uint32_t distinguisher) {
	if (length < HEADER_LENGTH) return false;
	if (memcmp(s, "\1Uncanny", 8) != 0) return false;
	if (*((uint32_t*)(s+8)) != UNCANNY_QUERY_INTERNAL_VERSION_NUMBER) return false;
	if (*((uint32_t*)(s+12)) != distinguisher) return false;
	return true;
}

// Takes in a compiled description of a tree, and outputs it.
Tree* UncannyQuery::unpack_tree(size_t length, const char* desc) {
	assert(verify_string(length, desc, TREE_DISTINGUISHER));
	desc += HEADER_LENGTH;
	Tree* tree = new Tree();
	tree->cmp_f = build_cmp_function(length, desc);
	tree->key_deallocator = build_mem_function(length, desc);
	tree->value_deallocator = build_mem_function(length, desc);
	return tree;
}

// Takes in a compiled description of an augmentation and a tree,
// and returns the described augmentation.
void UncannyQuery::unpack_augmentation(Tree* tree, size_t length, const char* desc) {
	assert(verify_string(desc_length, desc, AUGMENTATION_DISTINGUISHER));
	desc += HEADER_LENGTH;
	Augmentation aug;
	aug.base_case = build_base_case_function(length, desc);
	aug.compute = build_compute_function(length, desc);
	aug.compare = build_compare_function(length, desc);
	tree->aug_ctx.new_augmentation(&aug);
	return NULL;
}

int main() {
	auto x = build_cmp_function(4, "\0\0\0\0");
	cout << x((void*)5, (void*)4) << endl;
}

