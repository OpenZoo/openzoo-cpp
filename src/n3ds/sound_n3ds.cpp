#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "sound_n3ds.h"

using namespace ZZT;

void ZZT::ndsp_audio_callback(void *userdata) {
	NDSPStreamWrapper *wrapper = (NDSPStreamWrapper*) userdata;

	if (wrapper->s_buffer[wrapper->s_fill_block].status == NDSP_WBUF_DONE) {
		int16_t *samples = (int16_t*) wrapper->s_buffer[wrapper->s_fill_block].data_vaddr;
		wrapper->lock();
		wrapper->s_proc(samples, wrapper->s_bufsize, wrapper->s_userdata);
		wrapper->unlock();
		DSP_FlushDataCache(samples, wrapper->s_bufsize * 2 * wrapper->s_channels);
		ndspChnWaveBufAdd(wrapper->s_ndsp_channel, &(wrapper->s_buffer[wrapper->s_fill_block]));
		wrapper->s_fill_block = 1 - wrapper->s_fill_block;
	}
}

NDSPStreamWrapper::NDSPStreamWrapper(NDSPStreamWrappedProc proc, void *userdata, int ndspChn, int channels, int frequency, int bufferSize) {
	float soundMix[12];
	memset(soundMix, 0, sizeof(soundMix));
	soundMix[0] = soundMix[1] = 1.0f;

	this->s_proc = proc;
	this->s_userdata = userdata;
	this->s_bufsize = bufferSize;
	this->s_ndsp_channel = ndspChn;
	this->s_channels = channels;
	this->s_fill_block = 1;

	this->s_data = (int16_t*) linearAlloc(bufferSize * 2 * 2 * channels);
	memset(this->s_data, 0, bufferSize * 2 * 2 * channels);

	LightLock_Init(&this->s_lock);

	ndspChnReset(ndspChn);
	ndspChnSetInterp(ndspChn, NDSP_INTERP_LINEAR);
	ndspChnSetRate(ndspChn, frequency);
	ndspChnSetFormat(ndspChn, NDSP_CHANNELS(this->s_channels) | NDSP_ENCODING(NDSP_ENCODING_PCM16));
	ndspChnSetMix(ndspChn, soundMix);

	ndspSetCallback(ndsp_audio_callback, (void*) this);

	memset(this->s_buffer, 0, sizeof(this->s_buffer));
	this->s_buffer[0].data_vaddr = this->s_data;
	this->s_buffer[0].nsamples = bufferSize;
	this->s_buffer[1].data_vaddr = this->s_data + (bufferSize * channels);
	this->s_buffer[1].nsamples = bufferSize;

	DSP_FlushDataCache(this->s_data, bufferSize * 2 * 2 * channels);

	ndspChnWaveBufAdd(ndspChn, &(this->s_buffer[0]));
	ndspChnWaveBufAdd(ndspChn, &(this->s_buffer[1]));
}

NDSPStreamWrapper::~NDSPStreamWrapper() {
	ndspChnReset(this->s_ndsp_channel);

	linearFree(this->s_data);
}

void NDSPStreamWrapper::lock() {
	LightLock_Lock(&this->s_lock);
}

void NDSPStreamWrapper::unlock() {
	LightLock_Unlock(&this->s_lock);
}
