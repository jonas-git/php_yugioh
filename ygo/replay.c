#include <stdio.h>
#include <stdlib.h>
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
	fputs("ygo_replay_sread: not implemented", stderr);
	return -1;
}

// if there is not enough data, a zero value is returned
// and the `header` stays untouched. otherwise returns non-zero.
static size_t ygo_replay_parse_header(struct ygo_replay_header *header,
                                      const char *data, size_t size)
{
	if (YGO_REPLAY_HEADER_SIZE > size)
		return 0;

	const uint32_t *data_32 = (const uint32_t *)data;
	header->id        = *data_32++;
	header->version   = *data_32++;
	header->flag      = *data_32++;
	header->seed      = *data_32++;
	header->data_size = *data_32++;
	header->hash      = *data_32++;

	data = (const char *)data_32;
	for (unsigned int i = 0; i < YGO_REPLAY_PROPS_SIZE; ++i)
		header->props[i] = *data++;

	return YGO_REPLAY_HEADER_SIZE;
}
