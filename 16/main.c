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

enum {
	DIR_UP = 0,
	DIR_RIGHT,
	DIR_DOWN,
	DIR_LEFT,
	DIR_COUNT,
};

typedef struct PathNode {
	struct PathNode* next;
	struct PathNode* pathPrev;
	uint32_t x;
	uint32_t y;
	uint32_t dir;
	uint32_t score;
	uint32_t pathLen;
} PathNode;

PathNode* CreateNode(uint32_t x, uint32_t y, uint32_t dir, uint32_t score, PathNode* prev) {
	PathNode* node = malloc(sizeof(PathNode));
	if (node) {
		*node = (PathNode){
			.next = NULL,
			.pathPrev = prev,
			.x = x,
			.y = y,
			.dir = dir,
			.score = score,
			.pathLen = 0,
		};
		if (prev) node->pathLen = prev->pathLen + 1;
	}
	return node;
}

void FreeNodes(PathNode* nodes) {
	while (nodes) {
		PathNode* tmp = nodes->next;
		free(nodes);
		nodes = tmp;
	}
}

bool FindPoint(char** map, uint32_t width, uint32_t height, char toSearch, uint32_t* xRef,
			   uint32_t* yRef) {
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			if (map[y][x] == toSearch) {
				*xRef = x;
				*yRef = y;
				return true;
			}
		}
	}
	return false;
}

#define GetNextX(x, dir)                                                                           \
	({                                                                                             \
		uint32_t res = x;                                                                          \
		uint32_t dir_ = dir;                                                                       \
		if (dir_ == DIR_LEFT) res--;                                                               \
		if (dir_ == DIR_RIGHT) res++;                                                              \
		res;                                                                                       \
	})

#define GetNextY(y, dir)                                                                           \
	({                                                                                             \
		uint32_t res = y;                                                                          \
		uint32_t dir_ = dir;                                                                       \
		if (dir_ == DIR_UP) res--;                                                                 \
		if (dir_ == DIR_DOWN) res++;                                                               \
		res;                                                                                       \
	})

uint32_t FindLowestScore(char** map, uint32_t width, uint32_t height) {
	uint32_t startX = 0;
	uint32_t startY = 0;
	if (!FindPoint(map, width, height, 'S', &startX, &startY)) {
		printf("%d\n", __LINE__);
		return UINT32_MAX;
	}

	uint32_t endX = 0;
	uint32_t endY = 0;
	if (!FindPoint(map, width, height, 'E', &endX, &endY)) {
		printf("%d\n", __LINE__);
		return UINT32_MAX;
	}

	uint32_t* scoreMap = malloc(width * height * sizeof(*scoreMap));
	if (!scoreMap) {
		printf("%d\n", __LINE__);
		return UINT32_MAX;
	}

	PathNode* curr = CreateNode(startX, startY, DIR_RIGHT, 0, NULL);
	if (!curr) {
		free(scoreMap);
		{
			printf("%d\n", __LINE__);
			return UINT32_MAX;
		}
	}

	PathNode* last = curr;

	uint32_t lowestScore = UINT32_MAX;

	memset(scoreMap, 0x55, width * height * sizeof(*scoreMap));

	uint32_t allocCount = 0;

	while (curr) {
		if (curr->x == endX && curr->y == endY) {
			if (curr->score < lowestScore) lowestScore = curr->score;
		} else if (abs((int32_t)scoreMap[curr->x + curr->y * width] - (int32_t)curr->score) <
					   1001 ||
				   scoreMap[curr->x + curr->y * width] > curr->score) {
			scoreMap[curr->x + curr->y * width] = curr->score;
			for (uint32_t i = 0; i < DIR_COUNT; i++) {
				uint32_t nextDir = (curr->dir + i) % DIR_COUNT;
				uint32_t nextX = GetNextX(curr->x, nextDir);
				uint32_t nextY = GetNextY(curr->y, nextDir);
				uint32_t nextScore = curr->score + 1;
				if (i > 0) nextScore += 1000;
				if (i == 2) nextScore += 1000;

				if (map[nextY][nextX] != '#') {
					allocCount++;
					last->next = CreateNode(nextX, nextY, nextDir, nextScore, curr);
					if (!last->next) {
						FreeNodes(curr);
						return UINT32_MAX;
					}
					last = last->next;
				}
			}
		}
		PathNode* tmp = curr->next;
		free(curr);
		curr = tmp;
	}

	return lowestScore;
}

void FindAllPaths(char** map, uint32_t width, uint32_t height, uint32_t expectedScore) {
	uint32_t startX = 0;
	uint32_t startY = 0;
	if (!FindPoint(map, width, height, 'S', &startX, &startY)) return;

	uint32_t endX = 0;
	uint32_t endY = 0;
	if (!FindPoint(map, width, height, 'E', &endX, &endY)) return;

	uint32_t* scoreMap = malloc(width * height * sizeof(*scoreMap));
	if (!scoreMap) return;

	PathNode* nodes = CreateNode(startX, startY, DIR_RIGHT, 0, NULL);
	if (!nodes) {
		free(scoreMap);
		return;
	}

	PathNode* curr = nodes;
	PathNode* last = nodes;
	memset(scoreMap, 0x55, width * height * sizeof(*scoreMap));

	while (curr) {
		if (abs((int32_t)scoreMap[curr->x + curr->y * width] - (int32_t)curr->score) < 1001 ||
			scoreMap[curr->x + curr->y * width] > curr->score) {
			scoreMap[curr->x + curr->y * width] = curr->score;
			for (uint32_t i = 0; i < DIR_COUNT; i++) {
				uint32_t nextDir = (curr->dir + i) % DIR_COUNT;
				uint32_t nextX = GetNextX(curr->x, nextDir);
				uint32_t nextY = GetNextY(curr->y, nextDir);
				uint32_t nextScore = curr->score + 1;
				if (i > 0) nextScore += 1000;
				if (i == 2) nextScore += 1000;

				if (map[nextY][nextX] != '#') {
					last->next = CreateNode(nextX, nextY, nextDir, nextScore, curr);
					if (!last->next) {
						FreeNodes(nodes);
						free(scoreMap);
						return;
					}
					last = last->next;
				}
			}
		}
		curr = curr->next;
	}

	curr = nodes;
	while (curr) {
		if (curr->x == endX && curr->y == endY && curr->score == expectedScore) {
			PathNode* tmp = curr;
			while (tmp) {
				map[tmp->y][tmp->x] = 'O';
				tmp = tmp->pathPrev;
			}
		}
		curr = curr->next;
	}

	FreeNodes(nodes);
	free(scoreMap);
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	uint32_t width = strlen(*input);
	uint32_t height = 0;
	while (input[height]) height++;

	uint32_t lowestScore = FindLowestScore(input, width, height);
	FindAllPaths(input, width, height, lowestScore);

	uint32_t totalPathLen = 0;
	for (uint32_t y = 0; y < height; y++) {
		printf("%s\n", input[y]);
		for (uint32_t x = 0; x < width; x++) {
			if (input[y][x] == 'O') totalPathLen++;
		}
	}
	printf("The lowest score is : %u\n", lowestScore);
	printf("The total path length is : %u\n", totalPathLen);

	for (uint32_t y = 0; y < height; y++) {
		free(input[y]);
	}
	free(input);

	return 0;
}
