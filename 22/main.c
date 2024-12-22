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

#define PRUNE(num) ((num)&0xFFFFFFLLU)
#define MIX(a, b) ((a) ^ (b))

uint64_t Hash(uint64_t num) {
	num = PRUNE(MIX(num, num << 6));
	num = PRUNE(MIX(num, num >> 5));
	num = PRUNE(MIX(num, num << 11));
	return num;
}

int64_t* CreateMemo(uint32_t size) {
	int64_t* memo = malloc(sizeof(int64_t) * size);
	return memo;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	const uint32_t memoLen = (1 << 20) + 1;
	int64_t* totalMemo = CreateMemo(memoLen);
	int64_t* tempMemo = CreateMemo(memoLen);

	if (!totalMemo || !tempMemo) {
		free(totalMemo);
		free(tempMemo);
		exit(1);
	}

	char** input = OpenInputAsStringArray();
	if (!input) {
		free(totalMemo);
		free(tempMemo);
		exit(1);
	}

	memset(totalMemo, 0, sizeof(int64_t) * memoLen);

	uint64_t totalRand = 0;
	for (uint32_t i = 0; input[i]; i++) {
		uint64_t code = atoll(input[i]);
		uint64_t next = code;

		uint32_t diffIdx = 0;
		memset(tempMemo, -1, sizeof(int64_t) * memoLen);
		for (uint32_t j = 0; j < 2000; j++) {
			next = Hash(code);
			diffIdx = ((diffIdx & 0x7FFF) << 5) | (((next % 10) - (code % 10)) & 0x1F);
			code = next;

			if (j < 3) continue;
			if (tempMemo[diffIdx] == -1) tempMemo[diffIdx] = code % 10;
		}
		totalRand += code;

		for (uint32_t j = 0; j < memoLen; j++) {
			if (tempMemo[j] != -1) totalMemo[j] += tempMemo[j];
		}

		free(input[i]);
	}

	int64_t bestBanana = 0;
	for (uint32_t i = 0; i < memoLen; i++) {
		if (totalMemo[i] > bestBanana) bestBanana = totalMemo[i];
	}
	free(input);
	free(totalMemo);
	free(tempMemo);

	printf("total secret code : %lu\n", totalRand);
	printf("best banana profit : %lu\n", bestBanana);

	return 0;
}
