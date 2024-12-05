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

typedef struct UpdateList {
	uint8_t* IDs;
	uint32_t size;
} UpdateList;

UpdateList ParseUpdateList(char* str) {
	UpdateList list = {0};
	list.size = (strlen(str) + 1) / 3;
	list.IDs = malloc(sizeof(uint8_t) * list.size);

	if (!list.IDs) return (UpdateList){0};

	uint32_t idx = 0;

	do {
		list.IDs[idx++] = atoi(str);
		str = strchr(str, ',');
		if (str) str++;
	} while (str);

	return list;
}

void FixUpdateList(UpdateList list, bool* rules, bool* done) {
	bool sorted = false;
	while (!sorted) {
		sorted = true;
		for (uint32_t i = 0; i < list.size; i++) {
			bool inOrder = true;
			for (uint32_t j = i + 1; j < list.size; j++) {
				if (rules[list.IDs[i] * 100 + list.IDs[j]]) {
					if (!done[list.IDs[j]]) {
						uint8_t tmp = list.IDs[i];
						list.IDs[i] = list.IDs[j];
						list.IDs[j] = tmp;
						inOrder = false;
						break;
					}
				}
			}
			if (!inOrder) {
				sorted = false;
				break;
			}
		}
		memset(done, false, sizeof(bool) * 100);
	}
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	uint32_t infosCount = 0;
	while (strchr(input[infosCount], '|')) infosCount++;

	uint32_t listsCount = infosCount;
	while (input[listsCount]) listsCount++;
	listsCount -= infosCount;

	UpdateList* lists = malloc(sizeof(UpdateList) * listsCount);

	if (!lists) {
		for (uint32_t i = 0; input[i]; i++) {
			free(input[i]);
		}
		free(input);
		free(lists);
		exit(1);
	}

	for (uint32_t i = 0; i < listsCount; i++) {
		lists[i] = ParseUpdateList(input[infosCount + i]);
		if (!lists[i].IDs) {
			while (i) free(lists[--i].IDs);
			while (input[i]) free(input[i++]);
			free(input);
			free(lists);
			exit(1);
		}
	}

	bool* updateDeps = calloc(100 * 100, sizeof(bool));
	bool* updateDone = calloc(100, sizeof(bool));
	if (!updateDeps || !updateDone) {
		for (uint32_t i = 0; input[i]; i++) {
			free(input[i]);
		}
		for (uint32_t i = 0; i < listsCount; i++) {
			free(lists[i].IDs);
		}
		free(input);
		free(lists);
		free(updateDeps);
		free(updateDone);
		exit(1);
	}
	for (uint32_t i = 0; i < infosCount; i++) {
		uint8_t id = atoi(strchr(input[i], '|') + 1);
		uint8_t dep = atoi(input[i]);
		updateDeps[id * 100 + dep] = true;
		if (dep == 28 && id == 62) printf("fuck- %u\n", i);
	}

	uint32_t totalValidMiddle = 0;
	uint32_t totalFixedMiddle = 0;
	for (uint32_t i = 0; i < listsCount; i++) {
		bool validUpdate = true;
		for (uint32_t j = 0; j < lists[i].size; j++) {
			bool inOrder = true;
			for (uint32_t k = j + 1; k < lists[i].size; k++) {
				if (updateDeps[lists[i].IDs[j] * 100 + lists[i].IDs[k]]) {
					if (!updateDone[lists[i].IDs[k]]) {
						inOrder = false;
						break;
					}
				}
			}
			if (!inOrder) {
				validUpdate = false;
				break;
			}
			updateDone[lists[i].IDs[j]] = true;
		}
		memset(updateDone, false, sizeof(bool) * 100);
		if (validUpdate) {
			totalValidMiddle += lists[i].IDs[lists[i].size / 2];
		} else {
			FixUpdateList(lists[i], updateDeps, updateDone);
			totalFixedMiddle += lists[i].IDs[lists[i].size / 2];
		}
	}

	printf("Total valid updates middles sum : %u\n", totalValidMiddle);
	printf("Total fixed updates middles sum : %u\n", totalFixedMiddle);

	for (uint32_t i = 0; input[i]; i++) {
		free(input[i]);
	}
	for (uint32_t i = 0; i < listsCount; i++) {
		free(lists[i].IDs);
	}
	free(input);
	free(lists);
	free(updateDeps);
	free(updateDone);

	return 0;
}
