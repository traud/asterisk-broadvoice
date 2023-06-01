/*! \file
 *
 * \brief 16-bit raw data
 */

#include "asterisk/format_cache.h"      /* for ast_format_bv32      */
#include "asterisk/frame.h"             /* for ast_frame, etc       */

static uint8_t ex_bv32[] = {
	0xed, 0x89, 0x16, 0x44, 0x00, 0x69, 0xa6, 0x9a,
	0x69, 0xa6, 0x9a, 0x69, 0xa6, 0x9a, 0x69, 0x9e,
	0x79, 0xe7, 0x9e, 0x79,
};

static struct ast_frame *bv32_sample(void)
{
	static struct ast_frame f = {
		.frametype = AST_FRAME_VOICE,
		.datalen = sizeof(ex_bv32),
		.samples = BV32_SAMPLES,
		.mallocd = 0,
		.offset = 0,
		.src = __PRETTY_FUNCTION__,
		.data.ptr = ex_bv32,
	};

	f.subclass.format = ast_format_bv32;

	return &f;
}
