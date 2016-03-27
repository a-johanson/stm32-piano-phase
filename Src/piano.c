
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

#include "piano.h"

void PianoString_lowpass(float * buffer, uint16_t length, uint16_t runs) {
	uint16_t j, i;
	for(j=0; j<runs; ++j) {
		for(i=1; i<length; ++i) {
			buffer[i] = 0.5f*(buffer[i] + buffer[i-1]);
		}
		buffer[0] = 0.5f*(buffer[0] + buffer[length-1]);
	}
}

void PianoString_zeroAverage(float * buffer, uint16_t length) {
	float avg = 0.0f;
	uint16_t i;
	for(i=0; i<length; ++i) {
		avg += buffer[i];
	}
	avg /= (float)length;
	for(i=0; i<length; ++i) {
		buffer[i] -= avg;
	}
}

inline float PianoString_RNG_rand() {
	return 2.0f*((float)rand()/(float)(RAND_MAX)) - 1.0f;
}

inline void PianoString_pluck(PianoString * s) {
	s->loopGain = s->initialGain;
	uint16_t i;
	const uint16_t length = s->length;
	for(i=0; i<length; ++i) {
		s->buffer[i] = 0.6f*s->buffer[i] + s->initialSampleLP[i];
	}
}

inline void PianoString_mute(PianoString * s) {
	s->loopGain = 0.955f + s->initialGain - 0.995f;
}

void PianoString_setup(PianoString * s, float halfToneSteps, float sampleFreq) {
    const float refFreq = 441.0f;
	const float stringFreq = refFreq * powf(2.0f, halfToneSteps/12.0f);
	const uint16_t nSamples = (uint16_t)(sampleFreq / stringFreq);

	s->buffer = (float *)malloc(nSamples * sizeof(float));
	s->initialSampleLP = (float *)malloc(nSamples * sizeof(float));
	s->initialGain = 0.995f + (stringFreq * 0.000005f);
	if(s->initialGain >= 1.0f) {
        s->initialGain = 0.99999f;
	}
	s->loopGain = 0.0f;
	s->length = nSamples;
	s->position = 0;

	uint16_t i;
	for(i=0; i<nSamples; ++i) {
		s->buffer[i] = 0.0f;
		s->initialSampleLP[i] = PianoString_RNG_rand();
	}
	PianoString_lowpass(s->initialSampleLP, nSamples, 3);
	PianoString_zeroAverage(s->initialSampleLP, nSamples);
}

inline float PianoString_sample(PianoString * s) {
	const uint16_t oldPosition = s->position;
	const uint16_t newPosition = oldPosition < s->length-1 ? oldPosition+1 : 0;
	s->position = newPosition;
	const float a = s->buffer[oldPosition];
	const float b = s->buffer[newPosition];
	const float newValue = s->loopGain * (0.95f*a + 0.05f*b);
	s->buffer[oldPosition] = newValue;

	return a;
}



void PianoNote_setup(PianoNote * note, float afterNBeats, PianoString * string) {
	note->afterNBeats = afterNBeats;
	note->string = string;
}



void PianoVoice_setup(PianoVoice * voice, float sampleFreq, float beatsPerMinute, float nBeats, PianoNote * notes, uint16_t nNotes, PianoString * strings, uint16_t nStrings) {
	voice->notes = notes;
	voice->nNotes = nNotes;
	voice->strings = strings;
	voice->nStrings = nStrings;

	voice->iSample = 0;
	voice->samplePeriod = 1.0f / sampleFreq;
	voice->beatsPerSecond = beatsPerMinute/60.0f;

	const float secondsPerBeat = 60.0f/beatsPerMinute;
	voice->nSamples = (uint32_t)(nBeats * secondsPerBeat * sampleFreq);

	voice->nextNote = 0;
}

float PianoVoice_sample(PianoVoice * voice) {
	if(voice->nextNote < voice->nNotes) {
		const float t = ((float)voice->iSample) * voice->samplePeriod;
		const float iBeat = t * voice->beatsPerSecond;
		PianoNote * nextNote = voice->notes + voice->nextNote;
		if(iBeat >= nextNote->afterNBeats) {
			const uint16_t prevNoteIndex = (voice->nextNote == 0 ? voice->nNotes - 1 : voice->nextNote - 1);
			PianoString_mute(voice->notes[prevNoteIndex].string);
			PianoString_pluck(nextNote->string);
			voice->nextNote += 1;
		}
	}

	float sample = 0.0f;
	uint16_t i;
	const uint16_t nStrings = voice->nStrings;
	for(i=0; i<nStrings; ++i) {
		sample += PianoString_sample(voice->strings+i);
	}

	voice->iSample += 1;
	if(voice->iSample >= voice->nSamples) {
		voice->iSample = 0;
		voice->nextNote = 0;
	}

	return sample;
}

