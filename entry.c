#include "pcmr.h"

int main(int argc, char *argv[])
{
	snd_pcm_uframes_t frames = FRPP;
	unsigned int sample = 44100;
	pthread_t inThread, outThread;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	stream_desc_t *stream_d = initStreamDesc(&mutex);
	stream_d->sample = sample;
	stream_d->frames = &frames;

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
}
