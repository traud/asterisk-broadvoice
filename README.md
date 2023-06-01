# BroadVoice32 for Asterisk

This is an implementation of the HD audio codec BroadVoice32 (BV32; [RFC 4298](https://tools.ietf.org/html/rfc4298)). A research paper comparing BroadVoice with other audio codecs was published at [InterSpeech 2010](https://www.isca-speech.org/archive/interspeech_2010/ramo10_interspeech.html).

To add a codec for SIP/SDP (m= and rtmap), you create a format module in Asterisk: `bv32_pass-through.patch`. However, both call legs must support BV32 (pass-through only). If one leg does not support BV32, the call has no audio. Or, if you use the pre-recorded voice and music files of Asterisk, these files cannot be heard because they are not in BV32 but in slin. Therefore, this repository adds not just a format module for the audio-codec BV32 but also a transcoding module: `broadvoice_transcoding.patch` and `codecs/codec_bv32.c`.

## Installing the patch

At least Asterisk 13.7 is required. These changes were last tested with Asterisk 13.38.3 (and Asterisk 20.3.0). If you use a newer version and the patch fails, please, [report](https://help.github.com/articles/creating-an-issue/)!

	cd /usr/src/
	wget downloads.asterisk.org/pub/telephony/asterisk/old-releases/asterisk-13.38.3.tar.gz
	tar zxf ./asterisk*
	cd ./asterisk*/
	sudo apt install autoconf automake build-essential pkg-config libedit-dev libjansson-dev libsqlite3-dev uuid-dev libxslt1-dev xmlstarlet

Apply the patch:

	wget github.com/traud/asterisk-broadvoice/archive/master.tar.gz
	tar zxf master.tar.gz
	rm master.tar.gz
	cp --verbose --recursive ./asterisk-broadvoice*/* ./
	patch -p0 <./bv32_pass-through.patch

If you still use Asterisk 13, go for `bv32_pass-through_13.patch` instead.

Install libraries:

If you do not want transcoding but pass-through only, please, skip this step. To support transcoding, you’ll need to install the BroadVoice library from SignalWire FreeSWITCH, for example, in Debian/Ubuntu:

	wget files.freeswitch.org/downloads/libs/broadvoice-0.1.0.tar.gz
	tar zxf broadvoice*
	cd ./broadvoice*/
	sudo apt install autoconf automake build-essential libltdl-dev libtool
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

Enable `slin16` in menuselect for transcoding, for example via:

	make menuselect.makeopts
	./menuselect/menuselect --enable-category MENUSELECT_CORE_SOUNDS

Compile and install:

	make
	sudo make install

## Testing

Several VoIP/SIP implementations use BroadVoice16:
* [Linphone](https://github.com/BelledonneCommunications/mediastreamer2/blob/2ef88ff7f9b3506b7c10e6925c36ccba2f151f86/src/audiofilters/bv16.c)
* [Sipdroid](https://github.com/i-p-tel/sipdroid/blob/ef15c6d68cf402088c2960802d2dec87be15f2a9/app/src/main/jni/bv16_jni.cpp)

Unlike with other audio codecs, ‘16’ is not about the sample rate but bit rate. It is a non-HD audio codec. Although not compatible with this module, those projects can be used as coding example for the original BroadVoice library.

Several VoIP/SIP implementations use BroadVoice32:
* Aastra 6700i and 6800i Series, removed with firmware 4.1.x (Jun. 2015; DEF39955)
* Adran IP 706/712
* [Baresip](https://github.com/traud/asterisk-broadvoice/wiki), removed with version 0.6.5
* CounterPath X-lite [4.5.5](https://web.archive.org/web/20151120063639/http://counterpath.s3.amazonaws.com/downloads/X-Lite_Win32_4.5.5.2_76432.exe) (for Windows XP)
* CounterPath X-lite [4.7.1](https://web.archive.org/web/20160306091234/http://counterpath.s3.amazonaws.com/downloads/X-Lite_Win32_4.7.1_74247.exe) (for Windows 7 and newer), removed with version [4.8.x](https://github.com/flaviogoncalves/AsteriskTraining/blob/master/X-Lite_5.8.3_102651.exe) (Mar. 2015)
* [RTX8663](https://github.com/traud/asterisk-broadvoice/issues/1) introduced in [05.10](https://service.snom.com/x/VwIJAQ) (Sep. 2020)
* SignalWire [FreeSWITCH](https://github.com/signalwire/freeswitch/blob/8aa6a8a9049287d165a2650d2e337340d09514ea/src/mod/codecs/mod_bv/mod_bv.c)

Baresip and X-lite were tested.

## Library

The BV32 library is still [downloadable](https://docs.broadcom.com/docs/12358448) at Broadcom. In the past, it had its own Webpage:
* [version 1.0](https://web.archive.org/web/20091114062037/http://www.broadcom.com:80/support/broadvoice/downloads.php)
* [version 1.1](https://web.archive.org/web/20120929090646/http://www.broadcom.com:80/support/broadvoice/downloads.php)
* [version 1.2](https://web.archive.org/web/20150416080759/http://www.broadcom.com:80/support/broadvoice/downloads.php)

Till Jul. 2017, FreeSWITCH [maintained](https://github.com/signalwire/freeswitch/tree/03cc850c686700c2ae636317801901120f6bbed9/libs/broadvoice) its library variant in its source tree. That was based on version 1.0 but havily changed to build a proper shared library in Debian/Ubuntu. For example, all external symbols were renamed. The original version of that was available on [Soft-Switch.org](https://www.soft-switch.org/downloads/voipcodecs/snapshots/). Yet, another variant of version 1.0 can be found on [FreeSWITCH](https://files.freeswitch.org/downloads/libs/libbv32-0.1.tar.gz), which added just a Linux build system. SUSE created a library in [Oct. 2021…](https://build.opensuse.org/package/show/openSUSE%3AFactory/broadvoice32)

## Thanks go to
* [Steve Underwood](https://www.coppice.org/) for converting the BroadVoice library 1.0 into a shared library with an easy-to-use API.
* Asterisk team: Thanks to their efforts and architecture, this BV32 module was written in one working day.
