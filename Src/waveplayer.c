
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

#include "waveplayer.h"

// Buffer size in bytes; has to be divisible by 8!
#define AUDIO_BUFFER_SIZE 16384
// Ping-Pong buffer used for audio play
// After the first half of the buffer has been played back it is refilled; then the second half etc.
// 2 Channels, 16 bit signed each, left before right channel in each sample
uint8_t audioBuffer[AUDIO_BUFFER_SIZE];

volatile BUFFER_StateTypeDef BufferOffset = BUFFER_OFFSET_NONE;




#define N_STRINGS 7
#define N_NOTES 12

PianoString strings1[N_STRINGS];
PianoString strings2[N_STRINGS];
PianoNote notes1[N_NOTES];
PianoNote notes2[N_NOTES];
PianoVoice voice1, voice2;


void waveplayer_setupPianos() {
	// e', fis', h', cis'', d'', f',
	// -5/12, -3/12, 2/12, 4/12, 5/12, -4/12 relative to concert pitch a' (440 Hz)
	// e', c'', h', f', d'', c''
	// -5/12, 3/12, 2/12, -4/12, 5/12, 3/12
	const float e1 = -5.0f;
	const float fis1 = -3.0f;
	const float h1 = 2.0f;
	const float cis2 = 4.0f;
	const float d2 = 5.0f;
	const float f1 = -4.0f;
	const float c2 = 3.0f;
	const float sampleFreq = 44100.0f;

	PianoString_setup(&strings1[0], e1, sampleFreq);
	PianoString_setup(&strings1[1], fis1, sampleFreq);
	PianoString_setup(&strings1[2], h1, sampleFreq);
	PianoString_setup(&strings1[3], cis2, sampleFreq);
	PianoString_setup(&strings1[4], d2, sampleFreq);
	PianoString_setup(&strings1[5], f1, sampleFreq);
	PianoString_setup(&strings1[6], c2, sampleFreq);

	PianoString_setup(&strings2[0], e1, sampleFreq);
	PianoString_setup(&strings2[1], fis1, sampleFreq);
	PianoString_setup(&strings2[2], h1, sampleFreq);
	PianoString_setup(&strings2[3], cis2, sampleFreq);
	PianoString_setup(&strings2[4], d2, sampleFreq);
	PianoString_setup(&strings2[5], f1, sampleFreq);
	PianoString_setup(&strings2[6], c2, sampleFreq);

	PianoNote_setup(&notes1[ 0],  0.0f, &strings1[0]);
	PianoNote_setup(&notes1[ 1],  1.0f, &strings1[1]);
	PianoNote_setup(&notes1[ 2],  2.0f, &strings1[2]);
	PianoNote_setup(&notes1[ 3],  3.0f, &strings1[3]);
	PianoNote_setup(&notes1[ 4],  4.0f, &strings1[4]);
	PianoNote_setup(&notes1[ 5],  5.0f, &strings1[5]);
	PianoNote_setup(&notes1[ 6],  6.0f, &strings1[0]);
	PianoNote_setup(&notes1[ 7],  7.0f, &strings1[6]);
	PianoNote_setup(&notes1[ 8],  8.0f, &strings1[2]);
	PianoNote_setup(&notes1[ 9],  9.0f, &strings1[5]);
	PianoNote_setup(&notes1[10], 10.0f, &strings1[4]);
	PianoNote_setup(&notes1[11], 11.0f, &strings1[6]);

	PianoNote_setup(&notes2[ 0],  0.0f, &strings2[0]);
	PianoNote_setup(&notes2[ 1],  1.0f, &strings2[1]);
	PianoNote_setup(&notes2[ 2],  2.0f, &strings2[2]);
	PianoNote_setup(&notes2[ 3],  3.0f, &strings2[3]);
	PianoNote_setup(&notes2[ 4],  4.0f, &strings2[4]);
	PianoNote_setup(&notes2[ 5],  5.0f, &strings2[5]);
	PianoNote_setup(&notes2[ 6],  6.0f, &strings2[0]);
	PianoNote_setup(&notes2[ 7],  7.0f, &strings2[6]);
	PianoNote_setup(&notes2[ 8],  8.0f, &strings2[2]);
	PianoNote_setup(&notes2[ 9],  9.0f, &strings2[5]);
	PianoNote_setup(&notes2[10], 10.0f, &strings2[4]);
	PianoNote_setup(&notes2[11], 11.0f, &strings2[6]);

	const float beatsPerMinute = 70.0f*6.0f;
	PianoVoice_setup(&voice1, sampleFreq, beatsPerMinute, 12.0f, notes1, N_NOTES, strings1, N_STRINGS);
	PianoVoice_setup(&voice2, sampleFreq, 1.005f*beatsPerMinute, 12.0f, notes2, N_NOTES, strings2, N_STRINGS);
}


