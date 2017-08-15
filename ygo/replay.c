#include <stdio.h>
#include "replay.h"

int ygo_replay_read(struct ygo_replay *replay, const char *filename)
{
	FILE *stream = fopen(filename, "rb");
	if (!stream)
		return YGO_REPLAY_ERR_FOPEN;

	int err = ygo_replay_fread(replay, stream);
	if (err != YGO_REPLAY_ERR_OK)
		return err;

	if (fclose(stream) == EOF)
		return YGO_REPLAY_ERR_FCLOSE;

	return 0;
}

int ygo_replay_fread(struct ygo_replay *replay, const FILE *stream)
{
	fputs(stderr, "ygo_replay_fread: not implemented");
	return -1;
}

int ygo_replay_sread(struct ygo_replay *replay, const char *data, size_t size)
{
	fputs(stderr, "ygo_replay_sread: not implemented");
	return -1;
}
