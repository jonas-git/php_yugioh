#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include "lzma/C/Alloc.h"
#include "lzma/C/LzmaDec.h"
#include "replay_reader.h"
#include "util.h"

#define RR_NAME_SIZE 40 / 2

typedef unsigned char byte;

long fsize(FILE *stream)
{
	long pos = ftell(stream);
	fseek(stream, 0, SEEK_END);
	long size = ftell(stream);
	fseek(stream, 0, pos);
	return size;
}

void rr_destroy_replay(struct rr_replay *replay)
{
	if (!replay) return;

	size_t i;
	for (i = 0; i < replay->player_count; ++i)
		free(replay->players[i]);
	for (i = 0; i < replay->player_count; ++i) {
		free(replay->decks[i].main_deck);
		free(replay->decks[i].extra_deck);
	}

	free(replay->header);
	free(replay->players);
	free(replay->decks);
	free(replay);
}

void extract_replay_header(struct rr_replay_header *header, const byte **data)
{
	uint32_t *data_uint32;
	byte *data_byte;

	data_uint32 = (uint32_t *)*data;
	header->id = *data_uint32++;
	header->version = *data_uint32++;
	header->flag = *data_uint32++;
	header->seed = *data_uint32++;
	header->data_size = *data_uint32++;
	header->hash = *data_uint32++;
	data_byte = (byte *)data_uint32;

	char i;
	for (i = 0; i < RR_HEADER_PROPS_SIZE; ++i) {
		header->props[i] = *data_byte++;
	}

	*data = data_byte;
}

void extract_player_name(char **player, const byte **data)
{
	const byte *data_byte = *data;

	char *player_name = (char *)calloc(RR_NAME_SIZE, sizeof(char));
	if (!player_name) { exit(EXIT_FAILURE); }

	size_t i;
	for (i = 0; i < RR_NAME_SIZE; ++i, data_byte += 2) {
		if ((player_name[i] = *data_byte) == '\0') {
			break;
		}
	}
	data_byte += 2 * (RR_NAME_SIZE - i);

	*player = player_name;
	*data = data_byte;
}

void extract_replay_data(struct rr_replay *replay, struct rr_replay_header *header, const byte **data)
{
	bool is_tag = (header->flag & 0x2) != 0;
	size_t i, size = is_tag ? 4 : 2;
	int32_t j;

	replay->player_count = size;
	replay->players = (char **)calloc(size, sizeof(char) * RR_NAME_SIZE);
	if (!replay->players) { exit(EXIT_FAILURE); }
	for (i = 0; i < size; ++i) {
		extract_player_name(&replay->players[i], data);
	}

	const int32_t *data_int32 = (const int32_t *)*data;
	replay->life_points = *data_int32++;
	replay->hand_count = *data_int32++;
	replay->draw_count = *data_int32++;
	++data_int32; // other

	replay->decks = (struct rr_deck_info *)calloc(size, sizeof(struct rr_deck_info));
	if (!replay->decks) { exit(EXIT_FAILURE); }

	for (i = 0; i < size; ++i) {
		int32_t main_deck_size = *data_int32++;
		replay->decks[i].size_main = main_deck_size;
		replay->decks[i].main_deck = (int32_t *)calloc(main_deck_size, sizeof(int32_t));
		if (!replay->decks[i].main_deck) { exit(EXIT_FAILURE); }
		for (j = 0; j < main_deck_size; ++j) {
			replay->decks[i].main_deck[j] = *data_int32++;
		}
		
		int32_t extra_deck_size = *data_int32++;
		replay->decks[i].size_extra = extra_deck_size;
		replay->decks[i].extra_deck = (int32_t *)calloc(extra_deck_size, sizeof(int32_t));
		if (!replay->decks[i].extra_deck) { exit(EXIT_FAILURE); }
		for (j = 0; j < extra_deck_size; ++j) {
			replay->decks[i].extra_deck[j] = *data_int32++;
		}
	}

	*data = (const byte *)data_int32;
}

struct rr_replay *rr_read_replay(const char *file)
{
	FILE *stream = NULL;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	int err = fopen_s(&stream, file, "rb");
	if (err != 0 || !stream) { return NULL; }
#else
	stream = fopen(file, "rb");
#endif

	struct rr_replay *replay = rr_read_replay_f(stream);
	fclose(stream);
	return replay;
}

struct rr_replay *rr_read_replay_f(FILE *stream)
{
	long file_size = fsize(stream);
	byte *data = (byte *)calloc(file_size, sizeof(byte));
	if (!data) { exit(EXIT_FAILURE); }

	size_t size = fread(data, sizeof(byte), file_size, stream);
	if (!data || size == 0) {
		free(data);
		return NULL;
	}

	struct rr_replay *replay = rr_read_replay_a(data, size);

	free(data);
	return replay;
}

struct rr_replay *rr_read_replay_a(const void *data, size_t size)
{
	const byte *data_byte = (const byte *)data;
	const byte *current_data_byte = data_byte;

	struct rr_replay *replay = (struct rr_replay *)calloc(1, sizeof(struct rr_replay));
	if (!replay) { exit(EXIT_FAILURE); }

	replay->header = (struct rr_replay_header *)calloc(1, sizeof(struct rr_replay_header));
	if (!replay->header) { exit(EXIT_FAILURE); }

	extract_replay_header(replay->header, &current_data_byte);

	size_t header_size = (size_t)(current_data_byte - data_byte);
	size_t compressed_size = size - header_size;

	if ((replay->header->flag & 0x1) != 0) { // if compressed
		const byte *in_data = current_data_byte;
		byte *out_data = (byte *)calloc(replay->header->data_size, sizeof(byte));
		if (!out_data) { exit(EXIT_FAILURE); }

		ELzmaStatus status;
		size_t data_size = replay->header->data_size;
		SRes result = LzmaDecode(out_data, &data_size, in_data, &compressed_size,
			replay->header->props, RR_HEADER_PROPS_SIZE, LZMA_FINISH_ANY, &status, &g_Alloc);

		if (result ^ SZ_OK) {
			rr_destroy_replay(replay);
			free(out_data);
			return NULL;
		}

		const byte *current_out_data = out_data;
		extract_replay_data(replay, replay->header, &current_out_data);
		free(out_data);
	}
	else {
		extract_replay_data(replay, replay->header, &current_data_byte);
	}

	return replay;
}
