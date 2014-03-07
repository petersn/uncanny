// Uncanny trees.

#ifndef _UNCANNY_TREE_HEADER
#define _UNCANNY_TREE_HEADER

#include <map>
#include <vector>

namespace Uncanny {

struct Tree;

struct AugmentationResult;

struct Augmentation {
	int aug_id;
	int schema_index;

	// Should read in a key value pair, and write into output.
	void (*base_case)(void* key, void* value, AugmentationResult* output);

	// Should read in a and b, and write into output.
	void (*compute)(const AugmentationResult* a, const AugmentationResult* b, AugmentationResult* output);

	// Should return if a and b are the same.
	bool (*compare)(const AugmentationResult* a, const AugmentationResult* b);

	Augmentation();

	Augmentation(void (*_base_case)(void* key, void* value, AugmentationResult* output),
		void (*_compute)(const AugmentationResult* a, const AugmentationResult* b, AugmentationResult* output),
		bool (*_compare)(const AugmentationResult* a, const AugmentationResult* b));
};

// Augmentations all have an id, and a schema index.
// The id is completely unique, and used to refer to the augmentation.
// The schema index is the index into the AugmentationResult vector
// for where the data should be stored for the given augmentation.
// Every time we add or remove an augmentation we update the schema,
// repacking the remaining augmentations into the new schema.
// This operation increments the schema_version, which is used
// to compare against AugmentationData entries.
struct AugmentationCtx {
	int next_aug_id;
	int schema_version;
	std::map<int, Augmentation> augs;

	AugmentationCtx();

	int new_augmentation(Augmentation* aug);

	bool delete_augmentation(int aug_id);

	void recompute_schema();
};

// Stores the results of the augmentation
// at a single node in the tree.
struct AugmentationResult {
	bool clean;
	int aug_id;
	size_t data_length;
	union {
		int i;
		unsigned int ui;
		long long l;
		unsigned long long ul;
		float f;
		double d;
		void* vp;
	} data;
};

// Stores a vector of results at one node in the tree.
struct AugmentationData {
	int schema_version;
	std::vector<AugmentationResult> results;

	void fill_with_dirty_to_size(size_t length);
	void update_schema(AugmentationCtx* aug_ctx);
};

struct Node {
	Tree* ctx;
	Node* parent;
	Node* left;
	Node* right;
	void* key;
	void* value;
	int height;
	AugmentationData* aug_data;

	Node(Tree* _ctx, Node* _parent, void* _key, void* _value);
	~Node();
	bool recompute();
	int balance_factor();
	void change_child(Node* from, Node* to);
	void rotate_left();
	void rotate_right();
	void pprint(int depth);
};

struct Tree {
	AugmentationCtx aug_ctx;
	int (*cmp_f)(void* a, void* b);
	Node* root;

	Tree();
	~Tree();
	void free_subtree(Node* node);
	void pprint();
	void** get(void* key);
	void* get_default(void* key, void* otherwise);
	void insert(void* key, void* value);
	bool rebalance_node(Node* node);
	void remove(void* key);
	size_t compute_augmentation(Augmentation* aug, Node* node, AugmentationResult* output);
	size_t compute_augmentation_cut(Augmentation* aug, Node* node, void* key, int comparison_type, bool good_to_go, AugmentationResult* output);
	size_t augment_cut(int aug_id, void* key, int comparison_type, AugmentationResult* output);

#define _UNCANNY_TREE_AUG_CONV(name) \
	size_t name(int aug_id, void* key, AugmentationResult* output);

_UNCANNY_TREE_AUG_CONV(augment_lt)
_UNCANNY_TREE_AUG_CONV(augment_lte)
_UNCANNY_TREE_AUG_CONV(augment_gte)
_UNCANNY_TREE_AUG_CONV(augment_gt)

#undef _UNCANNY_TREE_AUG_CONV

};

}

#endif

