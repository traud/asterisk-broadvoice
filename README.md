# BroadVoice32 for Asterisk

This is an implementation of BroadVoice32 (BV32; [RFC 4298](http://tools.ietf.org/html/rfc4298). Research papers comparing BroadVoice with other audio codecs were published at [InterSpeech 2010](http://research.nokia.com/files/public/%5B12%5D_Interspeech%202010_Voice%20Quality%20Evaluation%20of%20Recent%20Open%20Source%20Codecs.pdf).

To add a codec for SIP/SDP (m= and rtmap), you create a format module in Asterisk: `bv32_pass-through.patch`. However, this requires both call legs to support BV32 (pass-through only). If one leg does not support BV32, the call has no audio. Or, if you use the pre-recorded voice and music files of Asterisk, these files cannot be heard, because they are not in BV32 but in slin. Therefore, this repository adds not just a format module for the audio-codec BV32 but a transcoding module as well: `broadvoice-transcoding.patch` and `codecs/codec_bv32.c`.

## Installing the patch

At least Asterisk 13.7 is required. These changes were last tested with Asterisk 13.38.3 (and Asterisk 20). If you use a newer version and the patch fails, please, [report](https://help.github.com/articles/creating-an-issue/)!

	cd /usr/src/
	wget downloads.asterisk.org/pub/telephony/asterisk/old-releases/asterisk-13.38.3.tar.gz
	tar zxf ./asterisk*
	cd ./asterisk*
	sudo apt --no-install-recommends install autoconf automake build-essential pkg-config libedit-dev libjansson-dev libsqlite3-dev uuid-dev libxslt1-dev xmlstarlet

Apply the patch:

	wget github.com/traud/asterisk-broadvoice/archive/master.tar.gz
	tar -xf master.tar.gz
	rm master.tar.gz
	cp --verbose --recursive ./asterisk-broadvoice*/* ./
	patch -p0 <./bv32_pass-through.patch

Install libraries:

If you do not want transcoding but pass-through only, please, skip this step. To support transcoding, you’ll need to install the BV32 library from FreeSWITCH, for example in Debian/Ubuntu:

	wget files.freeswitch.org/downloads/libs/broadvoice-0.1.0.tar.gz
	tar -xf broadvoice-0.1.0.tar.gz
	cd ./broadvoice-0.1.0
	sudo apt --no-install-recommends install autoconf automake build-essential libtool libltdl-dev
	./autogen.sh
	./configure
	make
	sudo make install
	cd /usr/src/asterisk*
	patch -p0 <./broadvoice_transcoding.patch

Run the bootstrap script to re-generate configure:

	./bootstrap.sh

Configure your patched Asterisk:

	./configure

Enable slin16 in menuselect for transcoding, for example via:

	make menuselect.makeopts
	./menuselect/menuselect --enable-category MENUSELECT_CORE_SOUNDS

Compile and install:

	make
	sudo make install

## Testing

The transcoding module was tested against the Snom M900 with DSP module A900 (firmware 06.50). The results should be the same for all [RTX RTX8663](https://www.rtx.dk/rtx-products/multi-cell-base-station-rtx8663/#DownloadProductSheet) based DECT base stations. That uses the same audio codec on the DECT interface, with just one slot. That is in contrast to other HD Voice codecs which require G.722 which uses two DECT slots. In other words, you are able to make more simultaneous calls. However, the quality is horrible and the cause is unknown. The BV32 library exists is several versions and variants. The variant from FreeSWITCH is based on version 1.0. The latest version is 1.2 and still available at [Broadcom…](https://docs.broadcom.com/docs/12358448) That version was tested with 64-bit and against different endian format of processing platform. However, before you build, you have to set the preprocessor directive G192BITSTREAM=0.

## Thanks go to
* [Steve Underwood](https://www.coppice.org/) for converting the BroadVoice Library 1.0 into a shared library with a easy-to-use API.
* Asterisk team: Thanks to their efforts and architecture this AMR module was written in one working day.
