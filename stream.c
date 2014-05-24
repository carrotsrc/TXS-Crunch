/*
* Copyright 2014, carrotsrc.org
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "stream.h"

#define RING_SIZE 64

void printState(snd_pcm_state_t state)
{
	switch(state) {
	case SND_PCM_STATE_OPEN:
		printf("SND_PCM_STATE_OPEN\n");
	break;
	case SND_PCM_STATE_SETUP:
		printf("SND_PCM_STATE_SETUP\n");
	break;
	case SND_PCM_STATE_PREPARED:
		printf("SND_PCM_STATE_PREPARED\n");
	break;
	case SND_PCM_STATE_RUNNING:
		printf("SND_PCM_STATE_RUNNING\n");
	break;
	case SND_PCM_STATE_XRUN:
		printf("SND_PCM_STATE_XRUN\n");
	break;
	case SND_PCM_STATE_DRAINING:
		printf("SND_PCM_STATE_DRAINING\n");
	break;
	case SND_PCM_STATE_PAUSED:
		printf("SND_PCM_STATE_PAUSED\n");
	break;
	case SND_PCM_STATE_SUSPENDED:
		printf("SND_PCM_STATE_SUSPENDED\n");
	break;
	case SND_PCM_STATE_DISCONNECTED:
		printf("SND_PCM_STATE_DISCONNECTED\n");
	break;
	}
}

snd_pcm_t *initStream(const char *device, snd_pcm_stream_t stream, unsigned int sample,
		snd_pcm_uframes_t *frames, snd_pcm_format_t format, short channels, int *usp)
{
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	int dir;
	int r = 0;

	r = snd_pcm_open(&handle, device, stream, 0);

	if(r < 0) {
		fprintf(stderr, "Failed to open stream\n");
		snd_strerror(r);
		return NULL;
	}

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(handle, params);

	r = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(r < 0) {
		fprintf(stderr, "Failed to set access for hw params\n");
		snd_strerror(r);
		return NULL;
	}

	r = snd_pcm_hw_params_set_format(handle, params, format);
	if(r < 0) {
		fprintf(stderr, "Failed to set stream format\n");
		snd_strerror(r);
		return NULL;
	}

	r = snd_pcm_hw_params_set_channels(handle, params, channels);

	if(r < 0) {
		fprintf(stderr, "Failed to set number of channels of stream\n");
		snd_strerror(r);
		return NULL;
	}

	r = snd_pcm_hw_params_set_rate_near(handle, params, &sample, &dir);

	if(r < 0) {
		fprintf(stderr, "Failed to set sampling rate of stream\n");
		snd_strerror(r);
		return NULL;
	}
	r = snd_pcm_hw_params_set_period_size_near(handle, params, frames, &dir);

	if(r < 0) {
		fprintf(stderr, "Failed to set period size\n");
		return NULL;

	}

	r = snd_pcm_hw_params(handle, params);

	if(r < 0) {
		fprintf(stderr, "Failed to apply parameters\n");
		snd_strerror(r);
		return NULL;

	}

	if(stream == SND_PCM_STREAM_CAPTURE) {
		r = snd_pcm_hw_params_get_period_size(params, frames, &dir);
		if(r <  0) {
			fprintf(stderr, "Failed to define period size");
			snd_strerror(r);
			return NULL;
		}
		int val = *frames;
		snd_pcm_hw_params_get_period_time(params, &val, &dir);
		*usp = val;
	}

	//snd_pcm_hw_params_free(params);

	return handle;
}

void *initSoftwareParams(snd_pcm_t *stream, snd_pcm_uframes_t frames)
{
	snd_pcm_sw_params_t *params;
	snd_pcm_sw_params_alloca(&params); 

	if(snd_pcm_sw_params_current(stream, params) < 0)
		fprintf(stderr, "Failed to initialize software parameters\n");

	snd_pcm_sw_params_set_avail_min(stream, params, frames);
	snd_pcm_sw_params_set_start_threshold(stream, params, 0U);
	snd_pcm_sw_params(stream, params);
}

void draincloseStream(snd_pcm_t *handle)
{
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
}


stream_desc_t *initStreamDesc(pthread_mutex_t *mutex)
{
	stream_desc_t *s = malloc(sizeof(stream_desc_t));
	s->buffer = NULL;
	s->nbuf = 0;
	s->mutex = mutex;
	s->sig = STREAM_RUN;
	s->frames = malloc(sizeof(snd_pcm_uframes_t));
	s->dir = 1;
	s->wp = 0;
	s->batch = BATCH_PROCESSED;
	return s;
}

