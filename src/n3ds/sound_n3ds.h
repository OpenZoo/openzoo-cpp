#ifndef __N3DS_SOUND_H__
#define __N3DS_SOUND_H__

#include <cstdint>
#include <3ds.h>

namespace ZZT {
    typedef void (*NDSPStreamWrappedProc)(int16_t *samples, int len, void *userdata);

	class NDSPStreamWrapper {
	private:
		friend void ndsp_audio_callback(void *userdata);

		int s_bufsize, s_ndsp_channel, s_channels;
		NDSPStreamWrappedProc s_proc;
		void *s_userdata;
		LightLock s_lock;
		ndspWaveBuf s_buffer[2];
		int16_t *s_data;
		uint8_t s_fill_block;
	
	public:
		NDSPStreamWrapper(NDSPStreamWrappedProc proc, void *userdata, int ndspChn, int channels, int frequency, int bufferSize);
		~NDSPStreamWrapper();

		void lock();
		void unlock();
	};
}

#endif