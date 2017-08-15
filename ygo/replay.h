#ifndef YGO_REPLAY_H
#define YGO_REPLAY_H

#define YGO_REPLAY_PROPS_SIZE 8
#define YGO_REPLAY_MAIN_SIZE_MAX 60
#define YGO_REPLAY_EXTRA_SIZE_MAX 15
#define YGO_REPLAY_NUM_PLAYERS_MAX 4
// cannot use `sizeof(struct ygo_replay_header)`
// due to the possibility of padding.
#define YGO_REPLAY_HEADER_SIZE \
	(6 * sizeof(uint32_t) + sizeof(char[YGO_REPLAY_PROPS_SIZE]))

#define YGO_REPLAY_ERR_OK 0
#define YGO_REPLAY_ERR_FOPEN 1
#define YGO_REPLAY_ERR_FCLOSE 2
#define YGO_REPLAY_ERR_FSEEK 3
#define YGO_REPLAY_ERR_FREAD 4
#define YGO_REPLAY_ERR_MALLOC 5

#include <stdint.h>
#include <stdio.h>

// holds meta-information about a Yu-Gi-Oh! replay.
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

// holds information about a single player in a replay
// including his name and the cards in the deck he played.
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

// holds general information about a yugioh duel (in a replay).
// this does not contain the progress and the result of it.
struct ygo_replay
{
	struct ygo_replay_deck decks[YGO_REPLAY_NUM_PLAYERS_MAX];
	struct ygo_replay_header header;
	size_t players_size;
	int32_t life_points;
	int32_t hand_count;
	int32_t draw_count;
};

/* ygo_replay_read(replay, filename):
 |     opens the specified file `filename`, parses its content and writes
 |     the result into the [struct ygo_replay] at the address of `replay`.
 |  -> returns an error code that indicates how the function terminated
 | replay:     pointer to a [struct ygo_replay]
 | filename:   path to a compressed replay file
 */
int ygo_replay_read(struct ygo_replay *replay, const char *filename);

/* ygo_replay_fread(replay, stream):
 |     parses the content of the file `stream` and writes the result
 |     into the [struct ygo_replay] at the address of `replay`.
 |  -> returns an error code that indicates how the function terminated
 | replay:   pointer to a [struct ygo_replay]
 | stream:   pointer to an opened [FILE] containing compressed replay data
 */
int ygo_replay_fread(struct ygo_replay *replay, const FILE *stream);

/* ygo_replay_sread(replay, data, size):
 |     parses the passed `data` and writes the result
 |     into the [struct ygo_replay] at the address of `replay`.
 |  -> returns an error code that indicates how the function terminated
 | replay:   pointer to a [struct ygo_replay]
 | data:     string of compressed replay data
 | size:     size of the data
 */
int ygo_replay_sread(struct ygo_replay *replay, const char *data, size_t size);

#endif /* YGO_REPLAY_H */
