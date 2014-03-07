// Uncanny trees.

#include <assert.h>
#include <stdlib.h>

using namespace std;
#include <iostream>

#include "uncanny.h"
using namespace Uncanny;

Augmentation::Augmentation() {
	aug_id = -1;
	schema_index = -1;
}

Augmentation::Augmentation(void (*_base_case)(void*, void*, AugmentationResult*),
	void (*_compute)(const AugmentationResult*, const AugmentationResult*, AugmentationResult*),
	bool (*_compare)(const AugmentationResult*, const AugmentationResult*)) {
	base_case = _base_case;
	compute = _compute;
	compare = _compare;
}

// Augmentations all have an id, and a schema index.
// The id is completely unique, and used to refer to the augmentation.
// The schema index is the index into the AugmentationResult vector
// for where the data should be stored for the given augmentation.
// Every time we add or remove an augmentation we update the schema,
// repacking the remaining augmentations into the new schema.
// This operation increments the schema_version, which is used
// to compare against AugmentationData entries.
AugmentationCtx::AugmentationCtx() {
	next_aug_id = 1;
	schema_version = 0;
}

int AugmentationCtx::new_augmentation(Augmentation* aug) {
	augs[next_aug_id] = *aug;
	augs[next_aug_id].aug_id = next_aug_id;
	recompute_schema();
	return next_aug_id++;
}

bool AugmentationCtx::delete_augmentation(int aug_id) {
	// Can't delete what we don't have.
	if (not augs.count(aug_id)) return false;
	augs.erase(aug_id);
	recompute_schema();
}

// Recomputes which augmentation goes in which AugmentationData slot.
void AugmentationCtx::recompute_schema() {
	// Renumber all of the augmentations.
	int schema_index = 0;
	for (map<int, Augmentation>::iterator iter = augs.begin(); iter != augs.end(); iter++)
		iter->second.schema_index = schema_index++;
	schema_version++;
}

void AugmentationData::fill_with_dirty_to_size(size_t length) {
	AugmentationResult fill_with;
	fill_with.clean = false;
	results.resize(length, fill_with);
}

void AugmentationData::update_schema(AugmentationCtx* aug_ctx) {
	// Produce a new blank slate of dirty results.
	AugmentationResult fill_with;
	fill_with.clean = false;
	vector<AugmentationResult> new_results;
	new_results.resize(aug_ctx->augs.size());
	// Copy over each old result, if it is clean, and still in the schema somewhere.
	for (int i=0; i<results.size(); i++) {
		AugmentationResult& r = results[i];
		if (r.clean and aug_ctx->augs.count(r.aug_id))
			new_results[aug_ctx->augs[r.aug_id].schema_index] = r;
	}
	// Update our schema version number and copy over the new results.
	schema_version = aug_ctx->schema_version;
	results = new_results;
}

Node::Node(Tree* _ctx, Node* _parent, void* _key, void* _value) {
	ctx = _ctx;
	parent = _parent;
	left = right = NULL;
	key = _key;
	value = _value;
	height = 0; // Initially wrong!
	aug_data = NULL;
}

Node::~Node() {
	delete aug_data;
}

bool Node::recompute() {
	// Invalidate our cache.
	delete aug_data;
	aug_data = NULL;
	int new_height = 0;
	if (left != NULL)
		new_height = left->height;
	if (right != NULL and right->height > new_height)
		new_height = right->height;
	new_height++;
	bool differs = new_height != height;
	height = new_height;
	return differs;
}

int Node::balance_factor() {
	int bf = 0;
	if (left != NULL)
		bf += left->height;
	if (right != NULL)
		bf -= right->height;
	return bf;
}

void Node::change_child(Node* from, Node* to) {
	if (left == from) left = to;
	if (right == from) right = to;
}

void Node::rotate_left() {
	if (parent != NULL)
		parent->change_child(this, right);
	right->parent = parent;
	parent = right;
	right = right->left;
	if (right != NULL)
		right->parent = this;
	parent->left = this;
}

void Node::rotate_right() {
	if (parent != NULL)
		parent->change_child(this, left);
	left->parent = parent;
	parent = left;
	left = left->right;
	if (left != NULL)
		left->parent = this;
	parent->right = this;
}

void Node::pprint(int depth) {
	if (right != NULL)
		right->pprint(depth+1);
	else if (left != NULL) {
		for (int i=0; i<depth; i++) cout << "  ";
		cout << "   ---" << endl;
	}
	for (int i=0; i<depth; i++) cout << "  ";
	cout << (long long)key << ": " << (long long)value << endl;
	if (left != NULL)
		left->pprint(depth+1);
	else if (right != NULL) {
		for (int i=0; i<depth; i++) cout << "  ";
		cout << "   ---" << endl;
	}
}

Tree::Tree() {
	root = NULL;
	cmp_f = NULL;
}

Tree::~Tree() {
	if (root == NULL) return;
	free_subtree(root);
	delete root;
}

