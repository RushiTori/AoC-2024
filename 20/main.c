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

#define START_TILE 'S'
#define END_TILE 'E'
#define WALL_TILE '#'
#define EMPTY_TILE '.'

enum {
	DIR_UP = 0,
	DIR_RIGHT,
	DIR_DOWN,
	DIR_LEFT,
	DIR_COUNT,
};

typedef struct Vec {
	int32_t x;
	int32_t y;
} Vec;

Vec VecAddDir(Vec pos, uint32_t dir) {
	if (dir == DIR_UP) pos.y--;
	if (dir == DIR_RIGHT) pos.x++;
	if (dir == DIR_DOWN) pos.y++;
	if (dir == DIR_LEFT) pos.x--;
	return pos;
}

typedef struct Map {
	char** map;
	uint32_t* lenFromStart;
	uint32_t* lenFromEnd;

	int32_t width;
	int32_t height;

	Vec startPos;
	Vec endPos;

	uint32_t startDir;
	uint32_t endDir;

	uint32_t startToEndLen;
} Map;

#define VecEqual(a, b) ((a).x == (b).x && (a).y == (b).y)
#define MapGet(map, x, y) (map)->map[y][x]

#define MapGetVec(map, pos) (map)->map[(pos).y][(pos).x]

#define MapInBounds(map, x, y) ((x) >= 0 && (x) < (map)->width && (y) >= 0 && (y) < (map)->height)

#define MapInBoundsVec(map, pos) MapInBounds(map, (pos).x, (pos).y)

void InitPathsLen(Map* map, uint32_t* pathsLen, Vec pos, uint32_t dir) {
	memset(pathsLen, 0xFF, sizeof(uint32_t) * map->width * map->height);
	uint32_t currLen = 0;
	while (true) {
		pathsLen[pos.x + pos.y * map->width] = currLen++;
		bool foundNext = false;
		for (uint32_t i = 0; i < DIR_COUNT; i++) {
			Vec next = VecAddDir(pos, (dir + i) % DIR_COUNT);
			if (MapGetVec(map, next) != '#' &&
				pathsLen[next.x + next.y * map->width] == UINT32_MAX) {
				foundNext = true;
				pos = next;
				dir = (dir + i) % DIR_COUNT;
				break;
			}
		}
		if (!foundNext) break;
	}
}

Map InitMap(char** input) {
	Map map = {0};
	map.map = input;
	map.width = strlen(*input);
	map.height = 0;
	while (input[map.height]) map.height++;

	map.lenFromStart = calloc(map.width * map.height, sizeof(uint32_t));
	map.lenFromEnd = calloc(map.width * map.height, sizeof(uint32_t));
	if (!map.lenFromStart || !map.lenFromEnd) {
		free(map.lenFromStart);
		free(map.lenFromEnd);
		return (Map){0};
	}

	for (int32_t y = 0; y < map.height; y++) {
		for (int32_t x = 0; x < map.width; x++) {
			if (input[y][x] == START_TILE) {
				map.startPos.x = x;
				map.startPos.y = y;

				if (input[y - 1][x] == EMPTY_TILE) map.startDir = DIR_UP;
				if (input[y][x + 1] == EMPTY_TILE) map.startDir = DIR_RIGHT;
				if (input[y + 1][x] == EMPTY_TILE) map.startDir = DIR_DOWN;
				if (input[y][x - 1] == EMPTY_TILE) map.startDir = DIR_LEFT;
			} else if (input[y][x] == END_TILE) {
				map.endPos.x = x;
				map.endPos.y = y;

				if (input[y - 1][x] == EMPTY_TILE) map.endDir = DIR_UP;
				if (input[y][x + 1] == EMPTY_TILE) map.endDir = DIR_RIGHT;
				if (input[y + 1][x] == EMPTY_TILE) map.endDir = DIR_DOWN;
				if (input[y][x - 1] == EMPTY_TILE) map.endDir = DIR_LEFT;
			}
		}
	}
	InitPathsLen(&map, map.lenFromStart, map.startPos, map.startDir);
	InitPathsLen(&map, map.lenFromEnd, map.endPos, map.endDir);
	map.startToEndLen = map.lenFromStart[map.endPos.x + map.endPos.y * map.width];

	return map;
}

void FreeMap(Map* map) {
	for (int32_t y = 0; y < map->height; y++) {
		free(map->map[y]);
	}
	free(map->map);
	free(map->lenFromStart);
	free(map->lenFromEnd);
}

uint32_t CheckCheats(Map* map, int32_t x, int32_t y, uint32_t minCheatValue, int32_t maxCheatLen) {
	uint32_t count = 0;

	for (int32_t j = -maxCheatLen; j <= maxCheatLen; j++) {
		for (int32_t i = -maxCheatLen; i <= maxCheatLen; i++) {
			if (!i && !j) continue;
			int32_t cheatLen = abs(i) + abs(j);
			if (cheatLen > maxCheatLen) continue;

			if (!MapInBounds(map, x + i, y + j)) continue;
			if (MapGet(map, x + i, y + j) == '#') continue;

			uint32_t cheatPathLen = map->lenFromStart[x + y * map->width] + cheatLen +
									map->lenFromEnd[(x + i) + (y + j) * map->width];

			if (cheatPathLen > map->startToEndLen) continue;

			uint32_t cheatValue = map->startToEndLen - cheatPathLen;

			if (cheatValue >= minCheatValue) count++;
		}
	}

	return count;
}

uint32_t FindAllCheats(Map* map, uint32_t minCheatValue, int32_t maxCheatLen) {
	uint32_t count = 0;

	for (int32_t y = 0; y < map->height; y++) {
		for (int32_t x = 0; x < map->width; x++) {
			if (MapGet(map, x, y) != '#') {
				count += CheckCheats(map, x, y, minCheatValue, maxCheatLen);
			}
		}
	}

	return count;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	Map map = InitMap(input);
	if (!map.map) {
		for (uint32_t i = 0; input[i]; i++) {
			free(input[i]);
		}
		free(input);
		exit(1);
	}

	uint32_t cheatSimple = FindAllCheats(&map, 100, 2);
	uint32_t cheatHard = FindAllCheats(&map, 100, 20);

	printf("There are %u (simple) cheats that saves at least 100 picoseconds.\n", cheatSimple);
	printf("There are %u (hard) cheats that saves at least 100 picoseconds.\n", cheatHard);

	FreeMap(&map);

	return 0;
}
