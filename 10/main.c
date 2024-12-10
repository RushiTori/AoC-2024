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

typedef struct VecNode {
	struct VecNode* next;
	struct VecNode* prev;
	int32_t x;
	int32_t y;
} VecNode;

uint32_t GetNodesLen(VecNode* nodes) {
	uint32_t len = 0;

	while (nodes) {
		nodes = nodes->next;
		len++;
	}

	return len;
}

VecNode* CreateVecNode(int32_t x, int32_t y, VecNode* prev) {
	VecNode* node = malloc(sizeof(VecNode));
	if (!node) return NULL;

	*node = (VecNode){
		.next = NULL,
		.prev = prev,
		.x = x,
		.y = y,
	};

	return node;
}

void FreeVecNodes(VecNode* nodes) {
	while (nodes) {
		VecNode* tmp = nodes->next;
		free(nodes);
		nodes = tmp;
	}
}

VecNode* GetAllTrailStart(char** map, int32_t width, int32_t height) {
	VecNode* nodes = NULL;
	VecNode* res = NULL;

	for (int32_t y = 0; y < height; y++) {
		for (int32_t x = 0; x < width; x++) {
			if (map[y][x] == '0') {
				if (!nodes) {
					nodes = CreateVecNode(x, y, NULL);
					if (!nodes) return NULL;
					res = nodes;
				} else {
					nodes->next = CreateVecNode(x, y, nodes);
					if (!nodes->next) {
						FreeVecNodes(nodes);
						return NULL;
					}
					nodes = nodes->next;
				}
			}
		}
	}

	return res;
}

VecNode* RemoveDuplicates(VecNode* nodes) {
	VecNode* res = nodes;

	while (nodes) {
		VecNode* next = nodes->next;
		while (next) {
			if (next->x == nodes->x && next->y == nodes->y) {
				next->prev->next = next->next;
				if (next->next) next->next->prev = next->prev;
				VecNode* tmp = next->next;
				free(next);
				next = tmp;
			} else {
				next = next->next;
			}
		}
		nodes = nodes->next;
	}

	return res;
}

bool InBounds(int32_t x, int32_t y, int32_t width, int32_t height) {
	if (x < 0) return false;
	if (y < 0) return false;
	if (x >= width) return false;
	if (y >= height) return false;
	return true;
}

VecNode* TryAddVec(VecNode* last, int32_t x, int32_t y, char expected, char** map, int32_t width,
				   int32_t height) {
	if (InBounds(x, y, width, height)) {
		if (map[y][x] == expected) {
			last->next = CreateVecNode(x, y, last);
			last = last->next;
		}
	}
	return last;
}

VecNode* FinishTrail(VecNode* nodes, char** map, int32_t width, int32_t height) {
	VecNode* curr = CreateVecNode(nodes->x, nodes->y, NULL);

	VecNode* last = curr;

	while (curr) {
		char c = map[curr->y][curr->x];
		if (c == '9') break;

		c++;

		last = TryAddVec(last, curr->x, curr->y - 1, c, map, width, height);
		if (!last) {
			FreeVecNodes(curr);
			return NULL;
		}

		last = TryAddVec(last, curr->x + 1, curr->y, c, map, width, height);
		if (!last) {
			FreeVecNodes(curr);
			return NULL;
		}

		last = TryAddVec(last, curr->x, curr->y + 1, c, map, width, height);
		if (!last) {
			FreeVecNodes(curr);
			return NULL;
		}

		last = TryAddVec(last, curr->x - 1, curr->y, c, map, width, height);
		if (!last) {
			FreeVecNodes(curr);
			return NULL;
		}

		VecNode* tmp = curr->next;
		free(curr);
		curr = tmp;
	}

	return curr;
}

VecNode* FinishAllTrails(VecNode* nodes, char** map, int32_t width, int32_t height,
						 bool noDuplicate) {
	VecNode* res = NULL;
	VecNode* last = NULL;

	while (nodes) {
		VecNode* tmp = FinishTrail(nodes, map, width, height);
		if (noDuplicate) {
			tmp = RemoveDuplicates(tmp);
		}
		if (!res) {
			res = tmp;
			last = res;
		} else {
			last->next = tmp;
		}
		while (last->next) last = last->next;

		nodes = nodes->next;
	}

	return res;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	int32_t width = strlen(*input);
	int32_t height = 0;
	while (input[height]) height++;

	VecNode* nodes = GetAllTrailStart(input, width, height);
	if (!nodes) {
		for (int32_t y = 0; y < height; y++) {
			free(input[y]);
		}
		free(input);
		exit(1);
	}

	VecNode* trailEnds = FinishAllTrails(nodes, input, width, height, true);
	VecNode* allTrailEnds = FinishAllTrails(nodes, input, width, height, false);

	printf("The total trail head score is %u\n", GetNodesLen(trailEnds));
	printf("The total trail head score (with duplicates) is %u\n", GetNodesLen(allTrailEnds));

	FreeVecNodes(nodes);
	FreeVecNodes(trailEnds);
	FreeVecNodes(allTrailEnds);
	for (int32_t y = 0; y < height; y++) {
		free(input[y]);
	}
	free(input);

	return 0;
}
