/** @file paex_sine.c
 @ingroup examples_src
 @brief Play a sine wave for several seconds.
 @author Ross Bencina <rossb@audiomulch.com>
 @author Phil Burk <philburk@softsynth.com>
 */
/*
 * $Id$
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com/
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however,
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also
 * requested that these non-binding requests be included along with the
 * license above.
 */
#include <stdio.h>
#include <math.h>
#include <portaudio.h>
#include <iostream>

#include "./Vst3PluginFactory.hpp"
#include "./Vst3HostCallback.hpp"
#include "./Vst3Plugin.hpp"
#include "./Buffer.hpp"
#include "./StrCnv.hpp"
#include <pluginterfaces/vst/ivstaudioprocessor.h>

#define NUM_SECONDS   (4)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (200)
typedef struct
{
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
    char message[20];
}
paTestData;

hwm::Vst3Plugin *g_plugin;
std::vector<int> const g_notes = { 48, 50, 52, 53 };
int g_last_note_index = -1;
int g_current_pos = 0;

/* This routine will be called by the PortAudio engine when audio is needed.
 ** It may called at interrupt level on some machines so don't do anything
 ** that could mess up the system like calling malloc() or free().
 */
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData )
{
    assert(g_plugin);
    
    int note_index = ((int)timeInfo->currentTime) % g_notes.size();
    if(note_index != g_last_note_index) {
        if(g_last_note_index >= 0) { g_plugin->AddNoteOff(g_notes[note_index]); }
        g_plugin->AddNoteOn(g_notes[note_index]);
    }
    g_last_note_index = note_index;
    
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    
    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    auto const result = g_plugin->ProcessAudio(g_current_pos, framesPerBuffer);
    for(int i = 0; i<framesPerBuffer; i++ ) {
        *out++ = result[0][i];
        *out++ = result[1][i];
    }
    g_current_pos += framesPerBuffer;
    
    return paContinue;
}

/*
 * This routine is called by portaudio when playback is done.
 */
static void StreamFinished( void* userData )
{
    paTestData *data = (paTestData *) userData;
    printf( "Stream Completed: %s\n", data->message );
}

/*******************************************************************/
int main(void)
{
    hwm::Vst3HostCallback host_context;
    
    String path = L"/Library/Audio/Plug-Ins/VST3/Zebra2.vst3/Contents/MacOS/Zebra2";
    hwm::Vst3PluginFactory factory(path);
    
    auto const cmp_count = factory.GetComponentCount();
    std::cout << "Component Count : " << cmp_count << std::endl;
    std::vector<int> effect_indices;
    for(int i = 0; i < cmp_count; ++i) {
        auto const &info = factory.GetComponentInfo(i);
        std::cout << hwm::to_utf8(info.name()) << ", " << hwm::to_utf8(info.category()) << std::endl;
        
        //! カテゴリがkVstAudioEffectClassなComponentを探索する。
        if(info.category() == hwm::to_wstr(kVstAudioEffectClass)) {
            effect_indices.push_back(i);
        }
    }
    
    if(effect_indices.empty()) {
        std::cout << "No AudioEffects found." << std::endl;
        return 0;
    }
    
    std::unique_ptr<hwm::Vst3Plugin> plugin;
    host_context.SetRequestToRestartHandler([&plugin](Steinberg::int32 flags) {
        if(plugin) {
            plugin->RestartComponent(flags);
        }
    });
    
    plugin = factory.CreateByIndex(effect_indices[0], host_context.GetUnknownPtr());


    
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;
    int i;
    
    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);
    
    /* initialise sinusoidal wavetable */
    for( i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    data.left_phase = data.right_phase = 0;
    
    err = Pa_Initialize();
    if( err != paNoError ) goto error;
    
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr,"Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    
    plugin->SetBlockSize(FRAMES_PER_BUFFER);
    plugin->SetSamplingRate(SAMPLE_RATE);
    plugin->Resume();
    g_plugin = plugin.get();
    
    err = Pa_OpenStream(
                        &stream,
                        NULL, /* no input */
                        &outputParameters,
                        SAMPLE_RATE,
                        FRAMES_PER_BUFFER,
                        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                        patestCallback,
                        &data );
    if( err != paNoError ) goto error;
    
    sprintf( data.message, "No Message" );
    err = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
    if( err != paNoError ) goto error;
    
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    
    printf("Play for %d seconds.\n", NUM_SECONDS );
    Pa_Sleep( NUM_SECONDS * 1000 );
    
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    
    Pa_Terminate();
    printf("Test finished.\n");
    
    plugin->Suspend();
    plugin.reset();
    
    return err;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}
