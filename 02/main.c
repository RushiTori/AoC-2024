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

typedef struct Record {
	uint8_t* values;
	uint32_t size;
} Record;

Record ParseRecord(char* str) {
	Record res = {0};
	char* tmp = str;
	while (*tmp) {
		if (*tmp++ == ' ') res.size++;
	}
	res.size++;

	res.values = calloc(res.size, sizeof(uint8_t));
	if (!res.values) return (Record){0};

	uint32_t idx = 0;
	while (*str) {
		while (!isdigit(*str)) str++;
		while (isdigit(*str)) {
			res.values[idx] = res.values[idx] * 10 + (*str++ - '0');
		}
		if (*str) idx++;
	}

	return res;
}

bool lt(uint8_t a, uint8_t b) { return (a < b) && (abs(a - b) >= 1) && (abs(a - b) <= 3); }

bool gt(uint8_t a, uint8_t b) { return (a > b) && (abs(a - b) >= 1) && (abs(a - b) <= 3); }

bool IsRecordSafe(Record record) {
	if (record.size == 0) return false;
	if (record.size == 1) return true;

	bool (*cmp)(uint8_t, uint8_t) = gt;

	if (record.values[0] < record.values[1]) cmp = lt;

	for (uint32_t i = 0; i < record.size - 1; i++) {
		if (!cmp(record.values[i], record.values[i + 1])) return false;
	}
	return true;
}

bool IsRecordSafeDampened(Record record) {
	if (IsRecordSafe(record)) return true;
	if (record.size == 2) return true;

	for (uint32_t banned = 0; banned < record.size; banned++) {
		bool (*cmp)(uint8_t, uint8_t) = gt;
		if (banned == 0) {
			if (record.values[1] < record.values[2]) cmp = lt;
		} else if (banned == 1) {
			if (record.values[0] < record.values[2]) cmp = lt;
		} else {
			if (record.values[0] < record.values[1]) cmp = lt;
		}

		bool res = true;

		for (uint32_t i = 0; i < record.size - 1; i++) {
			if (i == banned) continue;
			if (i + 1 == banned) {
				if (i + 2 < record.size && !cmp(record.values[i], record.values[i + 2])) {
					res = false;
					break;
				}
				continue;
			}
			if (!cmp(record.values[i], record.values[i + 1])) {
				res = false;
				break;
			}
		}

		if (res) return true;
	}

	return false;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	char** tmp = input;
	uint32_t recordCount = 0;
	while (*tmp++) recordCount++;

	Record* records = malloc(sizeof(Record) * recordCount);
	if (!records) {
		tmp = input;
		while (*tmp) free(*tmp++);
		free(input);
		exit(1);
	}

	for (uint32_t i = 0; i < recordCount; i++) {
		records[i] = ParseRecord(input[i]);
		free(input[i]);
	}
	free(input);

	for (uint32_t i = 0; i < recordCount; i++) {
		if (!records[i].values) {
			while (i) free(records[--i].values);
			free(records);
			exit(1);
		}
	}

	uint32_t totalSafe = 0;
	uint32_t totalSafeDampened = 0;
	for (uint32_t i = 0; i < recordCount; i++) {
		if (IsRecordSafe(records[i])) totalSafe++;
		if (IsRecordSafeDampened(records[i])) totalSafeDampened++;
	}

	printf("Total Safe : %u\n", totalSafe);
	printf("Total Safe Dampened : %u\n", totalSafeDampened);

	for (uint32_t i = 0; i < recordCount; i++) {
		free(records[i].values);
	}
	free(records);

	return 0;
}
