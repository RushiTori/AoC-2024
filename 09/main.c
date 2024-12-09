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

void CompressFilesystem(int32_t* filesystem, uint32_t len) {
	uint32_t i = 0;
	uint32_t j = len - 1;

	while (i < j) {
		if (filesystem[j] == -1) {
			while (filesystem[j] == -1) j--;
			continue;
		}

		if (filesystem[i] != -1) {
			while (filesystem[i] != -1) i++;
			continue;
		}

		int32_t tmp = filesystem[i];
		filesystem[i] = filesystem[j];
		filesystem[j] = tmp;

		i++;
		j--;
	}
}

uint64_t CheckSum(int32_t* filesystem, uint32_t len) {
	uint64_t sum = 0;

	for (uint32_t i = 0; i < len; i++) {
		if (filesystem[i] == -1) continue;
		sum += filesystem[i] * i;
	}

	return sum;
}

typedef struct MemBlockInfo {
	int32_t fileID;
	uint32_t size;

	struct MemBlockInfo* prev;
	struct MemBlockInfo* next;
} MemBlockInfo;

MemBlockInfo* CreateBlock(int32_t fileID, uint32_t size, MemBlockInfo* prev) {
	MemBlockInfo* block = malloc(sizeof(MemBlockInfo));
	if (!block) return NULL;
	block->fileID = fileID;
	block->size = size;
	block->prev = prev;
	block->next = NULL;
	return block;
}

void FreeBlocks(MemBlockInfo* blocks) {
	MemBlockInfo* next = blocks->next;

	while (next) {
		free(blocks);
		blocks = next;
		next = blocks->next;
	}
	free(blocks);
}

MemBlockInfo* InitBlocks(const char* str) {
	MemBlockInfo* blocks = CreateBlock(0, *str - '0', NULL);
	if (!blocks) return NULL;

	str++;
	MemBlockInfo* curr = blocks;
	bool emptyBlock = true;

	while (*str) {

		int32_t nextID = ((emptyBlock) ? (-1) : (curr->prev->fileID + 1));
		curr->next = CreateBlock(nextID, *str - '0', curr);
		if (!curr->next) {
			FreeBlocks(blocks);
			return NULL;
		}

		curr = curr->next;
		str++;
		emptyBlock = !emptyBlock;
	}

	return blocks;
}

MemBlockInfo* RemoveEmptyBlocks(MemBlockInfo* blocks) {
	MemBlockInfo* tmp;

	while (blocks && blocks->size == 0) {
		tmp = blocks->next;
		free(blocks);
		blocks = tmp;
	}

	MemBlockInfo* curr = blocks;
	while (curr) {
		if (curr->size == 0) {
			curr->prev->next = curr->next;
			if (curr->next) curr->next->prev = curr->prev;

			tmp = curr->next;
			free(curr);
			curr = tmp;
			continue;
		}
		curr = curr->next;
	}

	return blocks;
}

MemBlockInfo* ReorderBlocks(MemBlockInfo* blocks) {
	if (blocks->fileID == -1) {
		blocks->prev = CreateBlock(0, 0, NULL);
		if (!blocks->prev) {
			FreeBlocks(blocks);
			return NULL;
		}
		blocks->prev->next = blocks;
		blocks = blocks->prev;
	}

	MemBlockInfo* last = blocks;
	while (last->next) last = last->next;

	while (last) {
		if (last->fileID == -1) {
			last = last->prev;
			continue;
		}

		MemBlockInfo* curr = blocks;
		while (curr != last) {
			if (curr->fileID == -1 && curr->size >= last->size) break;
			curr = curr->next;
		}

		if (curr == last) {
			last = last->prev;
			continue;
		}

		curr->fileID = last->fileID;

		if (last->size < curr->size) {
			uint32_t diff = curr->size - last->size;
			curr->size = last->size;
			if (curr->next->fileID == -1) {
				curr->next->size += diff;
			} else {
				MemBlockInfo* new = CreateBlock(-1, diff, curr);
				if (!new) {
					FreeBlocks(blocks);
					return NULL;
				}
				new->next = curr->next;
				curr->next->prev = new;
				curr->next = new;
			}
		}

		last->fileID = -1;
		while (last->prev->fileID == -1) last = last->prev;
		while (last->next && last->next->fileID == -1) {
			MemBlockInfo* tmp = last->next;
			last->next = tmp->next;
			last->size += tmp->size;
			free(tmp);
		}
		if (last->next) last->next->prev = last;
	}

	return RemoveEmptyBlocks(blocks);
}

void WriteBlocks(int32_t* filesystem, const MemBlockInfo* blocks) {
	uint32_t i = 0;

	while (blocks) {
		for (uint32_t j = 0; j < blocks->size; j++) {
			filesystem[i++] = blocks->fileID;
		}
		blocks = blocks->next;
	}
}

int32_t* InitFilesystem(const MemBlockInfo* blocks, uint32_t* lenRef) {
	uint32_t len = 0;
	const MemBlockInfo* curr = blocks;

	while (curr) {
		len += curr->size;
		curr = curr->next;
	}

	if (lenRef) *lenRef = len;

	return malloc(sizeof(int32_t) * len);
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	uint32_t inputLen = 0;
	char* input = OpenInputAsBuffer(&inputLen);
	if (!input) exit(1);
	{
		char* end = strchr(input, '\n');
		if (end) {
			inputLen = end - input;
			*end = '\0';
		}
	}

	MemBlockInfo* blocks = InitBlocks(input);
	free(input);
	if (!blocks) exit(1);
	blocks = RemoveEmptyBlocks(blocks);
	if (!blocks) exit(1);

	uint32_t len = 0;
	int32_t* filesystem = InitFilesystem(blocks, &len);
	if (!filesystem) {
		FreeBlocks(blocks);
		exit(1);
	}

	WriteBlocks(filesystem, blocks);
	CompressFilesystem(filesystem, len);
	uint64_t checksum = CheckSum(filesystem, len);

	blocks = ReorderBlocks(blocks);
	if (!blocks) {
		free(filesystem);
		exit(1);
	}

	WriteBlocks(filesystem, blocks);
	uint64_t checksumReorder = CheckSum(filesystem, len);

	printf("This filesystem's checksum is %lu\n", checksum);
	printf("This filesystem's (unfragmented) checksum is %lu\n", checksumReorder);

	FreeBlocks(blocks);
	free(filesystem);

	return 0;
}
