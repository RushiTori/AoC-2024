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

typedef struct Vec {
	int32_t x;
	int32_t y;
} Vec;

Vec ParseVec(char* str) {
	return (Vec){
		.x = atoi(str),
		.y = atoi(strchr(str, ',') + 1),
	};
}

Vec VecAdd(Vec vec, Vec add) {
	return (Vec){
		.x = vec.x + add.x,
		.y = vec.y + add.y,
	};
}

typedef struct VecNode {
	struct VecNode* next;
	Vec vec;
	uint32_t pathLen;
} VecNode;

VecNode* CreateVecNode(Vec vec, uint32_t len) {
	VecNode* node = malloc(sizeof(VecNode));
	if (!node) return NULL;
	node->next = NULL;
	node->vec = vec;
	node->pathLen = len;
	return node;
}

void FreeNodes(VecNode* nodes) {
	while (nodes) {
		VecNode* tmp = nodes->next;
		free(nodes);
		nodes = tmp;
	}
}

#define CORRUPTED_TILE '#'
#define VISITED_TILE '*'
#define FREE_TILE '.'

typedef struct Map {
	char* map;
	int32_t width;
	int32_t height;
} Map;

#define MapInBounds(map, vec)                                                                      \
	((vec).x >= 0 && (vec).x < (map).width && (vec).y >= 0 && (vec).y < (map).height)
#define MapGet(map, vec) (map).map[(vec).x + (vec).y * (map).width]
#define MapReset(map) memset((map).map, FREE_TILE, sizeof(char) * (map).width * (map).height)

Map CreateMap(int32_t width, int32_t height) {
	Map map = {0};
	map.map = malloc(sizeof(char) * width * height);
	if (!map.map) return (Map){0};
	MapReset(map);
	map.width = width;
	map.height = height;
	return map;
}

void FreeMap(Map map) { free(map.map); }

void ApplyBugs(Map map, Vec* bugs, uint32_t bugCount) {
	MapReset(map);
	for (uint32_t i = 0; i < bugCount; i++) {
		MapGet(map, bugs[i]) = CORRUPTED_TILE;
	}
}

VecNode* TryAddNode(VecNode* last, Vec pos, uint32_t len, Map map) {
	if (MapInBounds(map, pos) && MapGet(map, pos) == FREE_TILE) {
		MapGet(map, pos) = VISITED_TILE;
		last->next = CreateVecNode(pos, len);
		last = last->next;
	}
	return last;
}

uint32_t FindShortestPath(Map map) {
	VecNode* curr = CreateVecNode((Vec){0}, 0);
	if (!curr) return 0;
	MapGet(map, curr->vec) = VISITED_TILE;

	VecNode* last = curr;
	uint32_t shortest = UINT32_MAX;

	while (curr) {
		if (curr->vec.x == map.width - 1 && curr->vec.y == map.height - 1) {
			shortest = curr->pathLen;
			break;
		} else {
			last = TryAddNode(last, VecAdd(curr->vec, (Vec){0, -1}), curr->pathLen + 1, map);
			if (!last) {
				FreeNodes(curr);
				return 0;
			}
			last = TryAddNode(last, VecAdd(curr->vec, (Vec){1, 0}), curr->pathLen + 1, map);
			if (!last) {
				FreeNodes(curr);
				return 0;
			}
			last = TryAddNode(last, VecAdd(curr->vec, (Vec){0, 1}), curr->pathLen + 1, map);
			if (!last) {
				FreeNodes(curr);
				return 0;
			}
			last = TryAddNode(last, VecAdd(curr->vec, (Vec){-1, 0}), curr->pathLen + 1, map);
			if (!last) {
				FreeNodes(curr);
				return 0;
			}
		}
		VecNode* tmp = curr->next;
		free(curr);
		curr = tmp;
	}

	FreeNodes(curr);

	return shortest;
}

uint32_t FindFirstFatalBug(Map map, Vec* bugs, uint32_t bugCount) {
	uint32_t l = 1;
	uint32_t r = bugCount;
	uint32_t c = l + (r - l) / 2;

	while (l < r) {
		c = l + (r - l) / 2;

		ApplyBugs(map, bugs, c);
		uint32_t pathLen = FindShortestPath(map);

		if (pathLen == UINT32_MAX) {
			r = c - 1;
		} else {
			l = c + 1;
		}
	}

	ApplyBugs(map, bugs, c);
	uint32_t pathLen = FindShortestPath(map);

	if (pathLen == UINT32_MAX) return c - 1;
	return UINT32_MAX;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	uint32_t bugCount = 0;
	while (input[bugCount]) bugCount++;

	Vec* bugs = malloc(sizeof(Vec) * bugCount);
	if (!bugs) {
		for (uint32_t i = 0; i < bugCount; i++) {
			free(input[i]);
		}
		free(input);
		exit(1);
	}

	for (uint32_t i = 0; i < bugCount; i++) {
		bugs[i] = ParseVec(input[i]);
		free(input[i]);
	}
	free(input);

	Map map = CreateMap(71, 71);
	if (!map.map) {
		free(bugs);
		exit(1);
	}

	ApplyBugs(map, bugs, 1024);
	uint32_t shortestPath = FindShortestPath(map);
	uint32_t fatalBugIdx = FindFirstFatalBug(map, bugs, bugCount);
	printf("shortest : %u\n", shortestPath);
	if (fatalBugIdx >= bugCount) {
		printf("There are no fatal bugs !\n");
	} else {
		printf("fatal bug : #%u (%d, %d)\n", fatalBugIdx, bugs[fatalBugIdx].x, bugs[fatalBugIdx].y);
	}

	free(bugs);
	FreeMap(map);

	return 0;
}
