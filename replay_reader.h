#ifndef REPLAY_READER_H
#define REPLAY_READER_H

#define RR_HEADER_PROPS_SIZE 8

#include <stdint.h>

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
	size_t size_main;
	size_t size_extra;
	int32_t *main_deck;
	int32_t *extra_deck;
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
