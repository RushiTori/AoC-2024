#include <math.h>
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

typedef struct Calibration {
	uint64_t result;
	uint64_t* operands;
	uint32_t operandCount;
	bool isValid;
} Calibration;

Calibration ParseCalibration(char* str) {
	Calibration tmp = {0};

	tmp.result = atoll(str);

	for (uint32_t i = 0; str[i]; i++) {
		if (str[i] == ' ') tmp.operandCount++;
	}

	tmp.operands = malloc(sizeof(uint64_t) * tmp.operandCount);
	if (!tmp.operands) return (Calibration){0};

	for (uint32_t i = 0, j = 0; str[i]; i++) {
		if (str[i] == ' ') {
			tmp.operands[j++] = atoll(str + i + 1);
		}
	}

	return tmp;
}

bool CheckCaliInternal(uint64_t* values, uint64_t count, uint64_t current, uint64_t expected) {
	if (!count) return current == expected;
	return CheckCaliInternal(values + 1, count - 1, current * *values, expected) ||
		   CheckCaliInternal(values + 1, count - 1, current + *values, expected);
}

void CheckCalibration(Calibration* calibration) {
	calibration->isValid =
		CheckCaliInternal(calibration->operands + 1, calibration->operandCount - 1,
						  *calibration->operands, calibration->result);
}

uint64_t Concat64(uint64_t a, uint64_t b) { return a * pow(10, floor(log10(b)) + 1) + b; }

bool CheckCaliConcatInternal(uint64_t* values, uint64_t count, uint64_t current,
							 uint64_t expected) {
	if (!count) return current == expected;
	return CheckCaliConcatInternal(values + 1, count - 1, current * *values, expected) ||
		   CheckCaliConcatInternal(values + 1, count - 1, current + *values, expected) ||
		   CheckCaliConcatInternal(values + 1, count - 1, Concat64(current, *values), expected);
}

void CheckCalibrationConcat(Calibration* calibration) {
	calibration->isValid =
		CheckCaliConcatInternal(calibration->operands + 1, calibration->operandCount - 1,
								*calibration->operands, calibration->result);
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	uint32_t calibrationCount = 0;
	while (input[calibrationCount]) calibrationCount++;

	Calibration* calibrations = malloc(sizeof(Calibration) * calibrationCount);
	if (!calibrations) {
		for (uint32_t i = 0; i < calibrationCount; i++) {
			free(input[i]);
		}
		free(input);
		exit(1);
	}

	for (uint32_t i = 0; i < calibrationCount; i++) {
		calibrations[i] = ParseCalibration(input[i]);
		if (!calibrations[i].operands) {
			while (i) free(calibrations[--i].operands);
			free(calibrations);
			while (i < calibrationCount) free(input[i++]);
			free(input);
			exit(1);
		}
		free(input[i]);
	}
	free(input);

	uint64_t totalValidCalibration = 0;
	uint64_t totalValidConcatCalibration = 0;

	for (uint32_t i = 0; i < calibrationCount; i++) {
		CheckCalibration(calibrations + i);
		if (calibrations[i].isValid) {
			totalValidCalibration += calibrations[i].result;
		} else {
			CheckCalibrationConcat(calibrations + i);
			if (calibrations[i].isValid) {
				totalValidConcatCalibration += calibrations[i].result;
			}
		}
		free(calibrations[i].operands);
	}
	free(calibrations);

	printf("Total correct calibration results : %lu\n", totalValidCalibration);
	printf("Total correct calibration (with concat) results : %lu\n",
		   totalValidCalibration + totalValidConcatCalibration);

	return 0;
}
