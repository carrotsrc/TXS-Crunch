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
#include "bufproc.h"

int main(int argc, char *argv[])
{
	snd_pcm_uframes_t frames = FRPP;
	unsigned int sample = 44100;

	stream_desc_t *stream_d = initStreamDesc(&mutex);
	stream_d->sample = sample;
	stream_d->frames = &frames;

	if((stream_d->in = initStream("default", SND_PCM_STREAM_CAPTURE, sample, stream_d->frames, SND_PCM_FORMAT_S16_LE, 2, &stream_d->usp)) == NULL) {
		fprintf(stderr, "Failed to initialize input PCM\n");
		exit(1);
	}

	printf("Crunch Server 0.2\n-----------------\n");
	printf("Time of period: %dus\n", stream_d->usp);
	long pbytes = sizeof(short)*(int)*stream_d->frames*2;
	printf("Period Size: %lu bytes\n", pbytes);
	pbytes = sizeof(short)*(int)*stream_d->frames*2 * (1000000/stream_d->usp);
	printf("Transfer rate: %lu Kbps\n", (pbytes/1000)*8);

	
	startBufferThreads((void*) stream_d);
	char buf[10];
	while(stream_d->sig == STREAM_RUN) {
		fgets(buf, 10, stdin);
		if(strcmp(buf, "q\n") == 0)
			stream_d->sig = STREAM_TERM;
	}
	draincloseStream(stream_d->in);
}
