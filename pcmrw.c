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
#include <alsa/asoundlib.h>
#include <pthread.h>


#define RING_SIZE 64
#define BUF_SIZE 64
#define FRPP 64

#define STREAM_TERM 0
#define STREAM_RUN 1

#define BATCH_EMPTY 0
#define BATCH_READY 1
#define BATCH_PROCESSED 2


typedef struct stream_desc_s {
	snd_pcm_t *in;
	snd_pcm_t *out;
	pthread_mutex_t *mutex;
	short *buffer;
	short nbuf;
	short sig;
	snd_pcm_uframes_t *frames;
	int sample;
	short dir;
	int usp;
	long wp;
	int batch;
} stream_desc_t;

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

snd_pcm_t *initStream(const char *device, snd_pcm_stream_t stream, unsigned int sample, snd_pcm_uframes_t *frames, snd_pcm_format_t format, short channels, int *usp)
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

void *processOutStream(void *sdesc)
{
	FILE *fp = fopen("streamed.raw", "wb");
	stream_desc_t *stream_d = (stream_desc_t*)sdesc;
	int pshorts = *stream_d->frames*2*sizeof(short);
	int bufs;
	size_t test;
	int periods = 0;
	int avg = 0;
	int total = 0;
	while(stream_d->sig > STREAM_TERM) {
		if(pthread_mutex_lock(stream_d->mutex) == 0) {
			if(stream_d->nbuf > 0) {
				periods += stream_d->nbuf;
				total++;
				avg = periods/total;

				test = fwrite(stream_d->buffer, 1, pshorts*stream_d->nbuf, fp);
				//fprintf(stdout, "@ %lu -- %d / %d\n", stream_d->wp, (int)test, stream_d->nbuf);
				stream_d->nbuf = 0;
				free(stream_d->buffer);
				stream_d->batch = BATCH_PROCESSED;
			}
			pthread_mutex_unlock(stream_d->mutex);
		}
	}
	printf("Closing file pointer\n");
	fclose(fp);
	printf("Avg period: %d\n", avg);
}

void *processInStream(void *sdesc)
{
	struct timeval stime, etime;

	stream_desc_t *stream_d = (stream_desc_t*)sdesc;
	snd_pcm_prepare(stream_d->in);

	int nframe;
	int pbytes = sizeof(short)*(int)*stream_d->frames*2;
	stream_d->buffer = malloc(pbytes);

	// local buffer and index
	int bindex = 0;
	int pindex = 0;
	int pnum = 0;
	short *lbuffer = malloc(pbytes*BUF_SIZE);
	int pshorts = *stream_d->frames*2*2;

	gettimeofday(&stime, NULL);

	size_t test;

	while(stream_d->sig > STREAM_TERM) {
		if(bindex == BUF_SIZE) {
			fprintf(stderr, "Buffer overflow\n");
			stream_d->sig = STREAM_TERM;
			break;
		}

		// added period to local buffer
		nframe = snd_pcm_readi(stream_d->in, lbuffer+(bindex*(*stream_d->frames)*2), *stream_d->frames);
		bindex++;
		stream_d->wp++;
		if(pthread_mutex_trylock(stream_d->mutex) == 0) {
			// thread has locked mutex so nothing is being processed
			// by other thread
			// copy everything into shared buffer
			if(nframe < 0) {
			      fprintf(stderr,
				      "error from read: %s\n",
				      snd_strerror(nframe));
				if(nframe == -EPIPE) {
					fprintf(stderr, "Overrun occured on capture\n");
					snd_pcm_prepare(stream_d->in);
				}
				else
					fprintf(stderr, "Unknown error on capture buffer\n");

				// reset the last buffer
				bindex--;
				stream_d->wp--;
				pthread_mutex_unlock(stream_d->mutex);
				continue;
			}

			if(stream_d->batch == BATCH_PROCESSED) {
				// last batch has been processed
				stream_d->buffer = lbuffer;
				stream_d->nbuf = bindex;

				if(pindex < bindex) {
					pindex = bindex;
					pnum++;
				}

				bindex = 0;
				lbuffer = malloc(pbytes*BUF_SIZE);

				// new batch ready
				stream_d->batch = BATCH_READY;
			}
			pthread_mutex_unlock(stream_d->mutex);
		}
	}
	gettimeofday(&etime, NULL);

	double period = ((((double)etime.tv_sec*1000000) + (double)etime.tv_usec) - (((double)stime.tv_sec*1000000) + (double)stime.tv_usec));
	printf("Closing input stream\n");
	printf("Period: %F seconds\n", period/1000000);
	printf("Peak buffer size: %d period(s)\n", pindex);
	printf("Peak push: %d\n", pnum);

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

int main(int argc, char *argv[])
{
	snd_pcm_uframes_t frames = FRPP;
	unsigned int sample = 44100;
	pthread_t inThread, outThread;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	stream_desc_t *stream_d = initStreamDesc(&mutex);
	stream_d->sample = sample;
	stream_d->frames = &frames;

	/*if((stream_d->out = initStream("default", SND_PCM_STREAM_PLAYBACK, sample, stream_d->frames, SND_PCM_FORMAT_S16_LE, 2, NULL)) == NULL) {
		fprintf(stderr, "Failed to initialize output PCM\n");
		exit(1);
	}*/

	if((stream_d->in = initStream("default", SND_PCM_STREAM_CAPTURE, sample, stream_d->frames, SND_PCM_FORMAT_S16_LE, 2, &stream_d->usp)) == NULL) {
		fprintf(stderr, "Failed to initialize input PCM\n");
		exit(1);
	}

	printf("PCMrw\n---\n");
	printf("Time of period: %dus\n", stream_d->usp);
	long pbytes = sizeof(short)*(int)*stream_d->frames*2;
	printf("Period Size: %lu bytes\n", pbytes);
	pbytes = sizeof(short)*(int)*stream_d->frames*2 * (1000000/stream_d->usp);
	printf("Transfer rate: %lu Kbps\n", (pbytes/1000)*8);

	if(pthread_create(&inThread, NULL, processInStream, (void*)stream_d) != 0)
		printf("Failed to generate thread\n");

	if(pthread_create(&outThread, NULL, processOutStream, (void*)stream_d) != 0)
		printf("Failed to generate thread\n");

	char buf[10];
	while(stream_d->sig == STREAM_RUN) {
		fgets(buf, 10, stdin);
		if(strcmp(buf, "q\n") == 0)
			stream_d->sig = STREAM_TERM;
	}
	draincloseStream(stream_d->in);
	//draincloseStream(stream_d->out);
}
