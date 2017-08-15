#ifndef YGO_REPLAY_H
#define YGO_REPLAY_H

#define YGO_REPLAY_PROPS_SIZE 8
#define YGO_REPLAY_MAIN_SIZE_MAX 60
#define YGO_REPLAY_EXTRA_SIZE_MAX 15
#define YGO_REPLAY_NUM_PLAYERS_MAX 4

#include <stdint.h>
#include <stdio.h>

struct ygo_replay_header
{
	uint32_t id;
	uint32_t version;
	uint32_t flag;
	uint32_t seed;
	uint32_t data_size;
	uint32_t hash;
	unsigned char props[YGO_REPLAY_PROPS_SIZE];
};

struct ygo_replay_deck
{
	char *owner;
	uint32_t main[YGO_REPLAY_MAIN_SIZE_MAX];
	uint32_t extra[YGO_REPLAY_EXTRA_SIZE_MAX];
	// sizes of the following two fields are computed
	// with `ceil(log2(n))` where `n` is the maximum
	// amount of cards that deck is allowed to hold.
	unsigned int size_main : 6;
	unsigned int size_extra : 4;
};

struct ygo_replay
{
	struct ygo_replay_deck decks[YGO_REPLAY_NUM_PLAYERS_MAX];
	struct ygo_replay_header header;
	size_t players_size;
	int32_t life_points;
	int32_t hand_count;
	int32_t draw_count;
};

int ygo_replay_read(struct ygo_replay *replay, const char *filename);
int ygo_replay_fread(struct ygo_replay *replay, const FILE *stream);
int ygo_replay_sread(struct ygo_replay *replay, const char *data, size_t size);

#endif /* YGO_REPLAY_H */
