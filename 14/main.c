#include <errno.h>
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

typedef struct Robot {
	int64_t px;
	int64_t py;
	int64_t vx;
	int64_t vy;
	int64_t lx;
	int64_t ly;
} Robot;

Robot* ParseRobots(char** lines, uint32_t* robotCount) {
	if (!lines || !robotCount) return NULL;
	uint32_t count = 0;
	while (lines[count]) count++;
	if (!count) return NULL;
	Robot* robots = malloc(sizeof(Robot) * count);
	if (!robots) return NULL;
	for (uint32_t i = 0; i < count; i++) {
		robots[i].px = atoi(strchr(lines[i], '=') + 1);
		robots[i].py = atoi(strchr(lines[i], ',') + 1);
		robots[i].vx = atoi(strrchr(lines[i], '=') + 1);
		robots[i].vy = atoi(strrchr(lines[i], ',') + 1);
	}
	*robotCount = count;
	return robots;
}

#define WIDTH 101
#define HEIGHT 103
#define SECONDS 100

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);
	uint32_t robotCount = 0;
	Robot* robots = ParseRobots(input, &robotCount);
	for (uint32_t i = 0; input[i]; i++) {
		free(input[i]);
	}
	free(input);
	if (!robots) exit(1);

	uint32_t nw = 0;
	uint32_t ne = 0;
	uint32_t sw = 0;
	uint32_t se = 0;
	for (uint32_t i = 0; i < robotCount; i++) {
		robots[i].lx = robots[i].px + robots[i].vx * SECONDS;
		while (robots[i].lx < 0) robots[i].lx += WIDTH;
		robots[i].lx %= WIDTH;
		robots[i].lx -= WIDTH / 2;

		robots[i].ly = robots[i].py + robots[i].vy * SECONDS;
		while (robots[i].ly < 0) robots[i].ly += HEIGHT;
		robots[i].ly %= HEIGHT;
		robots[i].ly -= HEIGHT / 2;

		if (robots[i].ly < 0) {
			if (robots[i].lx < 0) nw++;
			if (robots[i].lx > 0) ne++;
		}
		if (robots[i].ly > 0) {
			if (robots[i].lx < 0) sw++;
			if (robots[i].lx > 0) se++;
		}
	}
	uint64_t totalSafety = nw * ne * sw * se;

	uint32_t seconds = 0;
	printf("Enter the number of seconds you want the robots to run for : ");
	scanf("%u", &seconds);
	printf("Running the robots for %u seconds and printing results in img subfolder\n", seconds);

	char board[WIDTH * HEIGHT] = {0};
	char fileNameBuffer[64];

	system("rm -rf img");
	system("mkdir img");

	FILE* output = fopen("img/robots.txt", "w");
	if (!output) {
		printf("%s : %s\n", fileNameBuffer, strerror(errno));
		free(robots);
		exit(1);
	}

	for (uint32_t t = 0; t < seconds; t++) {
		fprintf(output, "%u:\n", t);

		memset(board, ' ', WIDTH * HEIGHT);
		for (uint32_t i = 0; i < robotCount; i++) {
			board[robots[i].px + robots[i].py * WIDTH] = '#';
			robots[i].px += robots[i].vx;
			if (robots[i].px < 0) robots[i].px += WIDTH;
			robots[i].px %= WIDTH;

			robots[i].py += robots[i].vy;
			if (robots[i].py < 0) robots[i].py += HEIGHT;
			robots[i].py %= HEIGHT;
		}
		for (uint32_t i = 0; i < HEIGHT; i++) {
			fprintf(output, "%.*s\n", WIDTH, board + i * WIDTH);
		}

		fprintf(output, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	}
	fclose(output);

	printf("The total safety after %d seconds is %lu\n", SECONDS, totalSafety);

	free(robots);

	return 0;
}
