#include <regex.h>
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

#define mulRegText "mul\\([0-9][0-9]?[0-9]?,[0-9][0-9]?[0-9]?\\)"
#define doRegText "(do\\(\\))"
#define dontRegText "(don[']t\\(\\))"

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	const char* mulDoDontRegText = "(" mulRegText "|" doRegText "|" dontRegText ")";

	regex_t mulRegex;
	if (regcomp(&mulRegex, mulDoDontRegText, REG_EXTENDED)) exit(1);

	char* input = OpenInputAsBuffer(NULL);
	if (!input) {
		regfree(&mulRegex);
		exit(1);
	}

	char* tmp = input;
	regmatch_t matchInfos = {0};
	char* matchStr = NULL;

	uint32_t totalMul = 0;
	uint32_t totalEnabledMul = 0;
	bool enabled = true;
	while (!regexec(&mulRegex, tmp, 1, &matchInfos, 0)) {
		matchStr = tmp + matchInfos.rm_so;

		if (matchStr[0] == 'm') {
			uint32_t lhs = atoi(strchr(matchStr, '(') + 1);
			uint32_t rhs = atoi(strchr(matchStr, ',') + 1);
			totalMul += lhs * rhs;
			if (enabled) totalEnabledMul += lhs * rhs;
		} else if (matchStr[2] == 'n') {
			enabled = false;
		} else {
			enabled = true;
		}

		tmp += matchInfos.rm_eo;
	}

	printf("Total Mul : %u\n", totalMul);
	printf("Total Enabled Mul : %u\n", totalEnabledMul);

	free(input);
	regfree(&mulRegex);

	return 0;
}