void Tree::free_subtree(Node* node) {
	if (node->left != NULL) {
		free_subtree(node->left);
		delete node->left;
	}
	if (node->right != NULL) {
		free_subtree(node->right);
		delete node->right;
	}
}

void Tree::pprint() {
	if (root == NULL) cout << "---" << endl;
	else root->pprint(0);
}

void** Tree::get(void* key) {
	Node* here = root;
	int last_result;
	while (here != NULL and (last_result = cmp_f(key, here->key)) != 0) {
		if (last_result > 0) here = here->right;
		else here = here->left;
	}
	// Key not found. :(
	if (here == NULL) return NULL;
	return &here->value;
}

void* Tree::get_default(void* key, void* otherwise) {
	void** ptr = get(key);
	if (ptr == NULL) return otherwise;
	return *ptr;
}

void Tree::insert(void* key, void* value) {
	Node *here = root, *prev_here = NULL, *leaf;
	int last_result;
	while (here != NULL and (last_result = cmp_f(key, here->key)) != 0) {
		prev_here = here;
		if (last_result > 0) here = here->right;
		else here = here->left;
	}
	if (here == NULL) here = prev_here;
	// Easy case, simply update a value.
	if (here != NULL and here->key == key) {
		here->value = value;
		// Make sure to propagate cache invalidity.
		while (here != NULL) {
			here->recompute();
			here = here->parent;
		}
		return;
	}
	// Hard case, have to make a new leaf node.
	leaf = new Node(this, here, key, value);
	// Edge case for first insert.
	if (here == NULL) {
		root = leaf;
		return;
	}
	if (key > here->key) here->right = leaf;
	else here->left = leaf;
	Node* ptr = leaf;
	// Rebalance the tree.
	while (leaf != NULL) {
		if (not rebalance_node(leaf))
			break;
		leaf = leaf->parent;
	}
	// Continue propagating height information.
	if (leaf != NULL) {
		leaf = leaf->parent;
		while (leaf != NULL) {
//			// Early-out on this too!
//			if (not leaf->recompute_height()) break;
			// We cannot early-out.
			leaf->recompute();
			leaf = leaf->parent;
		}
	}
}

bool Tree::rebalance_node(Node* node) {
	bool differs = node->recompute();
	int bf = node->balance_factor();
	assert(bf >= -2 and bf <= 2);
	if (bf == 2) {
		if (node->left->balance_factor() == -1)
			node->left->rotate_left();
		node->rotate_right();
		node->recompute();
		differs = true;
		// Check if we re-rooted.
		if (node == root) root = node->parent;
	} else if (bf == -2) {
		if (node->right->balance_factor() == 1)
			node->right->rotate_right();
		node->rotate_left();
		node->recompute();
		differs = true;
		// Check if we re-rooted.
		if (node == root) root = node->parent;
	}
	return differs;
}

void Tree::remove(void* key) {
	Node* here = root;
	int last_result;
	while (here != NULL and (last_result = cmp_f(key, here->key)) != 0) {
		if (last_result > 0) here = here->right;
		else here = here->left;
	}
	// If there does not exist such an item, we're done!
	if (here == NULL) return;
	Node* to_delete = NULL;
	int child_count = (here->left != NULL) + (here->right != NULL);
	// Easy case, if we zero or one children, delete here.
	if (child_count < 2)
		to_delete = here;
	else {
		// Otherwise, delete the predecessor.
		to_delete = here->left;
		while (to_delete->right != NULL)
			to_delete = to_delete->right;
		// Copy over the data.
		here->key = to_delete->key;
		here->value = to_delete->value;
		// We must invalidate the augmentation data here.
		// It is no longer correct, as we now represent a new tree.
		delete here->aug_data;
		here->aug_data = NULL;
	}
	// At this point, to_delete should only have at most one child.
	assert((to_delete->left != NULL) + (to_delete->right != NULL) < 2);
	Node* child = to_delete->left;
	if (child == NULL) child = to_delete->right;
	// Reroot if necessary.
	if (to_delete == root) root = child;
	else {
		assert(to_delete->parent != NULL);
		// Otherwise, fix up the trees.
		Node* fix = to_delete->parent;
		fix->change_child(to_delete, child);
		if (child != NULL)
			child->parent = fix;
		// Recompute the heights.
		while (fix != NULL) {
			//if (not fix->recompute_height()) break;
			// Unfortunately, we can no longer early-out.
			fix->recompute();
			fix = fix->parent;
		}
	}
	delete to_delete;
}

#define AUG_COMPUTE_BOTH_SUBTREES(left_comp, right_comp) \
	size_t elements = 1, new_elements; \
	AugmentationResult temp; \
	AugmentationResult double_buf[2]; \
	int i = 0; \
	aug->base_case(node->key, node->value, &double_buf[i]); \
	if (node->left != NULL) { \
		elements += new_elements = left_comp; \
                if (new_elements != 0) { \
			aug->compute(&temp, &double_buf[i], &double_buf[1-i]); \
			i = 1-i; /* Flip the buffer. */ \
		} \
	} \
	if (node->right != NULL) { \
		elements += new_elements = right_comp; \
		if (new_elements != 0) { \
			aug->compute(&double_buf[i], &temp, &double_buf[1-i]); \
			i = 1-i; \
		} \
	}

