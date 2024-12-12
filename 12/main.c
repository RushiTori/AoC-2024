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

typedef struct Patch {
	char crop;
	uint32_t area;
	uint32_t perimeter;
	uint32_t sideCount;
} Patch;

typedef struct Map {
	char** data;
	bool* cache;
	int32_t width;
	int32_t height;
} Map;

#define IsMapValid(map) ((map) && (map)->data && (map)->cache)
#define MapGet(map, x, y) (map)->data[(y)][(x)]
#define MapGetCache(map, x, y) (map)->cache[(x) + (y) * (map)->width]
#define MapResetCache(map) memset((map)->cache, false, sizeof(bool) * (map)->width * (map)->height)
#define InBounds(map, x, y) ((x) >= 0 && (x) < (map)->width && (y) >= 0 && (y) < (map)->height)

Map CreateMap(char** mapData) {
	Map map = {0};

	map.width = strlen(*mapData);
	while (mapData[map.height]) map.height++;

	map.cache = malloc(sizeof(bool) * map.width * map.height);
	if (!map.cache) return (Map){0};

	map.data = mapData;
	return map;
}

void FreeMap(Map* map) {
	for (int32_t y = 0; y < map->height; y++) {
		free(map->data[y]);
	}
	free(map->data);
	free(map->cache);
	*map = (Map){0};
}

void FloodfillCache(Map* map, int32_t x, int32_t y, char crop) {
	if (!InBounds(map, x, y)) return;
	if (MapGetCache(map, x, y)) return;
	if (MapGet(map, x, y) != crop) return;

	MapGetCache(map, x, y) = true;
	FloodfillCache(map, x, y - 1, crop);
	FloodfillCache(map, x + 1, y, crop);
	FloodfillCache(map, x, y + 1, crop);
	FloodfillCache(map, x - 1, y, crop);
}

uint32_t GetPatchCount(Map* map) {
	if (!IsMapValid(map)) return 0;

	MapResetCache(map);

	uint32_t count = 0;
	for (int32_t y = 0; y < map->height; y++) {
		for (int32_t x = 0; x < map->width; x++) {
			if (MapGetCache(map, x, y)) continue;
			FloodfillCache(map, x, y, MapGet(map, x, y));
			count++;
		}
	}

	return count;
}

void FloodfillPatch(Patch* patch, Map* map, int32_t x, int32_t y, char crop) {
	if (MapGetCache(map, x, y)) return;

	MapGetCache(map, x, y) = true;
	patch->area++;
	patch->perimeter += 4;

	bool up = false;
	bool right = false;
	bool down = false;
	bool left = false;

	if (InBounds(map, x, y - 1) && MapGet(map, x, y - 1) == crop) {
		up = true;
		patch->perimeter--;
		FloodfillPatch(patch, map, x, y - 1, crop);
	}
	if (InBounds(map, x + 1, y) && MapGet(map, x + 1, y) == crop) {
		right = true;
		patch->perimeter--;
		FloodfillPatch(patch, map, x + 1, y, crop);
	}
	if (InBounds(map, x, y + 1) && MapGet(map, x, y + 1) == crop) {
		down = true;
		patch->perimeter--;
		FloodfillPatch(patch, map, x, y + 1, crop);
	}
	if (InBounds(map, x - 1, y) && MapGet(map, x - 1, y) == crop) {
		left = true;
		patch->perimeter--;
		FloodfillPatch(patch, map, x - 1, y, crop);
	}

	bool upLeft = InBounds(map, x - 1, y - 1) && (MapGet(map, x - 1, y - 1) == crop);
	bool upRight = InBounds(map, x + 1, y - 1) && (MapGet(map, x + 1, y - 1) == crop);
	bool downLeft = InBounds(map, x - 1, y + 1) && (MapGet(map, x - 1, y + 1) == crop);
	bool downRight = InBounds(map, x + 1, y + 1) && (MapGet(map, x + 1, y + 1) == crop);

	if (!up && !right) patch->sideCount++;
	if (!up && !left) patch->sideCount++;
	if (!down && !right) patch->sideCount++;
	if (!down && !left) patch->sideCount++;

	if (up && right && !upRight) patch->sideCount++;
	if (up && left && !upLeft) patch->sideCount++;
	if (down && right && !downRight) patch->sideCount++;
	if (down && left && !downLeft) patch->sideCount++;
}

void InitPatches(Patch* patches, Map* map) {
	if (!patches || !IsMapValid(map)) return;
	MapResetCache(map);

	uint32_t patchIdx = 0;
	for (int32_t y = 0; y < map->height; y++) {
		for (int32_t x = 0; x < map->width; x++) {
			if (MapGetCache(map, x, y)) continue;
			patches[patchIdx] = (Patch){0};
			patches[patchIdx].crop = MapGet(map, x, y);
			FloodfillPatch(patches + patchIdx, map, x, y, MapGet(map, x, y));
			patchIdx++;
		}
	}
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	Map map = CreateMap(input);
	if (!map.cache) {
		for (uint32_t i = 0; input[i]; i++) {
			free(input[i]);
		}
		free(input);
		exit(1);
	}

	uint32_t patchCount = GetPatchCount(&map);

	Patch* patches = malloc(sizeof(Patch) * patchCount);
	if (!patches) {
		FreeMap(&map);
		exit(1);
	}

	InitPatches(patches, &map);

	uint32_t totalFencePrice = 0;
	uint32_t totalBulkFencePrice = 0;
	for (uint32_t i = 0; i < patchCount; i++) {
		totalFencePrice += patches[i].perimeter * patches[i].area;
		totalBulkFencePrice += patches[i].sideCount * patches[i].area;
	}

	printf("The total price for fences is : %u\n", totalFencePrice);
	printf("The total (bulk) price for fences is : %u\n", totalBulkFencePrice);

	FreeMap(&map);
	free(patches);

	return 0;
}
