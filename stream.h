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
#ifndef BUFPROC_H_
#define BUFPROC_H_

#include <alsa/asoundlib.h>
#include <pthread.h>

#define STREAM_RUN 1
#define STREAM_TERM 1

#define BATCH_EMPTY 0
#define BATCH_READY 1
#define BATCH_PROCESSED 2

#define BUF_SIZE 7

#define FRPP 64

typedef struct stream_desc_s stream_desc_t;

struct stream_desc_s {
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
};

stream_desc_t *initStreamDesc();
snd_pcm_t *initStream(const char *, snd_pcm_stream_t, unsigned int, snd_pcm_uframes_t *, snd_pcm_format_t, short, int *);
void draincloseStream(snd_pcm_t *);
#endif
