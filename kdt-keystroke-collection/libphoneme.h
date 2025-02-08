#define byte unsigned char
#ifndef LIBPHONEME_H
#define LIBPHONEME_H
struct phoneme {
	char length;
	char *str;
};

struct kv_pair {
	struct phoneme *key;
	unsigned long *value;
	struct kv_pair *next;
};

// Hashmap stuff
struct phoneme* phoneme_create(char *str, byte length); 
short hash_phoneme_struct(struct phoneme *p);
bool phoneme_compare(struct phoneme *p1, struct phoneme *p2);
struct kv_pair *kv_pair_create(struct phoneme *key, unsigned long *value);
bool hashmap_set(struct kv_pair **hashmap, struct phoneme *key, unsigned long *value);
struct time_delta_array* hashmap_get(struct kv_pair **hashmap, struct phoneme *key);
#endif
