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

typedef enum GuardDir {
	NORTH = 0,
	EAST,
	SOUTH,
	WEST,
	COUNT,
} GuardDir;

bool FindGuard(char** board, int32_t* x, int32_t* y, GuardDir* dir) {
	while (board[*y]) {
		if (strchr(board[*y], '^')) {
			*x = strchr(board[*y], '^') - board[*y];
			*dir = NORTH;
			break;
		}
		if (strchr(board[*y], '>')) {
			*x = strchr(board[*y], '>') - board[*y];
			*dir = EAST;
			break;
		}
		if (strchr(board[*y], '<')) {
			*x = strchr(board[*y], '<') - board[*y];
			*dir = SOUTH;
			break;
		}
		if (strchr(board[*y], 'v')) {
			*x = strchr(board[*y], 'v') - board[*y];
			*dir = WEST;
			break;
		}
		(*y)++;
	}

	return (*dir != COUNT);
}

bool InBound(int32_t x, int32_t y, int32_t width, int32_t height) {
	return (x >= 0) && (x < width) && (y >= 0) && (y < height);
}

void CleanBoard(char** board) {
	for (uint32_t j = 0; board[j]; j++) {
		for (uint32_t i = 0; board[j][i]; i++) {
			if (board[j][i] == '#') continue;
			board[j][i] = '.';
		}
	}
}

uint32_t CountGuardPositions(char** board, int32_t width, int32_t height, uint32_t x, uint32_t y,
							 GuardDir dir) {
	while (true) {
		board[y][x] = 'X';

		int32_t nextY = y;
		int32_t nextX = x;

		switch (dir) {
			case NORTH: nextY--; break;
			case EAST: nextX++; break;
			case SOUTH: nextY++; break;
			case WEST: nextX--; break;
			default: break;
		}

		if (!InBound(nextX, nextY, width, height)) break;

		if (board[nextY][nextX] == '#') {
			dir = (dir + 1) % COUNT;
		} else {
			x = nextX;
			y = nextY;
		}
	}

	uint32_t res = 0;

	for (int32_t j = 0; j < height; j++) {
		for (int32_t i = 0; i < width; i++) {
			if (board[j][i] == 'X') res++;
		}
	}

	return res;
}

bool CheckLoop(char** board, int32_t width, int32_t height, int32_t x, int32_t y, GuardDir dir) {
	uint32_t limit = width * height * 4;
	uint32_t steps = 0;

	while (true) {
		int32_t nextY = y;
		int32_t nextX = x;

		switch (dir) {
			case NORTH: nextY--; break;
			case EAST: nextX++; break;
			case SOUTH: nextY++; break;
			case WEST: nextX--; break;
			default: break;
		}

		if (!InBound(nextX, nextY, width, height)) break;

		if (board[nextY][nextX] == '#') {
			dir = (dir + 1) % COUNT;
		} else {
			x = nextX;
			y = nextY;
			steps++;
			if (steps > limit + 1) return true;
		}
	}

	return false;
}

uint32_t CountPossibleLoops(char** board, int32_t width, int32_t height, int32_t x, int32_t y,
							GuardDir dir) {
	int32_t oriX = x;
	int32_t oriY = y;
	int32_t oriDir = dir;

	while (true) {
		int32_t nextY = y;
		int32_t nextX = x;

		switch (dir) {
			case NORTH: nextY--; break;
			case EAST: nextX++; break;
			case SOUTH: nextY++; break;
			case WEST: nextX--; break;
			default: break;
		}

		if (!InBound(nextX, nextY, width, height)) break;

		if (board[nextY][nextX] == '#') {
			dir = (dir + 1) % COUNT;
		} else {
			board[nextY][nextX] = '#';
			if (CheckLoop(board, width, height, oriX, oriY, oriDir)) {
				board[nextY][nextX] = 'O';
			} else {
				board[nextY][nextX] = '.';
			}

			x = nextX;
			y = nextY;
		}
	}

	uint32_t res = 0;

	for (int32_t j = 0; j < height; j++) {
		for (int32_t i = 0; i < width; i++) {
			if (board[j][i] == 'O') res++;
		}
	}

	return res;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	int32_t width = strlen(*input);
	int32_t height = 0;
	while (input[height]) height++;

	int32_t y = 0;
	int32_t x = 0;
	GuardDir dir = COUNT;

	if (!FindGuard(input, &x, &y, &dir)) {
		for (int32_t i = 0; i < height; i++) {
			free(input[i]);
		}
		free(input);
		printf("There are no guards on the board\n");
		exit(2);
	}

	uint32_t totalMappedPositions = CountGuardPositions(input, width, height, x, y, dir);
	CleanBoard(input);
	uint32_t totalPossibleLoops = CountPossibleLoops(input, width, height, x, y, dir);

	printf("The guard mapped %u different positions\n", totalMappedPositions);
	printf("And there are %u different possible loops\n", totalPossibleLoops);

	for (int32_t i = 0; i < height; i++) {
		free(input[i]);
	}
	free(input);

	return 0;
}
