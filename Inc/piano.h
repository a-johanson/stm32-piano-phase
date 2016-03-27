
/*
 * Copyright 2016 Arne Johanson
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _H_PIANO
#define _H_PIANO

#include <math.h>
#include <stdlib.h>
#include <stdint.h>



struct s_PianoString {
	float * buffer;
	float * initialSampleLP;
	uint16_t length;
	uint16_t position;
	float initialGain;
	float loopGain;
};
typedef struct s_PianoString PianoString;

struct s_PianoNote {
	float afterNBeats;
	PianoString * string;
};
typedef struct s_PianoNote PianoNote;

struct s_PianoVoice {
	PianoNote * notes;
	uint16_t nNotes;
	PianoString * strings;
	uint16_t nStrings;

	uint32_t nSamples;
	uint32_t iSample;
	float samplePeriod;

	float beatsPerSecond;

	uint16_t nextNote;
};
typedef struct s_PianoVoice PianoVoice;


void PianoString_setup(PianoString * s, float halfToneSteps, float sampleFreq);
void PianoNote_setup(PianoNote * note, float afterNBeats, PianoString * string);
void PianoVoice_setup(PianoVoice * voice, float sampleFreq, float beatsPerMinute, float nBeats, PianoNote * notes, uint16_t nNotes, PianoString * strings, uint16_t nStrings);
float PianoVoice_sample(PianoVoice * voice);



#endif // _H_PIANO
