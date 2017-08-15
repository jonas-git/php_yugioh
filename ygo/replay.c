#include <stdio.h>
#include "replay.h"

int ygo_replay_read(struct ygo_replay *replay, const char *filename)
{
	FILE *stream = fopen(filename, "rb");
	if (!stream)
		return YGO_REPLAY_ERR_FOPEN;

	int err = ygo_replay_fread(replay, stream);
	if (err != YGO_REPLAY_ERR_OK) {
		// the error occured in `ygo_replay_fread` has higher
		// priority than one that might occur in `fclose`.
		fclose(stream);
		return err;
	}

	if (fclose(stream) == EOF)
		return YGO_REPLAY_ERR_FCLOSE;

	return 0;
}

int ygo_replay_fread(struct ygo_replay *replay, const FILE *stream)
{
	int err = 0;

	if (fseek(stream, 0, SEEK_END) != 0) {
		err = YGO_REPLAY_ERR_FSEEK;
		goto err_fseek;
	}
	long size = ftell(stream);
	rewind(stream);

	char *buf = malloc((size + 1) * sizeof(char));
	if (!buf) {
		err = YGO_REPLAY_ERR_MALLOC;
		goto err_malloc;
	}

	size_t nread = fread(buf, sizeof(char), size, stream);
	if (nread != size && ferror(stream) != 0) {
		err = YGO_REPLAY_ERR_FREAD;
		goto err_fread;
	}

	err = ygo_replay_sread(replay, buf, nread);

err_fread:
	free(buf);
err_malloc:
err_fseek:
	return err;
}

int ygo_replay_sread(struct ygo_replay *replay, const char *data, size_t size)
{
	fputs(stderr, "ygo_replay_sread: not implemented");
	return -1;
}
