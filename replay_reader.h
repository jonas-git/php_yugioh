#ifndef REPLAY_READER_H
#define REPLAY_READER_H

#define RR_HEADER_PROPS_SIZE 8
#define RR_MAIN_DECK_MAX_SIZE 60
#define RR_EXTRA_DECK_MAX_SIZE 15

#include <stdint.h>
#include <stdio.h>

struct rr_replay_header
{
	uint32_t id;
	uint32_t version;
	uint32_t flag;
	uint32_t seed;
	uint32_t data_size;
	uint32_t hash;
	unsigned char props[RR_HEADER_PROPS_SIZE];
};

struct rr_deck_info
{
	int32_t main_deck[RR_MAIN_DECK_MAX_SIZE];
	int32_t extra_deck[RR_EXTRA_DECK_MAX_SIZE];
	unsigned int size_main : 6; // ceil(log2(max_size))
	unsigned int size_extra : 4; // ceil(log2(max_size))
};

struct rr_replay
{
	struct rr_replay_header *header;
	struct rr_deck_info *decks;
	char **players;
	size_t player_count;
	int32_t life_points;
	int32_t hand_count;
	int32_t draw_count;
};

struct rr_replay *rr_read_replay(const char *file);
struct rr_replay *rr_read_replay_f(FILE *stream);
struct rr_replay *rr_read_replay_a(const void *data, size_t size);
void rr_destroy_replay(struct rr_replay *replay);

#endif // !REPLAY_READER_H
