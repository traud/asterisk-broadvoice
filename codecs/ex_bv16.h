/*! \file
 *
 * \brief 8-bit raw data
 */

#include "asterisk/format_cache.h"      /* for ast_format_bv16      */
#include "asterisk/frame.h"             /* for ast_frame, etc       */

static uint8_t ex_bv16[] = {
	0xed, 0x89, 0x16, 0x44, 0x00, 0x69, 0xa6, 0x9a,
	0x69, 0xa6,
};

static struct ast_frame *bv16_sample(void)
{
	static struct ast_frame f = {
		.frametype = AST_FRAME_VOICE,
		.datalen = sizeof(ex_bv16),
		.samples = BV16_SAMPLES,
		.mallocd = 0,
		.offset = 0,
		.src = __PRETTY_FUNCTION__,
		.data.ptr = ex_bv16,
	};

	f.subclass.format = ast_format_bv16;

	return &f;
}
