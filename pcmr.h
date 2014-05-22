#ifndef PCMR_H_
#define PCMR_H_
#include <alsa/asoundlib.h>
#include <pthread.h>

#define STREAM_TERM 0
#define STREAM_RUN 1

#define FRPP 64

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

stream_desc_t *initStreamDesc(pthread_mutex_t *);
snd_pcm_t *initStream(const char *, snd_pcm_stream_t, unsigned int, snd_pcm_uframes_t *, snd_pcm_format_t, short, int *);
void draincloseStream(snd_pcm_t *);
void *processOutStream(void *);
void *processInStream(void *);
#endif
