/* ------------------------------------------------------------------
 * Copyright (C) 2009 Martin Storsjo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "wav.h"
extern "C" {
#include <dec_if.h>
}

/* From pvamrwbdecoder_api.h, by dividing by 8 and rounding up */
const int sizes[] = { 17, 23, 32, 36, 40, 46, 50, 58, 60, 5, -1, -1, -1, -1, -1, -1 };

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s in.amr\n", argv[0]);
		return 1;
	}

	FILE* in = fopen(argv[1], "rb");
	if (!in) {
		perror(argv[1]);
		return 1;
	}
	char header[9];
	int n = fread(header, 1, 9, in);
	if (n != 9 || memcmp(header, "#!AMR-WB\n", 9)) {
		fprintf(stderr, "Bad header\n");
		return 1;
	}

	WavWriter wav("out.wav", 16000, 16, 1);
	void* amr = D_IF_init();
	while (true) {
		uint8_t buffer[500];
		/* Read the mode byte */
		n = fread(buffer, 1, 1, in);
		if (n <= 0)
			break;
		/* Find the packet size */
		int size = sizes[(buffer[0] >> 3) & 0x0f];
		if (size <= 0)
			break;
		n = fread(buffer + 1, 1, size, in);
		if (n != size)
			break;

		/* Decode the packet */
		int16_t outbuffer[320];
		D_IF_decode(amr, buffer, outbuffer, 0);

		/* Convert to little endian and write to wav */
		uint8_t littleendian[640];
		uint8_t* ptr = littleendian;
		for (int i = 0; i < 320; i++) {
			*ptr++ = (outbuffer[i] >> 0) & 0xff;
			*ptr++ = (outbuffer[i] >> 8) & 0xff;
		}
		wav.writeData(littleendian, 640);
	}
	fclose(in);
	D_IF_exit(amr);
	return 0;
}