size_t Tree::compute_augmentation(Augmentation* aug, Node* node, AugmentationResult* output) {
	if (node->aug_data == NULL) {
		// Start a cache for the node.
		AugmentationData* aug_data = node->aug_data = new AugmentationData();
		aug_data->schema_version = aug_ctx.schema_version;
		aug_data->fill_with_dirty_to_size(aug_ctx.augs.size());
	}
	AugmentationData* aug_data = node->aug_data;
	// Check if the schema is up-to-date, and if not, update it.
	if (aug_data->schema_version != aug_ctx.schema_version)
		aug_data->update_schema(&aug_ctx);
	// Finally, now that we have an up-to-date, valid cache, check the value.
	AugmentationResult* cached_result = &aug_data->results[aug->schema_index];
	if (not cached_result->clean) {
		// Crap, it's a dirty value. Better update it.
		AUG_COMPUTE_BOTH_SUBTREES(compute_augmentation(aug, node->left, &temp), \
			compute_augmentation(aug, node->right, &temp))
		*cached_result = double_buf[i];
		cached_result->clean = true;
	}
	*output = *cached_result;
	return cached_result->data_length;
}

size_t Tree::compute_augmentation_cut(Augmentation* aug, Node* node, void* key, int comparison_type, bool good_to_go, AugmentationResult* output) {
	// Firstly, figure out of we care about this node's value at all.
	int comparison = cmp_f(node->key, key);
	// Now we do a little re-mapping.
	// Valid comparison_types are:
	//   -2 less than
	//   -1 less than or equal
	//    1 greater than or equal
	//    2 greater than.
	// First thing first, we multiply by comparison_type, to make
	// sure that less than searches have the opposite sense.
	comparison *= comparison_type;
	// Next, we remap equals if this is a strong comparison.
	if (comparison == 0 and (comparison_type == 2 or comparison_type == -2))
		comparison = -1;
	// Define macros that move us further and closer to the cut.
#define GET_BETTER(node) (comparison_type < 0 ? node->left : node->right)
#define GET_WORSE(node) (comparison_type < 0 ? node->right : node->left)
	if (comparison < 0) {
//		cout << "Wrong at: " << (long long) node->key << endl;
		// We're on the wrong side of the tree-cut, go left.
		if (GET_BETTER(node) == NULL)
			return 0; // No good value at all!
		return compute_augmentation_cut(aug, GET_BETTER(node), key, comparison_type, false, output);
	} else if (comparison == 0) {
//		cout << "Border at: " << (long long) node->key << endl;
		// We're JUST on the border of the tree-cut, left child only.
		if (GET_BETTER(node) == NULL) {
			// We have no left child, I guess it's just us.
			aug->base_case(node->key, node->value, output);
			return 1;
		}
		// Compute the base value just here.
		AugmentationResult temp;
		aug->base_case(node->key, node->value, &temp);
		AugmentationResult better_result;
		size_t elements = compute_augmentation_cut(aug, GET_BETTER(node), key, comparison_type, true, &better_result);
		// Early out if the better subtree had no good items at all.
		if (elements == 0) {
			*output = temp;
			return 1;
		}
		// This yields the output result.
		aug->compute(&better_result, &temp, output);
		return elements + 1;
	} else if (good_to_go) {
//		cout << "Great at: " << (long long) node->key << endl;
		// We're on entirely good side of the tree-cut, we can use a cached value!
		return compute_augmentation(aug, node, output);
	}
//	cout << "Right at: " << (long long) node->key << endl;
	// Finally, we're on the good side of the tree-cut, but our right subtree might not be.
	// Four cases: No children, just left, just right, both -- this handles all of them.
	AUG_COMPUTE_BOTH_SUBTREES(compute_augmentation_cut(aug, node->left, key, comparison_type, true, &temp), \
		compute_augmentation_cut(aug, node->right, key, comparison_type, false, &temp))
	*output = double_buf[i];
	return elements;
}

size_t Tree::augment_cut(int aug_id, void* key, int comparison_type, AugmentationResult* output) {
	assert(aug_ctx.augs.count(aug_id) == 1);
	Augmentation* aug = &aug_ctx.augs[aug_id];
	return compute_augmentation_cut(aug, root, key, comparison_type, false, output);
}

// Make some convenience functions.
#define AUG_CONV(name, val) \
size_t Tree::name(int aug_id, void* key, AugmentationResult* output) { \
	augment_cut(aug_id, key, val, output); \
}

AUG_CONV(augment_lt, -2)
AUG_CONV(augment_lte, -1)
AUG_CONV(augment_gte, 1)
AUG_CONV(augment_gt, 2)

