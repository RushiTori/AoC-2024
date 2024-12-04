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

uint32_t SearchXMAS(char** grid, uint32_t width, uint32_t height) {
	uint32_t count = 0;

	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			if (grid[y][x] == 'X') {
				if (!strncmp(grid[y] + x, "XMAS", 4)) count++;
				if (y + 3 < height) {
					if (grid[y + 1][x] == 'M' && grid[y + 2][x] == 'A' && grid[y + 3][x] == 'S') {
						count++;
					}

					if (x + 3 < width) {
						if (grid[y + 1][x + 1] == 'M' && grid[y + 2][x + 2] == 'A' &&
							grid[y + 3][x + 3] == 'S') {
							count++;
						}
					}

					if (x >= 3) {
						if (grid[y + 1][x - 1] == 'M' && grid[y + 2][x - 2] == 'A' &&
							grid[y + 3][x - 3] == 'S') {
							count++;
						}
					}
				}
			}
			if (grid[y][x] == 'S') {
				if (!strncmp(grid[y] + x, "SAMX", 4)) count++;
				if (y + 3 < height) {
					if (grid[y + 1][x] == 'A' && grid[y + 2][x] == 'M' && grid[y + 3][x] == 'X') {
						count++;
					}
					if (x + 3 < width) {
						if (grid[y + 1][x + 1] == 'A' && grid[y + 2][x + 2] == 'M' &&
							grid[y + 3][x + 3] == 'X') {
							count++;
						}
					}

					if (x >= 3) {
						if (grid[y + 1][x - 1] == 'A' && grid[y + 2][x - 2] == 'M' &&
							grid[y + 3][x - 3] == 'X') {
							count++;
						}
					}
				}
			}
		}
	}

	return count;
}

uint32_t SearchX_MAS(char** grid, uint32_t height) {
	regex_t masReg;
	if (regcomp(&masReg, "[MS].[MS]", REG_EXTENDED)) return -1;

	regmatch_t matchInfo = {0};
	uint32_t count = 0;
	for (uint32_t i = 0; i < height - 2; i++) {
		uint32_t offset = 0;
		while (!regexec(&masReg, grid[i] + offset, 1, &matchInfo, 0)) {
			char topLeft = grid[i][offset + matchInfo.rm_so];
			char topRight = grid[i][offset + matchInfo.rm_so + 2];

			char bottomLeft = grid[i + 2][offset + matchInfo.rm_so];
			char bottomRight = grid[i + 2][offset + matchInfo.rm_so + 2];

			char middle = grid[i + 1][offset + matchInfo.rm_so + 1];

			if (middle == 'A') {
				if ((topLeft == 'M' && bottomRight == 'S') ||
					(topLeft == 'S' && bottomRight == 'M')) {
					if ((bottomLeft == 'M' && topRight == 'S') ||
						(bottomLeft == 'S' && topRight == 'M')) {
						count++;
					}
				}
			}
			offset += matchInfo.rm_so + 1;
		}
	}

	regfree(&masReg);

	return count;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	uint32_t width = strlen(*input);
	uint32_t height = 0;
	while (input[height]) height++;

	uint32_t xmasCount = SearchXMAS(input, width, height);
	uint32_t x_masCount = SearchX_MAS(input, height);

	printf("The (%ux%u)word puzzle contains %u XMAS", width, height, xmasCount);
	printf(" and %u X-MAS\n", x_masCount);

	while (height) free(input[--height]);
	free(input);

	return 0;
}
