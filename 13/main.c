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

typedef struct Vec2 {
	int64_t x;
	int64_t y;
} Vec2;

typedef struct ClawMachine {
	Vec2 a;
	Vec2 b;
	Vec2 prizePos;
	int64_t tokenCost;
	bool canWin;
} ClawMachine;

Vec2 ParseVec(char* str, char token) {
	if (!str) return (Vec2){0};
	Vec2 vec;
	vec.x = atoi(strchr(str, token) + 1);
	vec.y = atoi(strrchr(str, token) + 1);
	return vec;
}

ClawMachine* ParseClawMachines(char** lines, uint32_t* machineCount) {
	if (!lines || !machineCount) return NULL;

	uint32_t lineCount = 0;
	while (lines[lineCount]) lineCount++;

	uint32_t count = lineCount / 3;
	if (!count) return NULL;

	ClawMachine* machines = malloc(sizeof(ClawMachine) * count);
	if (!machines) return NULL;

	for (uint32_t i = 0, j = 0; i < lineCount; i += 3, j++) {
		machines[j].a = ParseVec(lines[i], '+');
		machines[j].b = ParseVec(lines[i + 1], '+');
		machines[j].prizePos = ParseVec(lines[i + 2], '=');
	}

	*machineCount = count;
	return machines;
}

void SolveClawMachine(ClawMachine* machine, bool limit100) {
	if (!machine) return;
	machine->tokenCost = UINT64_MAX;
	machine->canWin = false;

	int64_t ax = machine->a.x;
	int64_t ay = machine->a.y;

	int64_t bx = machine->b.x;
	int64_t by = machine->b.y;

	int64_t px = machine->prizePos.x;
	int64_t py = machine->prizePos.y;

	/* Treating the equation as a matrix
	 *
	 * |ax bx|   |ap|   |px|
	 * |ay by| . |bp| = |py|
	 *
	 *
	 * |ap|   | by -bx|   |px|
	 * |bp| = |-ay  ax| . |py| . 1 / (ax * by - ay * bx)
	 *
	 * |ap|   |(by * px - bx * py) / (ax * by - ay * bx)|
	 * |bp| = |(ax * py - ay * px) / (ax * by - ay * bx)|
	 *
	 */

	int64_t denom = (ax * by - ay * bx);

	int64_t ap = (by * px - bx * py) / denom;
	int64_t bp = (ax * py - ay * px) / denom;

	if (ap < 0 || bp < 0) return;

	if (ax * ap + bx * bp != px) return;
	if (ay * ap + by * bp != py) return;

	if (limit100 && (ap > 100 || bp > 100)) return;
	machine->canWin = true;
	machine->tokenCost = ap * 3 + bp;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	uint32_t machineCount = 0;
	ClawMachine* machines = ParseClawMachines(input, &machineCount);

	for (uint32_t i = 0; input[i]; i++) {
		printf("%s\n", input[i]);
		free(input[i]);
	}
	free(input);
	if (!machines) exit(1);

	int64_t totalCost = 0;
	int64_t totalCostRigged = 0;
	for (uint32_t i = 0; i < machineCount; i++) {
		SolveClawMachine(machines + i, true);
		if (machines[i].canWin) totalCost += machines[i].tokenCost;

		machines[i].prizePos.x += 10000000000000ULL;
		machines[i].prizePos.y += 10000000000000ULL;
		SolveClawMachine(machines + i, false);

		if (machines[i].canWin) totalCostRigged += machines[i].tokenCost;
	}

	printf("The total cost of token is %lu\n", totalCost);
	printf("The total (rigged) cost of token is %lu\n", totalCostRigged);

	free(machines);

	return 0;
}
