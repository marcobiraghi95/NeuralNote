//
// Created by Damien Ronssin on 04.03.23.
//

#ifndef AUDIO2MIDIPLUGIN_CONSTANTS_H
#define AUDIO2MIDIPLUGIN_CONSTANTS_H

static constexpr int NUM_HARMONICS = 8;
static constexpr int NUM_FREQ_IN = 264;
static constexpr int NUM_FREQ_OUT = 88;

static constexpr int MIDI_OFFSET = 21;
static constexpr int FFT_HOP = 256;
static constexpr int AUDIO_SAMPLE_RATE = 22050;
static constexpr int MAX_FREQ_IDX = 87;
// duration in seconds of training examples - original 1
static constexpr int AUDIO_WINDOW_LENGTH = 2;

#endif //AUDIO2MIDIPLUGIN_CONSTANTS_H
