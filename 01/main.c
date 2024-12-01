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
	return res;
}

int32_t FastParse5(char* str) {
	return ((str[0] - '0') * 10000) + ((str[1] - '0') * 1000) + ((str[2] - '0') * 100) +
		   ((str[3] - '0') * 10) + ((str[4] - '0') * 1);
}

int32_t CompareInt32(const void* a, const void* b) { return *((const int*)a) - *((const int*)b); }

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);
	char** tmp = input;

	uint32_t listSize = 0;
	while (*tmp++) listSize++;

	int32_t* left = malloc(sizeof(int32_t) * listSize);
	int32_t* right = malloc(sizeof(int32_t) * listSize);
	if (!left || !right) {
		tmp = input;
		while (*tmp) free(*tmp++);
		free(input);
		exit(1);
	}

	uint32_t idx = 0;

	while (input[idx]) {
		left[idx] = FastParse5(input[idx]);
		right[idx] = FastParse5(input[idx] + 8);
		idx++;
	}

	qsort(left, listSize, sizeof(int32_t), CompareInt32);
	qsort(right, listSize, sizeof(int32_t), CompareInt32);

	uint32_t totalDifference = 0;
	uint32_t totalSimilarity = 0;
	for (uint32_t i = 0; i < listSize; i++) {
		totalDifference += abs(left[i] - right[i]);
		for (uint32_t j = 0; j < listSize && right[j] <= left[i]; j++) {
			if (right[j] == left[i]) totalSimilarity += left[i];
		}
	}

	printf("Total Difference : %u\n", totalDifference);
	printf("Total Similarity : %u\n", totalSimilarity);

	tmp = input;
	while (*tmp) free(*tmp++);
	free(input);

	free(left);
	free(right);

	return 0;
}
