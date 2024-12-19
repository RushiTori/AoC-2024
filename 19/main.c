#include <ctype.h>
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

int32_t CompareU64(const void* a, const void* b) {
	uint64_t a_ = *(const uint64_t*)a;
	uint64_t b_ = *(const uint64_t*)b;
	if (a_ < b_) return -1;
	if (a_ > b_) return 1;
	return 0;
}

uint64_t* ParseTowels(char* str, uint32_t* towelCount) {
	if (!str || !towelCount) return NULL;

	uint32_t len = strlen(str);
	uint32_t count = 0;
	for (uint32_t i = 0; i < len; i++) {
		if (!isalpha(str[i])) continue;
		count++;
		while (isalpha(str[i + 1])) i++;
	}

	uint64_t* towels = calloc(count, sizeof(uint64_t));
	if (!towels) return NULL;

	for (uint32_t i = 0, idx = 0; i < len; i++) {
		if (!isalpha(str[i])) continue;
		while (isalpha(str[i])) towels[idx] = towels[idx] * 0x100 + str[i++];
		idx++;
		i--;
	}

	qsort(towels, count, sizeof(uint64_t), CompareU64);

	*towelCount = count;
	return towels;
}

int64_t IsPatternPossible(char* pattern, char* start, int64_t* memo, uint64_t* towels,
						  uint32_t towelCount) {
	if (memo[pattern - start] > -1) return memo[pattern - start];
	if (!*pattern) return 1;
	int64_t sub = 0;
	for (uint32_t i = 1; i <= 8; i++) {
		if (!pattern[i - 1]) break;
		uint64_t subPattern = 0;
		for (uint32_t j = 0; j < i; j++) {
			subPattern = subPattern * 0x100 + pattern[j];
		}
		if (!bsearch(&subPattern, towels, towelCount, sizeof(uint64_t), CompareU64)) continue;
		sub += IsPatternPossible(pattern + i, start, memo, towels, towelCount);
	}
	memo[pattern - start] = sub;

	return sub;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();

	uint32_t towelCount = 0;
	uint64_t* towels = ParseTowels(*input, &towelCount);
	if (!towels) {
		for (uint32_t i = 0; input[i]; i++) {
			free(input[i]);
		}
		free(input);
	}

	uint32_t memoLen = 0;

	for (uint32_t i = 1; input[i]; i++) {
		if (strlen(input[i]) > memoLen) memoLen = strlen(input[i]);
	}

	int64_t* memo = malloc(sizeof(int64_t) * memoLen);
	if (!memo) {
		for (uint32_t i = 0; input[i]; i++) {
			free(input[i]);
		}
		free(input);
		free(towels);
	}

	uint32_t possibleCount = 0;
	uint64_t possibleCombinations = 0;
	for (uint32_t i = 1; input[i]; i++) {
		memset(memo, 0xFF, sizeof(int64_t) * memoLen);
		int64_t combinations = IsPatternPossible(input[i], input[i], memo, towels, towelCount);
		if (combinations) possibleCount++;
		possibleCombinations += combinations;
	}

	printf("There are %u possible patterns and %lu different ways to make them !\n", possibleCount,
		   possibleCombinations);

	for (uint32_t i = 0; input[i]; i++) {
		free(input[i]);
	}
	free(input);
	free(towels);
	free(memo);

	return 0;
}