float waveplayer_samplePianos(int16_t * pcmBuffer, uint32_t nSamples, float maxObservedAmplitude) {
	HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);

	float maxAmplitude = maxObservedAmplitude;
	float maxAmplitudeInverse = 1.0f/maxAmplitude;
	uint32_t iSample;
	for(iSample=0; iSample<nSamples; ++iSample) {
		const float v1 = PianoVoice_sample(&voice1);
		const float v2 = PianoVoice_sample(&voice2);

		const float c1 = v1 + 0.9f*v2;
		const float c2 = v2 + 0.9f*v1;

		const float c1_abs = fabsf(c1);
		if(c1_abs > maxAmplitude) {
			maxAmplitude = c1_abs;
			maxAmplitudeInverse = 1.0f/maxAmplitude;
		}
		const float c2_abs = fabsf(c2);
		if(c2_abs > maxAmplitude) {
			maxAmplitude = c2_abs;
			maxAmplitudeInverse = 1.0f/maxAmplitude;
		}

		const float maxSampleVal = 32760.0f;
		pcmBuffer[2*iSample  ] = (int16_t)(maxSampleVal*maxAmplitudeInverse*c1);
		pcmBuffer[2*iSample+1] = (int16_t)(maxSampleVal*maxAmplitudeInverse*c2);
	}

	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_SET);

	return maxAmplitude;
}


void waveplayer_start() {
	waveplayer_setupPianos();

	const uint32_t nSamplesPerBuffer = AUDIO_BUFFER_SIZE / (2*2);
	const uint32_t nSamplesPerHalfBuffer = nSamplesPerBuffer / 2;

	float maxObservedAmplitude = waveplayer_samplePianos((int16_t*)audioBuffer, nSamplesPerBuffer, 1.0f);

	uint8_t volume = 50;

	if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, volume, 44100) != AUDIO_OK) {
		return;
	}

	if(BSP_AUDIO_OUT_Play((uint16_t*)audioBuffer, AUDIO_BUFFER_SIZE) != AUDIO_OK) {
		return;
	}

	GPIO_PinState userBtnPrev = GPIO_PIN_RESET;
	while(1) {
		if(BufferOffset != BUFFER_OFFSET_NONE) {
			const GPIO_PinState userBtn = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
			if(userBtn == GPIO_PIN_SET && userBtnPrev == GPIO_PIN_RESET) {
				volume += 10;
				if(volume > 100) {
					volume = 10;
				}
				BSP_AUDIO_OUT_SetVolume(volume);
			}
			userBtnPrev = userBtn;
		}

		if(BufferOffset == BUFFER_OFFSET_HALF) {
			HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_SET);
			maxObservedAmplitude = waveplayer_samplePianos((int16_t*)audioBuffer, nSamplesPerHalfBuffer, maxObservedAmplitude);
			BufferOffset = BUFFER_OFFSET_NONE;
		}
		if(BufferOffset == BUFFER_OFFSET_FULL) {
			HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_RESET);
			maxObservedAmplitude = waveplayer_samplePianos((int16_t*)(audioBuffer+(AUDIO_BUFFER_SIZE/2)), nSamplesPerHalfBuffer, maxObservedAmplitude);
			BufferOffset = BUFFER_OFFSET_NONE;
		}
	}
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
	BufferOffset = BUFFER_OFFSET_HALF;
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void){
	BufferOffset = BUFFER_OFFSET_FULL;
	// Note: the division by two has to be present; this appears to be due to an inconsistent abstraction of the library.
	//       This is related to the bit depth of the audio samples.
	BSP_AUDIO_OUT_ChangeBuffer((uint16_t*)audioBuffer, AUDIO_BUFFER_SIZE/2);
}

void BSP_AUDIO_OUT_Error_CallBack(void) {
	/* Stop the program with an infinite loop */
	while(1) {
	}
}

