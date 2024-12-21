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

#define BUTTON_LEFT '<'
#define BUTTON_RIGHT '>'
#define BUTTON_UP '^'
#define BUTTON_DOWN 'v'

#define BUTTON_A 'A'

#define BUTTON_0 '0'
#define BUTTON_1 '1'
#define BUTTON_2 '2'
#define BUTTON_3 '3'
#define BUTTON_4 '4'
#define BUTTON_5 '5'
#define BUTTON_6 '6'
#define BUTTON_7 '7'
#define BUTTON_8 '8'
#define BUTTON_9 '9'

#define BUTTON_X '\0'

typedef struct Vec {
	int32_t x;
	int32_t y;
} Vec;

#define VecSub(a, b)                                                                               \
	(Vec) { .x = (a).x - (b).x, .y = (a).y - (b).y }

Vec keyPad[0x100] = {
	[BUTTON_7] = (Vec){0, 0}, [BUTTON_8] = (Vec){1, 0}, [BUTTON_9] = (Vec){2, 0},
	[BUTTON_4] = (Vec){0, 1}, [BUTTON_5] = (Vec){1, 1}, [BUTTON_6] = (Vec){2, 1},
	[BUTTON_1] = (Vec){0, 2}, [BUTTON_2] = (Vec){1, 2}, [BUTTON_3] = (Vec){2, 2},
	[BUTTON_X] = (Vec){0, 3}, [BUTTON_0] = (Vec){1, 3}, [BUTTON_A] = (Vec){2, 3},
};

Vec robotPad[0x100] = {
	[BUTTON_X] = (Vec){0, 0},	 [BUTTON_UP] = (Vec){1, 0},	  [BUTTON_A] = (Vec){2, 0},
	[BUTTON_LEFT] = (Vec){0, 1}, [BUTTON_DOWN] = (Vec){1, 1}, [BUTTON_RIGHT] = (Vec){2, 1},
};

#define MEMO_WIDTH 3
#define MEMO_HEIGHT 4
#define MEMO_PLANE (MEMO_WIDTH * MEMO_HEIGHT)
#define MEMO_PLANE_2 (MEMO_PLANE * MEMO_PLANE)

uint64_t* CreateMemo(uint32_t depth) {
	uint32_t memoLen = MEMO_PLANE_2 * depth;
	uint64_t* memo = malloc(sizeof(uint64_t) * memoLen);
	if (memo) memset(memo, 0xFF, sizeof(uint64_t) * memoLen);
	return memo;
}

#define MemoGet(memo, sX, sY, eX, eY, d)                                                           \
	(memo)[(sX) + ((sY)*MEMO_WIDTH) + ((eX)*MEMO_PLANE) + ((eY)*MEMO_PLANE * MEMO_WIDTH) +         \
		   ((d)*MEMO_PLANE_2)]

uint64_t GetComplexity(char buttonA, char buttonB, Vec* pad, uint32_t depth, uint64_t* memo,
					   uint32_t maxDepth) {
	if (depth >= maxDepth) return 1;

	uint32_t sX = pad[(uint32_t)buttonA].x;
	uint32_t sY = pad[(uint32_t)buttonA].y;
	uint32_t eX = pad[(uint32_t)buttonB].x;
	uint32_t eY = pad[(uint32_t)buttonB].y;

	if (MemoGet(memo, sX, sY, eX, eY, depth) != UINT64_MAX) {
		return MemoGet(memo, sX, sY, eX, eY, depth);
	}

	uint32_t badX = pad[(uint32_t)BUTTON_X].x;
	uint32_t badY = pad[(uint32_t)BUTTON_X].y;

	uint32_t diffX = abs((int32_t)sX - (int32_t)eX) - 1;
	uint32_t diffY = abs((int32_t)sY - (int32_t)eY) - 1;

	uint64_t complexity = UINT64_MAX;

	if (!(sX == badX && eY == badY)) {
		uint64_t subComplexity = 0;
		char midA = BUTTON_A;
		char midB;

		if (sY != eY) {
			midA = BUTTON_UP;
			if (eY > sY) midA = BUTTON_DOWN;
			subComplexity += GetComplexity(BUTTON_A, midA, robotPad, depth + 1, memo, maxDepth);
			subComplexity += diffY;
		}

		midB = midA;
		if (sX != eX) {
			midB = BUTTON_LEFT;
			if (eX > sX) midB = BUTTON_RIGHT;
			subComplexity += GetComplexity(midA, midB, robotPad, depth + 1, memo, maxDepth);
			subComplexity += diffX;
		}
		subComplexity += GetComplexity(midB, BUTTON_A, robotPad, depth + 1, memo, maxDepth);

		if (subComplexity < complexity) complexity = subComplexity;
	}

	if (!(sY == badY && eX == badX)) {
		uint64_t subComplexity = 0;
		char midA = BUTTON_A;
		char midB;

		if (sX != eX) {
			midA = BUTTON_LEFT;
			if (eX > sX) midA = BUTTON_RIGHT;
			subComplexity += GetComplexity(BUTTON_A, midA, robotPad, depth + 1, memo, maxDepth);
			subComplexity += diffX;
		}

		midB = midA;
		if (sY != eY) {
			midB = BUTTON_UP;
			if (eY > sY) midB = BUTTON_DOWN;
			subComplexity += GetComplexity(midA, midB, robotPad, depth + 1, memo, maxDepth);
			subComplexity += diffY;
		}
		subComplexity += GetComplexity(midB, BUTTON_A, robotPad, depth + 1, memo, maxDepth);

		if (subComplexity < complexity) complexity = subComplexity;
	}

	if (complexity == UINT64_MAX) complexity = 1;
	MemoGet(memo, sX, sY, eX, eY, depth) = complexity;
	return complexity;
}

uint64_t GetTotalComplexity(char** codes, uint32_t depth) {
	uint64_t* memo = CreateMemo(depth);
	if (!memo) return 0;

	uint64_t totalComplexity = 0;

	for (uint32_t i = 0; codes[i]; i++) {
		uint64_t complexity = GetComplexity(BUTTON_A, codes[i][0], keyPad, 0, memo, depth);
		for (uint32_t j = 0; codes[i][j + 1]; j++) {
			complexity += GetComplexity(codes[i][j], codes[i][j + 1], keyPad, 0, memo, depth);
		}
		totalComplexity += complexity * atoi(codes[i]);
	}

	free(memo);

	return totalComplexity;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	const uint32_t depth1 = 3;
	const uint32_t depth2 = 26;

	uint64_t totalComplexity1 = GetTotalComplexity(input, depth1);
	uint64_t totalComplexity2 = GetTotalComplexity(input, depth2);

	printf("The total complexity for 3 layers is %lu\n", totalComplexity1);
	printf("The total complexity for 26 layers is %lu\n", totalComplexity2);

	for (uint32_t i = 0; input[i]; i++) {
		free(input[i]);
	}
	free(input);

	return 0;
}
