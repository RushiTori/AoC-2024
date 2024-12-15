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

#define MOVABLE_TILE 'O'
#define PLAYER_TILE '@'
#define WALL_TILE '#'
#define EMPTY_TILE '.'
#define BOX_LEFT_TILE '['
#define BOX_RIGHT_TILE ']'

#define MOVE_UP '^'
#define MOVE_RIGHT '>'
#define MOVE_DOWN 'v'
#define MOVE_LEFT '<'

typedef struct Sokoban {
	char** map;
	uint32_t width;
	uint32_t height;

	uint32_t pX;
	uint32_t pY;

	char* moves;
	uint32_t moveCount;
} Sokoban;

Sokoban ParseSokoban(char** lines) {
	if (!lines) return (Sokoban){0};
	Sokoban sokoban = {0};
	sokoban.map = lines;
	sokoban.width = strlen(*lines);
	sokoban.height = sokoban.width;

	for (uint32_t i = sokoban.height; lines[i]; i++) {
		sokoban.moveCount += strlen(lines[i]);
	}

	sokoban.moves = calloc(sokoban.moveCount + 1, sizeof(char));
	if (!sokoban.moves) return (Sokoban){0};
	for (uint32_t i = sokoban.height; lines[i]; i++) {
		strcat(sokoban.moves, lines[i]);
	}

	for (uint32_t y = 0; y < sokoban.height; y++) {
		bool foundPlayer = false;
		for (uint32_t x = 0; x < sokoban.width; x++) {
			if (sokoban.map[y][x] == PLAYER_TILE) {
				foundPlayer = true;
				sokoban.pX = x;
				sokoban.pY = y;
				break;
			}
		}
		if (foundPlayer) break;
	}

	return sokoban;
}

void MoveSokoban(Sokoban* game, char move) {
	int32_t dirX = 0;
	int32_t dirY = 0;

	if (move == MOVE_UP) dirY--;
	if (move == MOVE_DOWN) dirY++;
	if (move == MOVE_LEFT) dirX--;
	if (move == MOVE_RIGHT) dirX++;

	uint32_t nextX = game->pX + dirX;
	uint32_t nextY = game->pY + dirY;

	if (game->map[nextY][nextX] == WALL_TILE) return;
	if (game->map[nextY][nextX] == MOVABLE_TILE) {
		while (game->map[nextY][nextX] == MOVABLE_TILE) {
			nextX += dirX;
			nextY += dirY;
		}
		if (game->map[nextY][nextX] == WALL_TILE) return;
		game->map[nextY][nextX] = MOVABLE_TILE;
	}

	game->map[game->pY][game->pX] = EMPTY_TILE;
	game->pX += dirX;
	game->pY += dirY;
	game->map[game->pY][game->pX] = PLAYER_TILE;
}

uint64_t PlaySokoban(Sokoban* game) {
	for (uint32_t i = 0; i < game->moveCount; i++) {
		MoveSokoban(game, game->moves[i]);
	}

	uint64_t totalGPS = 0;
	uint64_t rowGPS = 0;
	for (uint32_t y = 0; y < game->height; y++) {
		for (uint32_t x = 0; x < game->width; x++) {
			if (game->map[y][x] == MOVABLE_TILE) {
				totalGPS += rowGPS + x;
			}
		}
		rowGPS += 100;
	}

	return totalGPS;
}

typedef struct WideSokoban {
	char* map;
	uint32_t width;
	uint32_t height;

	uint32_t pX;
	uint32_t pY;

	char* moves;
	uint32_t moveCount;
} WideSokoban;

#define WideSokobanGet(game, x, y) (game)->map[(x) + (y) * (game)->width]
#define WideSokobanIsBox(game, x, y)                                                               \
	(WideSokobanGet(game, x, y) == BOX_LEFT_TILE || WideSokobanGet(game, x, y) == BOX_RIGHT_TILE)

WideSokoban ParseWideSokoban(char** lines) {
	WideSokoban game = {0};
	game.width = strlen(*lines);
	game.height = game.width;

	game.map = malloc(sizeof(char) * game.width * 2 * game.height);
	if (!game.map) return (WideSokoban){0};

	uint32_t idx = 0;
	for (uint32_t y = 0; y < game.height; y++) {
		for (uint32_t x = 0; x < game.width; x++) {
			if (lines[y][x] == WALL_TILE) {
				game.map[idx++] = WALL_TILE;
				game.map[idx++] = WALL_TILE;
			} else if (lines[y][x] == MOVABLE_TILE) {
				game.map[idx++] = BOX_LEFT_TILE;
				game.map[idx++] = BOX_RIGHT_TILE;
			} else if (lines[y][x] == PLAYER_TILE) {
				game.pX = x * 2;
				game.pY = y;
				game.map[idx++] = PLAYER_TILE;
				game.map[idx++] = EMPTY_TILE;
			} else {
				game.map[idx++] = EMPTY_TILE;
				game.map[idx++] = EMPTY_TILE;
			}
		}
	}
	game.width *= 2;

	for (uint32_t i = game.height; lines[i]; i++) {
		game.moveCount += strlen(lines[i]);
	}

	game.moves = calloc(game.moveCount + 1, sizeof(char));
	if (!game.moves) {
		free(game.map);
		return (WideSokoban){0};
	}
	for (uint32_t i = game.height; lines[i]; i++) {
		strcat(game.moves, lines[i]);
	}

	return game;
}

