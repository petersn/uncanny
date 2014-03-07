// Uncanny trees test file.

using namespace std;
#include <iostream>
#include "uncanny.h"
using namespace Uncanny;

int integer_compare(void* _a, void* _b) {
	long long a = (long long)_a, b = (long long)_b;
	if (a > b) return 1;
	if (a == b) return 0;
	return -1;
}

void sum_base_case(void* key, void* value, AugmentationResult* output) {
	cout << "   [" << (long long)value << "]" << endl;
	output->data.l = (long long)value;
	output->data_length = 1;
}

void sum_compute(const AugmentationResult* a, const AugmentationResult* b, AugmentationResult* output) {
	cout << "   [" << a->data.l << " + " << b->data.l << "]" << endl;
	output->data.l = a->data.l + b->data.l;
	output->data_length = a->data_length + b->data_length;
}

bool sum_compare(const AugmentationResult* a, const AugmentationResult* b) {
	return a->data.l == b->data.l;
}

int main(int argc, char** argv) {
	// Make a tree, and do some basic tests.
	Tree t;
	t.cmp_f = integer_compare;

	Augmentation sum_aug(sum_base_case, sum_compute, sum_compare);
	int aug_id = t.aug_ctx.new_augmentation(&sum_aug);

	cout << "Augmentation has ID: " << aug_id << endl;

	for (long long i=0; i<30; i++)
		t.insert((void*)i, (void*)i);

	AugmentationResult output;

//	t.augment_lt(aug_id, (void*)1000, &output);

//	t.pprint();

	for (int i=0; i<5; i++) {
//		t.augment_lte(aug_id, (void*)24, &output);
//		cout << "Result: " << (long long)output.data.l << endl;
		if (i == 1)
			t.remove((void*)15);
		if (i == 2)
			t.insert((void*)15, (void*)20);
		if (i == 3)
			t.insert((void*)1, (void*)11);
	}

	for (int i=0; i<2; i++) {
		t.augment_range(aug_id, (void*)7, false, (void*)26, true, &output);
		cout << "Range result: " << (long long)output.data.l << endl;
	}

	return 0;
}

