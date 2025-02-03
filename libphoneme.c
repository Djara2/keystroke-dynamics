#include <string.h>
#include "libphoneme.h"
struct phoneme* phoneme_create(char *str, byte length) {
	if(str == NULL) {
		fprintf(stderr, "[phoneme_create] Cannot create a phoneme using a string that points to NULL.\n");
		return NULL;
	}
	if(length <= 1) {
		fprintf(stderr, "[phoneme_create] Cannot create a phoneme of length 0 or 1.\n");
		return NULL;
	}

	struct phoneme *new_phoneme = malloc(sizeof(struct phoneme));
	if(new_phoneme == NULL) {
		fprintf(stderr, "[phoneme_create] Failed to allocate sufficient memory to create a new phoneme struct.\n");
		return NULL;
	}

	// Set string member
	new_phoneme->str = malloc( (sizeof(char) * length) + 1 );
	if(new_phoneme->str == NULL) {
		fprintf(stderr, "[phoneme_create] Failed to allocate sufficient memory to create the memory buffer for the new phoneme's string attribute.\n");
		return NULL;
	}
	strcpy(new_phoneme->str, str);

	// Set length member
	new_phoneme->length = length;

	return new_phoneme;
}

bool phoneme_compare(struct phoneme *p1, struct phoneme *p2) {
	if(p1 == NULL) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme that points to NULL");
		return false;
	} 
	if(p1->str == NULL) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme with a string member that points to NULL.\n");
		return false;
	}
	if(p1->length <= 1) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme with an invalid length member. Phoneme lengths must be greater than 1.\n");
		return false;
	}
	if(p2 == NULL) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme that points to NULL.\n");
		return false;
	}
	if(p2->str == NULL) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme with a string member that points to NULL.\n");
		return false;
	}
	if(p2->length <= 1) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme with an invalid length member. Phoneme lengths must be greater than 1.\n");
		return false;
	}
	
	if(p1->length != p2->length)      return false;
	if(strcmp(p1->str, p2->str) != 0) return false;

	return true;
}

// key is required, but time deltas is not (it can be NULL). Just check for this later
struct kv_pair *kv_pair_create(struct phoneme *key, struct time_delta_array *value) {
	if(key == NULL) {
		fprintf(stderr, "[kv_pair_create] Cannot create a new key-value pair with a phoneme key that points to NULL.\n");
		return NULL;
	}

	// Create new pair
	struct kv_pair *new_kv_pair = malloc(sizeof(struct kv_pair));
	if(new_kv_pair == NULL) {
		fprintf(stderr, "[kv_pair_create] Failed to allocate memory for a new kv_pair struct.\n");
		return NULL;
	}

	// Assign key and value
	new_kv_pair->key = key;	
	new_kv_pair->value = value;
	new_kv_pair->next = NULL;
}

short hash_phoneme_struct(struct phoneme *p) {
	if(p == NULL) {
		fprintf(stderr, "[hash_phoneme_struct] Cannot hash phoneme struct that points to NULL.\n");
		return -1;
	}
	
	unsigned long hash = 0;
	for(int i = 0; i < p->length; i++) 
		hash += p->str[i];

	hash *= HASH_SCALAR;
	hash = hash % MODULUS;
	return (unsigned short) hash;
}

bool hashmap_set(struct kv_pair **hashmap, struct phoneme *key, struct time_delta_array *value) {
	if(hashmap == NULL) {
		fprintf(stderr, "[hashmap_insert] Insertion into a hashmap that points to NULL cannot be done.\n");
		return false;
	}

	if(key == NULL) {
		fprintf(stderr, "[hashmap_insert] Insertion of a phoneme key that points to NULL cannot be done.\n");
		return false;
	}

	short hashed_index = hash_phoneme_struct(key);
	if(hashed_index == -1) { 
		fprintf(stderr, "[hashmap_insert] Hashed index came back as -1 for provided phoneme key.\n");
		return false;
	}
	
	// Case 1: Index is NULL, meaning it is has never been used before and is free for insertion
	if(hashmap[hashed_index] == NULL) {
		hashmap[hashed_index] = kv_pair_create(key, value);
		if(hashmap[hashed_index] == NULL) {
			fprintf(stderr, "[hashmap_insert] Index %hd assigned to NULL after call to kv_pair_create.\n");
			return false;
		}
		return true;
	}
	else {
	// Case 2: Index is occupied. The key may or may not already exist
		struct kv_pair *current = hashmap[hashed_index];
		while(current->next != NULL)
		{
			// Case 2.1: Key already exists, just as a node in a linear chain. Update value and then return true.
			if(phoneme_compare(hashmap[hashed_index]->key, key) == 0) {
				// Prevent memory leaks
				if(hashmap[hashed_index]->value != NULL) {
					free(hashmap[hashed_index]->value->values);
					free(hashmap[hashed_index]->value);
				}
				// Update value mapped by pre-existing key and return
				hashmap[hashed_index]->value = value;
				return true;
			}
		
			current = current->next;
		}
		// Case 2.2: Key does NOT exist. Add it via linear probing.
		current->next = kv_pair_create(key, value);
		if(current->next == NULL) {
			fprintf(stderr, "[hashmap_set] Failed to insert new hashmap key. Call to kv_pair_create returned NULL.\n");
			return false;
		}
		return true;
	}
}

struct time_delta_array* hashmap_get(struct kv_pair **hashmap, struct phoneme *key) {
	if(hashmap == NULL) {
		fprintf(stderr, "[hashmap_get] Cannot retrieve values from hashmap that points to NULL.\n");
		return NULL;
	}
	if(key == NULL) {
		fprintf(stderr, "[hashmap_get] Cannot search hashmap for a key that points to NULL.\n");
		return NULL;
	}

	short hashed_index = hash_phoneme_struct(key);
	if(hashed_index == -1) {
		fprintf(stderr, "[hashmap_get] Failed to hash provided key. Function hash_phoneme_struct returned -1.\n");
		return NULL;
	}
	
	// Case 1: Immediate miss
	if(hashmap[hashed_index] == NULL) {
		fprintf(stderr, "[hashmap_get] Cannot get the value mapped to a key that does not exist in the hashmap.\n");
		return NULL;
	}
	else{
		// Do linear probing
		struct kv_pair *current = hashmap[hashed_index];
		while(current->next != NULL) {
			// Case 2.1: Key found, return value
			if(phoneme_compare(current->key, key) == 0)
				return current->value;
			
			current = current->next;
		}

		// Case 2.2: Key does not exist in the hashmap, return NULL
		return NULL;
	}
}
