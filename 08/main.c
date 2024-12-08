#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* OpenInputAsBuffer(uint32_t* size) {
	FILE* input = fopen("input.txt", "r");
	if (!input) return NULL;

	fseek(input, 0, SEEK_END);
	uint32_t tmpSize = (uint32_t)ftell(input);
	fseek(input, 0, SEEK_SET);
	char* res = malloc(sizeof(char) * (tmpSize + 1));

	if (!res) {
		fclose(input);
		return NULL;
	}

	fread(res, sizeof(char), tmpSize, input);

	if (size) *size = tmpSize;

	fclose(input);

	res[tmpSize] = '\0';
	return res;
}

char** OpenInputAsStringArray() {
	uint32_t bufferSize = 0;
	char* buffer = OpenInputAsBuffer(&bufferSize);
	if (!buffer || !bufferSize) return NULL;

	uint32_t lineCount = 0;
	for (char* ptr = buffer; ptr && *ptr; ptr = strchr(ptr, '\n')) {
		while (*ptr == '\n') ptr++;
		if (*ptr) lineCount++;
	}

	char** res = malloc(sizeof(char*) * (lineCount + 1));
	uint32_t idx = 0;

	for (char *ptr = buffer, *end = strchr(ptr, '\n'); end && *ptr; end = strchr(ptr, '\n')) {
		uint32_t tmpSize = end - ptr;
		char* tmp = malloc(sizeof(char) * (tmpSize + 1));

		if (!tmp) {
			while (idx) free(res[--idx]);
			free(res);
			return NULL;
		}

		memcpy(tmp, ptr, tmpSize);
		tmp[tmpSize] = '\0';
		res[idx++] = tmp;

		ptr = end;
		while (*ptr == '\n') ptr++;
	}

	free(buffer);

	res[lineCount] = NULL;
	return res;
}

typedef struct Antenna {
	char label;
	int32_t x;
	int32_t y;
} Antenna;

int32_t CompareAntenna(const void* a, const void* b) {
	const Antenna* a_ = (const Antenna*)a;
	const Antenna* b_ = (const Antenna*)b;

	if (a_->label == b_->label) {
		if (a_->y == b_->y) return a_->x - b_->x;
		return a_->y - b_->y;
	}
	return a_->label - b_->label;
}

bool InBounds(int32_t x, int32_t y, int32_t width, int32_t height) {
	if (x < 0) return false;
	if (x >= width) return false;
	if (y < 0) return false;
	if (y >= height) return false;
	return true;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	int32_t width = strlen(*input);
	int32_t height = 0;
	while (input[height]) height++;

	int32_t antennasCount = 0;
	for (int32_t y = 0; y < height; y++) {
		for (int32_t x = 0; x < width; x++) {
			if (input[y][x] != '.') antennasCount++;
		}
	}

	Antenna* antennas = malloc(sizeof(Antenna) * antennasCount);

	if (!antennas) {
		for (int32_t y = 0; y < height; y++) {
			free(input[y]);
		}
		free(input);
		exit(1);
	}

	int32_t idx = 0;
	for (int32_t y = 0; y < height; y++) {
		for (int32_t x = 0; x < width; x++) {
			if (input[y][x] != '.') {
				antennas[idx].label = input[y][x];
				antennas[idx].x = x;
				antennas[idx].y = y;
				idx++;
			}
		}
	}

	qsort(antennas, antennasCount, sizeof(Antenna), CompareAntenna);

	for (int32_t i = 0; i < antennasCount; i++) {
		for (int32_t j = i + 1; j < antennasCount; j++) {
			if (antennas[j].label != antennas[i].label) break;
			int32_t xDiff = antennas[j].x - antennas[i].x;
			int32_t yDiff = antennas[j].y - antennas[i].y;

			if (InBounds(antennas[j].x + xDiff, antennas[j].y + yDiff, width, height)) {
				input[antennas[j].y + yDiff][antennas[j].x + xDiff] = '#';
			}

			if (InBounds(antennas[i].x - xDiff, antennas[i].y - yDiff, width, height)) {
				input[antennas[i].y - yDiff][antennas[i].x - xDiff] = '#';
			}
		}
	}

	uint32_t antinodeCount = 0;

	for (int32_t y = 0; y < height; y++) {
		for (int32_t x = 0; x < width; x++) {
			if (input[y][x] == '#') antinodeCount++;
		}
	}

	for (int32_t i = 0; i < antennasCount; i++) {
		for (int32_t j = i + 1; j < antennasCount; j++) {
			if (antennas[j].label != antennas[i].label) break;
			int32_t xDiff = antennas[j].x - antennas[i].x;
			int32_t yDiff = antennas[j].y - antennas[i].y;

			int32_t time = 0;
			while (InBounds(antennas[j].x + (xDiff * time), antennas[j].y + (yDiff * time), width,
							height)) {
				input[antennas[j].y + (yDiff * time)][antennas[j].x + (xDiff * time)] = '#';
				time++;
			}

			time = 0;
			while (InBounds(antennas[i].x - (xDiff * time), antennas[i].y - (yDiff * time), width,
							height)) {
				input[antennas[i].y - (yDiff * time)][antennas[i].x - (xDiff * time)] = '#';
				time++;
				
			}
		}
	}

	uint32_t harmonicAntinodeCount = 0;

	for (int32_t y = 0; y < height; y++) {
		for (int32_t x = 0; x < width; x++) {
			if (input[y][x] == '#') harmonicAntinodeCount++;
		}
	}

	printf("There are %u antinodes and %u harmonic antinodes\n", antinodeCount,
		   harmonicAntinodeCount);

	for (int32_t y = 0; y < height; y++) {
		printf("%s\n", input[y]);
		free(input[y]);
	}
	free(input);
	free(antennas);

	return 0;
}