void FreeWideSokoban(WideSokoban* game) {
	free(game->map);
	free(game->moves);
}

bool CanMoveBox(WideSokoban* game, uint32_t x, uint32_t y, char dir) {
	int32_t yDir = 0;
	if (dir == MOVE_UP) yDir--;
	if (dir == MOVE_DOWN) yDir++;

	int32_t pairOffset = 0;
	if (WideSokobanGet(game, x, y) == BOX_RIGHT_TILE) pairOffset--;
	if (WideSokobanGet(game, x, y) == BOX_LEFT_TILE) pairOffset++;

	if (WideSokobanGet(game, x, y + yDir) == WALL_TILE) return false;
	if (WideSokobanGet(game, x + pairOffset, y + yDir) == WALL_TILE) return false;

	if (WideSokobanIsBox(game, x, y + yDir)) {
		if (!CanMoveBox(game, x, y + yDir, dir)) return false;
	}
	if (WideSokobanIsBox(game, x + pairOffset, y + yDir)) {
		if (!CanMoveBox(game, x + pairOffset, y + yDir, dir)) return false;
	}

	return true;
}

bool TryMoveBox(WideSokoban* game, uint32_t x, uint32_t y, char dir) {
	if (dir == MOVE_LEFT || dir == MOVE_RIGHT) {
		int32_t xDir = 0;
		if (dir == MOVE_LEFT) xDir--;
		if (dir == MOVE_RIGHT) xDir++;

		uint32_t nextX = x;
		while (WideSokobanIsBox(game, nextX, y)) nextX += xDir;
		if (WideSokobanGet(game, nextX, y) == WALL_TILE) return false;

		while (nextX != x) {
			WideSokobanGet(game, nextX, y) = WideSokobanGet(game, nextX - xDir, y);
			nextX -= xDir;
		}
		WideSokobanGet(game, x, y) = EMPTY_TILE;
		return true;
	}

	if (!CanMoveBox(game, x, y, dir)) return false;

	int32_t yDir = 0;
	if (dir == MOVE_UP) yDir--;
	if (dir == MOVE_DOWN) yDir++;

	int32_t pairOffset = 0;
	if (WideSokobanGet(game, x, y) == BOX_RIGHT_TILE) pairOffset--;
	if (WideSokobanGet(game, x, y) == BOX_LEFT_TILE) pairOffset++;

	if (WideSokobanIsBox(game, x, y + yDir)) {
		TryMoveBox(game, x, y + yDir, dir);
	}
	if (WideSokobanIsBox(game, x + pairOffset, y + yDir)) {
		TryMoveBox(game, x + pairOffset, y + yDir, dir);
	}

	WideSokobanGet(game, x, y + yDir) = WideSokobanGet(game, x, y);
	WideSokobanGet(game, x, y) = EMPTY_TILE;

	WideSokobanGet(game, x + pairOffset, y + yDir) = WideSokobanGet(game, x + pairOffset, y);
	WideSokobanGet(game, x + pairOffset, y) = EMPTY_TILE;
	return true;
}

void MoveWideSokoban(WideSokoban* game, char move) {
	WideSokobanGet(game, game->pX, game->pY) = EMPTY_TILE;
	uint32_t nextX = game->pX;
	uint32_t nextY = game->pY;

	if (move == MOVE_UP) nextY--;
	if (move == MOVE_DOWN) nextY++;
	if (move == MOVE_LEFT) nextX--;
	if (move == MOVE_RIGHT) nextX++;

	bool canMove = true;

	if (WideSokobanGet(game, nextX, nextY) == WALL_TILE) canMove = false;
	if (WideSokobanIsBox(game, nextX, nextY)) canMove = TryMoveBox(game, nextX, nextY, move);
	if (canMove) {
		game->pX = nextX;
		game->pY = nextY;
	}
	WideSokobanGet(game, game->pX, game->pY) = PLAYER_TILE;
}

uint64_t PlayWideSokoban(WideSokoban* game) {
	for (uint32_t i = 0; i < game->moveCount; i++) {
		MoveWideSokoban(game, game->moves[i]);
	}

	uint64_t totalGPS = 0;
	for (uint32_t y = 0; y < game->height; y++) {
		for (uint32_t x = 0; x < game->width; x++) {
			if (WideSokobanGet(game, x, y) == BOX_LEFT_TILE) {
				totalGPS += y * 100 + x;
			}
		}
	}

	return totalGPS;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	Sokoban game = ParseSokoban(input);
	if (!game.map) {
		for (uint32_t i = 0; input[i]; i++) {
			free(input[i]);
		}
		free(input);
		exit(1);
	}

	WideSokoban wideGame = ParseWideSokoban(input);
	if (!wideGame.map) {
		for (uint32_t i = 0; input[i]; i++) {
			free(input[i]);
		}
		free(input);
		free(game.moves);
		exit(1);
	}

	uint64_t sokobanGPS = PlaySokoban(&game);
	uint64_t wideSokobanGPS = PlayWideSokoban(&wideGame);

	printf("After playing every moves the total GPS is : %lu\n", sokobanGPS);
	printf("After playing every moves the new total GPS is : %lu\n", wideSokobanGPS);

	for (uint32_t i = 0; input[i]; i++) {
		free(input[i]);
	}
	free(input);
	free(game.moves);
	FreeWideSokoban(&wideGame);

	return 0;
}
