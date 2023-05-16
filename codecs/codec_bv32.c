/*! \file
 *
 * \brief Translate between signed linear and BroadVoice32 (BV32)
 *
 * \ingroup codecs
 */

/*** MODULEINFO
	<depend>broadvoice</depend>
 ***/

#include "asterisk.h"

#include "asterisk/codec.h"             /* for AST_MEDIA_TYPE_AUDIO */
#include "asterisk/frame.h"             /* for ast_frame            */
#include "asterisk/linkedlists.h"       /* for AST_LIST_NEXT, etc   */
#include "asterisk/logger.h"            /* for ast_log, etc         */
#include "asterisk/module.h"
#include "asterisk/translate.h"         /* for ast_trans_pvt, etc   */

#include <broadvoice.h>

#define BUFFER_SAMPLES    16000
#define BV32_SAMPLES      320

/* Sample frame data */
#include "asterisk/slin.h"
#include "ex_bv32.h"

struct bv32_translator_pvt {
	bv32_encode_state_t *encoder;
	bv32_decode_state_t *decoder;
	int16_t buf[BUFFER_SAMPLES];
};

static int bv32_new(struct ast_trans_pvt *pvt)
{
	struct bv32_translator_pvt *tmp = pvt->pvt;

	tmp->encoder = bv32_encode_init(NULL);
	tmp->decoder = bv32_decode_init(NULL);

	if (!tmp->encoder || !tmp->decoder) {
		ast_log(LOG_ERROR, "Error creating BroadVoice32 (BV32) conversion\n");
		return -1;
	}

	return 0;
}

/*! \brief decode and store in outbuf. */
static int bv32tolin_framein(struct ast_trans_pvt *pvt, struct ast_frame *f)
{
	struct bv32_translator_pvt *tmp = pvt->pvt;
	int x;

	for (x = 0; x < f->datalen; x += BV32_FRAME_LEN) {
		unsigned char *src = f->data.ptr + x;
		int16_t *dst = pvt->outbuf.i16 + pvt->samples;

		int len = bv32_decode(tmp->decoder, dst, src, BV32_FRAME_LEN);

		pvt->samples += BV32_SAMPLES;
		pvt->datalen += len;
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
		struct ast_frame *current;

		/* Encode a frame of data */
		int len = bv32_encode(tmp->encoder, pvt->outbuf.uc, tmp->buf + samples, BV32_SAMPLES);

		samples += BV32_SAMPLES;
		pvt->samples -= BV32_SAMPLES;

		current = ast_trans_frameout(pvt, len, BV32_SAMPLES);

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

static void bv32_destroy_stuff(struct ast_trans_pvt *pvt)
{
	struct bv32_translator_pvt *tmp = pvt->pvt;

	if (tmp->encoder) {
		bv32_encode_free(tmp->encoder);
	}

	if (tmp->decoder) {
		bv32_decode_free(tmp->decoder);
	}
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
	.destroy = bv32_destroy_stuff,
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
	.destroy = bv32_destroy_stuff,
	.sample = slin16_sample,
	.desc_size = sizeof(struct bv32_translator_pvt),
	.buffer_samples = BUFFER_SAMPLES,
	.buf_size = BUFFER_SAMPLES,
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
