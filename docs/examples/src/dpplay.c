//
// Copyright(C) 2007 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

#include <stdio.h>
#include <windows.h>

float frequencies[] = {
    0, 175.00, 180.02, 185.01, 190.02, 196.02, 202.02, 208.01, 214.02, 220.02,
    226.02, 233.04, 240.02, 247.03, 254.03, 262.00, 269.03, 277.03, 285.04,
    294.03, 302.07, 311.04, 320.05, 330.06, 339.06, 349.08, 359.06, 370.09,
    381.08, 392.10, 403.10, 415.01, 427.05, 440.12, 453.16, 466.08, 480.15,
    494.07, 508.16, 523.09, 539.16, 554.19, 571.17, 587.19, 604.14, 622.09,
    640.11, 659.21, 679.10, 698.17, 719.21, 740.18, 762.41, 784.47, 807.29,
    831.48, 855.32, 880.57, 906.67, 932.17, 960.69, 988.55, 1017.20, 1046.64,
    1077.85, 1109.93, 1141.79, 1175.54, 1210.12, 1244.19, 1281.61, 1318.43,
    1357.42, 1397.16, 1439.30, 1480.37, 1523.85, 1569.97, 1614.58, 1661.81,
    1711.87, 1762.45, 1813.34, 1864.34, 1921.38, 1975.46, 2036.14, 2093.29,
    2157.64, 2217.80, 2285.78, 2353.41, 2420.24, 2490.98, 2565.97, 2639.77,
};

int read_head(FILE *stream)
{
	unsigned char head[4];
	int len;

	if (fread(head, 1, 4, stream) != 4) {
		return -1;
	}

	if (head[0] != 0 || head[1] != 0) {
		return -1;
	}

	len = (head[3] << 8) | head[2];

	return len;
}

void play_sample(int val, int len)
{
	int ms;

	ms = (len * 1000) / 140;

	if (val == 0) {
		Sleep(ms);
	} else {
		Beep((int) frequencies[val], ms);
	}
}

void play_file(char *filename)
{
	FILE *stream;
	int len;
	int i;
	int sample;
	int current_val;
	int current_len;

	stream = fopen(filename, "rb");

	if (!stream) {
		fprintf(stderr, "failed to open %s\n", filename);
		return;
	}

	len = read_head(stream);

	if (len < 0) {
		fprintf(stderr, "failed to read header or not a PC "
		                "speaker sound file!\n");
		return;
	}

	current_val = 0;
	current_len = 0;

	for (i=0; i<len; ++i) {
		sample = fgetc(stream);

		if (sample == EOF) {
			fprintf(stderr, "Error: file ended abruptly!\n");
			break;
		}

		if (sample == current_val) {
			current_len += 1;
		} else {
			play_sample(current_val, current_len);
			current_val = sample;
			current_len = 1;
		}
	}

	play_sample(current_val, current_len);

	fclose(stream);
}

int main(int argc, char *argv[])
{
	play_file(argv[1]);
}

