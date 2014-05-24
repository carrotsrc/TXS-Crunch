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

static void *processOutStream(void *);
static void *processInStream(void *);

static void *processOutStream(void *sdesc)
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
				stream_d->nbuf = 0;
				free(stream_d->buffer);
				stream_d->batch = BATCH_PROCESSED;
			}
			pthread_mutex_unlock(stream_d->mutex);
		}
	}
	printf("Closing file pointer\n");
	fclose(fp);
	stream_d->batch = BATCH_PROCESSED;
	pthread_mutex_unlock(stream_d->mutex);
	printf("Avg period: %d\n", avg);
}

static void *processInStream(void *sdesc)
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
	short *lbuffer = malloc(pbytes<<BUF_SIZE);
	int pshorts = *stream_d->frames<<3;

	gettimeofday(&stime, NULL);

	size_t test;

	while(stream_d->sig > STREAM_TERM) {
		if(bindex == 1<<BUF_SIZE) {
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
				lbuffer = malloc(pbytes<<BUF_SIZE);

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

int startBufferThreads(stream_desc_t *stream_d)
{
	stream_d->mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(stream_d->mutex, NULL);
	
	pthread_t inThread, outThread;
	if(pthread_create(&inThread, NULL, &processInStream, (void*)stream_d) != 0) {
		return -1;
	}

	if(pthread_create(&outThread, NULL, &processOutStream, (void*)stream_d) != 0) {
		return -1;
	}

	return 0;
}
