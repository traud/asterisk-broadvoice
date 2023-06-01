/*! \file
 *
 * \brief Translate between signed linear and BroadVoice32 (BV32)
 *
 * \ingroup codecs
 */

/*** MODULEINFO
	<depend>bv32</depend>
 ***/

#include "asterisk.h"

#include "asterisk/codec.h"             /* for AST_MEDIA_TYPE_AUDIO */
#include "asterisk/frame.h"             /* for ast_frame            */
#include "asterisk/linkedlists.h"       /* for AST_LIST_NEXT, etc   */
#include "asterisk/logger.h"            /* for ast_log, etc         */
#include "asterisk/module.h"
#include "asterisk/translate.h"         /* for ast_trans_pvt, etc   */

/* these headers depend on each other, therefore, their order matters: */
#include <bvcommon/typedef.h>
#include <bvcommon/bvcommon.h>
#include <bv32/bv32cnst.h>
#include <bv32/bv32strct.h>
/* these headers are actually used, therefore, they only have to come last: */
#include <bv32/bv32.h>
#include <bv32/bitpack.h>

#define BUFFER_SAMPLES    16000
#define BV32_SAMPLES      80
#define BV32_FRAME_LEN    20

/* Sample frame data */
#include "asterisk/slin.h"
#include "ex_bv32.h"

struct bv32_translator_pvt {
	struct BV32_Encoder_State encoder;
	struct BV32_Decoder_State decoder;
	int16_t buf[BUFFER_SAMPLES];
};

static int bv32_new(struct ast_trans_pvt *pvt)
{
	struct bv32_translator_pvt *tmp = pvt->pvt;

	Reset_BV32_Coder(&tmp->encoder);
	Reset_BV32_Decoder(&tmp->decoder);

	return 0;
}

/*! \brief decode and store in outbuf. */
static int bv32tolin_framein(struct ast_trans_pvt *pvt, struct ast_frame *f)
{
	struct bv32_translator_pvt *tmp = pvt->pvt;
	int x;

	for (x = 0; x < f->datalen; x += BV32_FRAME_LEN) {
		struct BV32_Bit_Stream stream;
		unsigned char *src = f->data.ptr + x;
		int16_t *dst = pvt->outbuf.i16 + pvt->samples;

		BV32_BitUnPack(src, &stream);
		BV32_Decode(&stream, &tmp->decoder, dst);

		pvt->samples += BV32_SAMPLES;
		pvt->datalen += BV32_SAMPLES * 2;
	}

	return 0;
}

/*! \brief store samples into working buffer for later decode */
static int lintobv32_framein(struct ast_trans_pvt *pvt, struct ast_frame *f)
{
	struct bv32_translator_pvt *tmp = pvt->pvt;

	memcpy(tmp->buf + pvt->samples, f->data.ptr, f->datalen);
	pvt->samples += f->samples;

	return 0;
}

/*! \brief encode and produce a frame */
static struct ast_frame *lintobv32_frameout(struct ast_trans_pvt *pvt)
{
	struct bv32_translator_pvt *tmp = pvt->pvt;
	struct ast_frame *result = NULL;
	struct ast_frame *last = NULL;
	int samples = 0; /* output samples */

	while (pvt->samples >= BV32_SAMPLES) {
		struct BV32_Bit_Stream stream;
		struct ast_frame *current;

		/* Encode a frame of data */
		BV32_Encode(&stream, &tmp->encoder, tmp->buf + samples);
		BV32_BitPack(pvt->outbuf.uc, &stream);

		samples += BV32_SAMPLES;
		pvt->samples -= BV32_SAMPLES;

		current = ast_trans_frameout(pvt, BV32_FRAME_LEN, BV32_SAMPLES);

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

static struct ast_translator bv32tolin16 = {
	.name = "bv32tolin",
	.src_codec = {
		.name = "bv32",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 16000,
	},
	.dst_codec = {
		.name = "slin",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 16000,
	},
	.format = "slin",
	.newpvt = bv32_new,
	.framein = bv32tolin_framein,
	.sample = bv32_sample,
	.desc_size = sizeof(struct bv32_translator_pvt),
	.buffer_samples = BUFFER_SAMPLES,
	.buf_size = BUFFER_SAMPLES * 2,
};

static struct ast_translator lin16tobv32 = {
	.name = "lintobv32",
	.src_codec = {
		.name = "slin",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 16000,
	},
	.dst_codec = {
		.name = "bv32",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 16000,
	},
	.format = "bv32",
	.newpvt = bv32_new,
	.framein = lintobv32_framein,
	.frameout = lintobv32_frameout,
	.sample = slin16_sample,
	.desc_size = sizeof(struct bv32_translator_pvt),
	.buffer_samples = BUFFER_SAMPLES,
	.buf_size = BUFFER_SAMPLES * 2,
};

static int unload_module(void)
{
	int res;

	res = ast_unregister_translator(&lin16tobv32);
	res |= ast_unregister_translator(&bv32tolin16);

	return res;
}

static int load_module(void)
{
	int res;

	res = ast_register_translator(&bv32tolin16);
	res |= ast_register_translator(&lin16tobv32);

	if (res) {
		unload_module();
		return AST_MODULE_LOAD_DECLINE;
	}

	return AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "BroadVoice32 (BV32) Coder/Decoder");
