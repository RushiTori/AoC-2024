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

typedef struct KeyLock {
	uint8_t heights[5];
	bool isLock;
} KeyLock;

int32_t CompareKeyLock(const void* a, const void* b) {
	const KeyLock* a_ = (const KeyLock*)a;
	const KeyLock* b_ = (const KeyLock*)b;

	if (!a_->isLock && b_->isLock) return -1;
	if (a_->isLock && !b_->isLock) return 1;

	for (uint32_t i = 0; i < 5; i++) {
		if (a_->heights[i] < b_->heights[i]) return -1;
		if (a_->heights[i] > b_->heights[i]) return 1;
	}

	return 0;
}

KeyLock* ParseKeyLocks(char** lines, uint32_t* countRef, uint32_t* lockStart) {
	uint32_t count = 0;
	while (lines[count]) count++;
	count /= 7;
	if (!count) return NULL;

	KeyLock* keylocks = calloc(count, sizeof(KeyLock));
	if (!keylocks) return NULL;

	for (uint32_t i = 0; i < count; i++) {
		keylocks[i].isLock = lines[i * 7][0] == '#';
		for (uint32_t j = 0; j < 5; j++) {
			for (uint32_t k = 0; k < 5; k++) {
				if (lines[(i * 7) + 1 + j][k] == '#') keylocks[i].heights[k]++;
			}
		}
	}

	qsort(keylocks, count, sizeof(KeyLock), CompareKeyLock);

	*lockStart = count;

	for (uint32_t i = 0; i < count; i++) {
		if (keylocks[i].isLock) {
			*lockStart = i;
			break;
		}
	}

	*countRef = count;

	return keylocks;
}

bool KeyLockCompatible(KeyLock a, KeyLock b) {
	for (uint32_t i = 0; i < 5; i++) {
		if (a.heights[i] + b.heights[i] > 5) return false;
	}
	return true;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	uint32_t lockStart = 0;
	uint32_t keylockCount = 0;
	KeyLock* keylocks = ParseKeyLocks(input, &keylockCount, &lockStart);

	uint32_t compatibleCount = 0;

	for (uint32_t i = 0; i < lockStart; i++) {
		for (uint32_t j = lockStart; j < keylockCount; j++) {
			if (KeyLockCompatible(keylocks[i], keylocks[j])) compatibleCount++;
		}
	}

	printf("There are %u compatible pairs\n", compatibleCount);

	free(keylocks);
	uint32_t inputLen = 0;
	while (input[inputLen]) free(input[inputLen++]);
	free(input);

	return 0;
}
