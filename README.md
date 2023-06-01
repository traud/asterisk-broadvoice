# BroadVoice for Asterisk

This is an implementation of the audio codecs
* BroadVoice16 (BV16) and
* BroadVoice32 (BV32).

[RFC 4298](https://tools.ietf.org/html/rfc4298) describes their behaviour in RTP. ‘16’ is not about the sample rate but bit rate. Therefore, BV16 is a narrowband (NB) codec. BV32 is the wideband codec (WB), a codec for HD Voice. A research paper comparing other audio codecs was published at [InterSpeech 2010](https://www.isca-speech.org/archive/interspeech_2010/ramo10_interspeech.html). BroadVoice is one of the few audio codecs allowing speech packets not only at 20 milliseconds, 10 milliseconds but also 5 milliseconds. Although this module supports this, Asterisk itself is not able to leverage this, see the Jira issue [ASTERISK-25483](https://issues.asterisk.org/jira/browse/ASTERISK-25483) for details. Nowadays, several vendors of DECT headsets and DECT phones leverage BV32 as alternative to G.722. The latter requires two DECT slots, which halves the number of concurrent calls in one DECT cell.

To add a codec for SIP/SDP (m= and rtmap), you create a format module in Asterisk: `bv32_pass-through.patch`. However, both call legs must support BroadVoice (pass-through only). If one leg does not support BroadVoice, the call has no audio. Or, if you use the pre-recorded voice and music files of Asterisk, these files cannot be heard because they are not in BroadVoice but in slin. Therefore, this repository adds not just a format module for the audio-codec BroadVoice but also a transcoding module: `bv32_transcoding.patch` and `codecs/codec_bv32.c`.

## Installing the patch

At least Asterisk 13.7 is required. These changes were last tested with Asterisk 13.38.3 (and Asterisk 20.3.0). If you use a newer version and the patch fails, please, [report](https://help.github.com/articles/creating-an-issue/)!

	cd /usr/src/
	wget downloads.asterisk.org/pub/telephony/asterisk/old-releases/asterisk-13.38.3.tar.gz
	tar zxf ./asterisk*
	cd ./asterisk*/
	sudo apt install autoconf automake build-essential pkg-config libedit-dev libjansson-dev libsqlite3-dev uuid-dev libxslt1-dev xmlstarlet

Apply the patch:

	wget github.com/traud/asterisk-broadvoice/archive/Library-1.2.tar.gz
	tar zxf Library-1.*.tar.gz
	rm Library-1.*.tar.gz
	cp --verbose --recursive ./asterisk-broadvoice*/* ./
	patch -p0 <./bv32_pass-through.patch
	patch -p0 <./bv16_pass-through.patch

If you still use Asterisk 13, go for `bv32_pass-through_13.patch` instead.

Install libraries …

If you do not want transcoding but pass-through only, please, skip this step. To support transcoding, you’ll need to download the latest [BV32 library](https://docs.broadcom.com/docs/12358448) and [BV16 library](https://docs.broadcom.com/docs/12358447). Then, you have to build and install, for example, in Debian/Ubuntu:

… for BroadVoice32:

	cd ~/Downloads/
	unzip -qq ./BroadVoice32OpenSource.v1.2.zip
	cd ./BroadVoice32/FloatingPoint/
	patch -p0 <~/Downloads/bv32_library.patch
	sudo mkdir /usr/local/include/bvcommon
	sudo cp -R ./bvcommon/*.h /usr/local/include/bvcommon/
	sudo mkdir /usr/local/include/bv32
	sudo cp -R ./bv32/*.h /usr/local/include/bv32/
	cd ./Linux/
	patch -p0 <~/Downloads/bv32_makefile.patch
	make
	sudo cp ./libbv32.so /usr/local/lib/
	cd /usr/src/asterisk*
	patch -p0 <./bv32_transcoding.patch

… BroadVoice16:

	cd ~/Downloads/
	unzip -qq ./BroadVoice16OpenSource.v1.2.zip
	cd ./BroadVoice16/FloatingPoint/
	patch -p0 <~/Downloads/bv16_library.patch
	sudo mkdir /usr/local/include/bv16-floatingpoint/bvcommon
	sudo cp -R ./bvcommon/*.h /usr/local/include/bv16-floatingpoint/bvcommon/
	sudo mkdir /usr/local/include/bv16-floatingpoint/bv16
	sudo cp -R ./bv16/*.h /usr/local/include/bv16-floatingpoint/bv16/
	cd ./Linux/
	patch -p0 <~/Downloads/bv16_makefile.patch
	make
	sudo cp ./libbv16.so /usr/local/lib/
	cd /usr/src/asterisk*
	patch -p0 <./bv16_transcoding.patch
	
The system headers for BroadVoice16 are installed at a different location to follow Linphone.

Re-load the system libraries:

	sudo ldconfig

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

Several VoIP/SIP implementations use BV16, for example:
* [Linphone](https://github.com/BelledonneCommunications/mediastreamer2/blob/2ef88ff7f9b3506b7c10e6925c36ccba2f151f86/src/audiofilters/bv16.c)
* [Sipdroid](https://github.com/i-p-tel/sipdroid/blob/ef15c6d68cf402088c2960802d2dec87be15f2a9/app/src/main/jni/bv16_jni.cpp)

Several VoIP/SIP implementations use BV32, for example:
* Adran IP 706/712
* [Baresip](https://github.com/traud/asterisk-broadvoice/wiki), removed with version 0.6.5
* CounterPath X-lite [4.5.5](https://web.archive.org/web/20151120063639/http://counterpath.s3.amazonaws.com/downloads/X-Lite_Win32_4.5.5.2_76432.exe) (for Windows XP)
* CounterPath X-lite [4.7.1](https://web.archive.org/web/20160306091234/http://counterpath.s3.amazonaws.com/downloads/X-Lite_Win32_4.7.1_74247.exe) (for Windows 7 and newer), removed with version [4.8.x](https://github.com/flaviogoncalves/AsteriskTraining/blob/master/X-Lite_5.8.3_102651.exe) (Mar. 2015)
* [RTX8663](https://github.com/traud/asterisk-broadvoice/issues/1) introduced in [05.10](https://service.snom.com/x/VwIJAQ) (Sep. 2020)

Some implementations use both:
* Aastra 6700i and 6800i Series, removed with firmware 4.1.x (Jun. 2015; DEF39955)
* SignalWire [FreeSWITCH](https://github.com/signalwire/freeswitch/blob/8aa6a8a9049287d165a2650d2e337340d09514ea/src/mod/codecs/mod_bv/mod_bv.c)

Baresip, Linphone, and X-lite were tested.

## Library

Till Apr. 2015, the BroadVoice library had its own webpage:
* [version 1.0](https://web.archive.org/web/20091114062037/http://www.broadcom.com:80/support/broadvoice/downloads.php)
* [version 1.1](https://web.archive.org/web/20120929090646/http://www.broadcom.com:80/support/broadvoice/downloads.php)
* [version 1.2](https://web.archive.org/web/20150416080759/http://www.broadcom.com:80/support/broadvoice/downloads.php)

Till Jul. 2017, FreeSWITCH [maintained](https://github.com/signalwire/freeswitch/tree/03cc850c686700c2ae636317801901120f6bbed9/libs/broadvoice) its library variant in its source tree. That was based on version 1.0 but havily changed to build a proper shared library in Debian/Ubuntu. For example, all external symbols were renamed. The original version of that was available on [Soft-Switch.org](https://www.soft-switch.org/downloads/voipcodecs/snapshots/). Yet, another variant of version 1.0 can be found on [FreeSWITCH](https://files.freeswitch.org/downloads/libs/libbv32-0.1.tar.gz), which added just a Linux build system. SUSE created a library in [Oct. 2021…](https://build.opensuse.org/package/show/openSUSE%3AFactory/broadvoice32)

## Thanks go to
* [Steve Underwood](https://www.coppice.org/) for converting the BroadVoice library 1.0 into a shared library with an easy-to-use API.
* Asterisk team: Thanks to their efforts and architecture, this module was written in one working day.
