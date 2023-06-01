/*! \file
 *
 * \brief Translate between signed linear and BroadVoice16 (BV16)
 *
 * \ingroup codecs
 */

/*** MODULEINFO
	<depend>bv16</depend>
 ***/

#include "asterisk.h"

#include "asterisk/codec.h"             /* for AST_MEDIA_TYPE_AUDIO */
#include "asterisk/frame.h"             /* for ast_frame            */
#include "asterisk/linkedlists.h"       /* for AST_LIST_NEXT, etc   */
#include "asterisk/logger.h"            /* for ast_log, etc         */
#include "asterisk/module.h"
#include "asterisk/translate.h"         /* for ast_trans_pvt, etc   */

/* these headers depend on each other, therefore, their order matters: */
#include <bv16-floatingpoint/bvcommon/typedef.h>
#include <bv16-floatingpoint/bvcommon/bvcommon.h>
#include <bv16-floatingpoint/bv16/bv16cnst.h>
#include <bv16-floatingpoint/bv16/bv16strct.h>
/* these headers are actually used, therefore, they only have to come last: */
#include <bv16-floatingpoint/bv16/bv16.h>
#include <bv16-floatingpoint/bv16/bitpack.h>

#define BUFFER_SAMPLES    8000
#define BV16_SAMPLES      40
#define BV16_FRAME_LEN    10

/* Sample frame data */
#include "asterisk/slin.h"
#include "ex_bv16.h"

struct bv16_translator_pvt {
	struct BV16_Encoder_State encoder;
	struct BV16_Decoder_State decoder;
	int16_t buf[BUFFER_SAMPLES];
};

static int bv16_new(struct ast_trans_pvt *pvt)
{
	struct bv16_translator_pvt *tmp = pvt->pvt;

	Reset_BV16_Encoder(&tmp->encoder);
	Reset_BV16_Decoder(&tmp->decoder);

	return 0;
}

/*! \brief decode and store in outbuf. */
static int bv16tolin_framein(struct ast_trans_pvt *pvt, struct ast_frame *f)
{
	struct bv16_translator_pvt *tmp = pvt->pvt;
	int x;

	for (x = 0; x < f->datalen; x += BV16_FRAME_LEN) {
		struct BV16_Bit_Stream stream;
		unsigned char *src = f->data.ptr + x;
		int16_t *dst = pvt->outbuf.i16 + pvt->samples;

		BV16_BitUnPack(src, &stream);
		BV16_Decode(&stream, &tmp->decoder, dst);

		pvt->samples += BV16_SAMPLES;
		pvt->datalen += BV16_SAMPLES * 2;
	}

	return 0;
}

/*! \brief store samples into working buffer for later decode */
static int lintobv16_framein(struct ast_trans_pvt *pvt, struct ast_frame *f)
{
	struct bv16_translator_pvt *tmp = pvt->pvt;

	memcpy(tmp->buf + pvt->samples, f->data.ptr, f->datalen);
	pvt->samples += f->samples;

	return 0;
}

/*! \brief encode and produce a frame */
static struct ast_frame *lintobv16_frameout(struct ast_trans_pvt *pvt)
{
	struct bv16_translator_pvt *tmp = pvt->pvt;
	struct ast_frame *result = NULL;
	struct ast_frame *last = NULL;
	int samples = 0; /* output samples */

	while (pvt->samples >= BV16_SAMPLES) {
		struct BV16_Bit_Stream stream;
		struct ast_frame *current;

		/* Encode a frame of data */
		BV16_Encode(&stream, &tmp->encoder, tmp->buf + samples);
		BV16_BitPack(pvt->outbuf.uc, &stream);

		samples += BV16_SAMPLES;
		pvt->samples -= BV16_SAMPLES;

		current = ast_trans_frameout(pvt, BV16_FRAME_LEN, BV16_SAMPLES);

		if (!current) {
			continue;
		} else if (last) {
			AST_LIST_NEXT(last, frame_list) = current;
		} else {
			result = current;
		}
		last = current;
	}

	/* Move the data at the end of the buffer to the front */
	if (samples) {
		memmove(tmp->buf, tmp->buf + samples, pvt->samples * 2);
	}

	return result;
}

static struct ast_translator bv16tolin = {
	.name = "bv16tolin",
	.src_codec = {
		.name = "bv16",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 8000,
	},
	.dst_codec = {
		.name = "slin",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 8000,
	},
	.format = "slin",
	.newpvt = bv16_new,
	.framein = bv16tolin_framein,
	.sample = bv16_sample,
	.desc_size = sizeof(struct bv16_translator_pvt),
	.buffer_samples = BUFFER_SAMPLES,
	.buf_size = BUFFER_SAMPLES * 2,
};

static struct ast_translator lintobv16 = {
	.name = "lintobv16",
	.src_codec = {
		.name = "slin",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 8000,
	},
	.dst_codec = {
		.name = "bv16",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 8000,
	},
	.format = "bv16",
	.newpvt = bv16_new,
	.framein = lintobv16_framein,
	.frameout = lintobv16_frameout,
	.sample = slin8_sample,
	.desc_size = sizeof(struct bv16_translator_pvt),
	.buffer_samples = BUFFER_SAMPLES,
	.buf_size = BUFFER_SAMPLES * 2,
};

static int unload_module(void)
{
	int res;

	res = ast_unregister_translator(&lintobv16);
	res |= ast_unregister_translator(&bv16tolin);

	return res;
}

static int load_module(void)
{
	int res;

	res = ast_register_translator(&bv16tolin);
	res |= ast_register_translator(&lintobv16);

	if (res) {
		unload_module();
		return AST_MODULE_LOAD_DECLINE;
	}

	return AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "BroadVoice16 (BV16) Coder/Decoder");
