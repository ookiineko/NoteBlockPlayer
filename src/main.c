#include <stdio.h>      // for fprintf
#include <stdlib.h>     // for EXIT_*
#include <string.h>     // for memset, strncmp
#include <stdbool.h>

#include <sndfile.h>
#include <portaudio.h>

#include "res.h"

#define MAX_CHANNELS                2

#ifdef __linux__
#define FIND_PULSE_DEV
#endif

#ifdef FIND_PULSE_DEV
#define DEV_PA_UNSET                -1

static const char *DEV_PA_PULSE =   "pulse";
#endif

#define POLL_INTERVAL               100

struct callback_ctx_t {
    float *data;
    int channels;
    sf_count_t cnt, i;
};

static struct callback_ctx_t ctx;

static int mCallback(
    __attribute__((unused)) const void *input,
    void *output, unsigned long frameCount,
    __attribute__((unused)) const PaStreamCallbackTimeInfo* timeInfo,
    __attribute__((unused)) PaStreamCallbackFlags statusFlags,
    void *userData ) {
    float *out;
    struct callback_ctx_t *ctx;
    sf_count_t cnt, remaining;
    bool finish;
    int i;

    out = (float *) output;
    ctx = (struct callback_ctx_t *) userData;

    cnt = frameCount * ctx->channels;
    finish = false;

    i = ctx->i;
    ctx->i += cnt;
    if ((remaining = ctx->cnt - ctx->i) < 0) {
        cnt += remaining;
        finish = true;
    } else if (remaining == 0)
        finish = true;

    memcpy(out, ctx->data + i, cnt * sizeof(float));

    return finish ? paComplete : paContinue;
}

int main() {
    union {
        PaError err;
        PaDeviceIndex num_devs;
    } pa_ctx;
    const PaDeviceInfo *dev_info;
#ifdef FIND_PULSE_DEV
    int i;
    size_t len;
#endif
    PaDeviceIndex dev_idx;
    SNDFILE *sndfile;
    SF_INFO sndinfo;
    PaStream *stream;
    PaStreamParameters param;

    pa_ctx.err = Pa_Initialize();
    if (pa_ctx.err != paNoError) {
        fprintf(stderr, "failed to initialize PA\n");
        goto pa_err;
    }

    if ((pa_ctx.num_devs = Pa_GetDeviceCount()) < 0) {
        fprintf(stderr, "failed to get device count\n");
        goto pa_err;
    }

#ifdef FIND_PULSE_DEV
    dev_idx = DEV_PA_UNSET;
    len = strlen(DEV_PA_PULSE);
    for (i = 1; i <= pa_ctx.num_devs; i++) {
        dev_info = Pa_GetDeviceInfo(i);
        if (strncmp(dev_info->name, DEV_PA_PULSE, len)) {
            dev_idx = i;
            break;
        }
    }

    if (dev_idx == DEV_PA_UNSET) {
        fprintf(stderr, "cannot find pulse output device, falling back to default\n");
#endif
        dev_idx = Pa_GetDefaultOutputDevice();
        dev_info = Pa_GetDeviceInfo(dev_idx);
#ifdef FIND_PULSE_DEV
    }
#endif

    /* must initialize SF_INFO before using */
    memset(&sndinfo, 0, sizeof(sndinfo));

    sndinfo.format = NOTES_FORMAT;

    if (!(sndfile = sf_open(HARP_PATH, SFM_READ, &sndinfo))) {
        fprintf(stderr, "sndfile open failed: %s\n", sf_strerror(NULL));
        return EXIT_FAILURE;
    }

    if (sndinfo.channels > MAX_CHANNELS) {
        fprintf(stderr, "cannot handle more than %d channel(s) (got %d)\n",
                MAX_CHANNELS, sndinfo.channels);
        sf_close(sndfile);
        return EXIT_FAILURE;
    }

    ctx.cnt = sndinfo.channels * sndinfo.frames;
    ctx.data = (float *) malloc(ctx.cnt * sizeof(float));
    sf_readf_float(sndfile, ctx.data, sndinfo.frames);
    ctx.channels = sndinfo.channels;
    ctx.i = 0;

    memset(&param, 0, sizeof(param));
    param.channelCount = sndinfo.channels;
    param.device = dev_idx;
    param.hostApiSpecificStreamInfo = NULL;  // must be set to NULL if unused
    param.sampleFormat = paFloat32;
    param.suggestedLatency = dev_info->defaultLowOutputLatency;

    sf_close(sndfile);

    if (Pa_IsFormatSupported(NULL, &param, sndinfo.samplerate) != paFormatIsSupported) {
        fprintf(stderr, "output param is unsupported\n");
        free(ctx.data);
        Pa_Terminate();
        return EXIT_FAILURE;
    }

    // must set input param to NULL if opening output-only stream
    if ((pa_ctx.err = Pa_OpenStream(&stream, NULL, &param, sndinfo.samplerate,
        paFramesPerBufferUnspecified, paNoFlag, &mCallback, &ctx)) != paNoError) {
        fprintf(stderr, "failed to open stream\n");
        goto pa_err;
    }

    if ((pa_ctx.err = Pa_StartStream(stream)) != paNoError) {
        fprintf(stderr, "failed to start streaming\n");
        Pa_CloseStream(stream);
        goto pa_err;
    }

    // wait for EOF
    while (Pa_IsStreamActive(stream))
        Pa_Sleep(POLL_INTERVAL);

    Pa_CloseStream(stream);

    free(ctx.data);
    Pa_Terminate();
    return EXIT_SUCCESS;

pa_err:
    fprintf(stderr, "PA error: %s (%d)\n",
            Pa_GetErrorText(pa_ctx.err), pa_ctx.err);
    free(ctx.data);
    Pa_Terminate();
    return EXIT_FAILURE;
}
