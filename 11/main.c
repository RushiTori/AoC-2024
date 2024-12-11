#include <ctype.h>
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

uint64_t GetDigitsCount(uint64_t n) {
	if (n < 10ULL) return 1;
	if (n < 100ULL) return 2;
	if (n < 1000ULL) return 3;
	if (n < 10000ULL) return 4;
	if (n < 100000ULL) return 5;
	if (n < 1000000ULL) return 6;
	if (n < 10000000ULL) return 7;
	if (n < 100000000ULL) return 8;
	if (n < 1000000000ULL) return 9;
	if (n < 10000000000ULL) return 10;
	if (n < 100000000000ULL) return 11;
	if (n < 1000000000000ULL) return 12;
	if (n < 10000000000000ULL) return 13;
	if (n < 100000000000000ULL) return 14;
	if (n < 1000000000000000ULL) return 15;
	if (n < 10000000000000000ULL) return 16;
	if (n < 100000000000000000ULL) return 17;
	if (n < 1000000000000000000ULL) return 18;
	if (n < 10000000000000000000ULL) return 19;
	return 20;
	//      18 446 744 073 709 551 615
	//      18446744073709551615
	//      10000000000000000000
}

uint64_t GetPow10(uint64_t n) {
	static const uint64_t pow_10[] = {
		0ULL,
		10ULL,
		100ULL,
		1000ULL,
		10000ULL,
		100000ULL,
		1000000ULL,
		10000000ULL,
		100000000ULL,
		1000000000ULL,
		10000000000ULL,
		100000000000ULL,
		1000000000000ULL,
		10000000000000ULL,
		100000000000000ULL,
		1000000000000000ULL,
		10000000000000000ULL,
		100000000000000000ULL,
		1000000000000000000ULL,
		10000000000000000000ULL,
	};
	return pow_10[n];
}

typedef struct Stone {
	uint64_t num;
	uint64_t count;
} Stone;

Stone* ParseStones(const char* str, uint64_t* count) {
	if (!str || !count) return NULL;

	uint64_t tmp = 0;
	for (uint64_t i = 0; str[i]; i++) {
		if (isdigit(str[i])) {
			tmp++;
			while (isdigit(str[i + 1])) i++;
		}
	}

	Stone* stones = malloc(sizeof(Stone) * tmp);
	if (!stones) return NULL;

	*count = tmp;
	for (uint64_t i = 0, j = 0; str[i]; i++) {
		if (isdigit(str[i])) {
			stones[j++] = (Stone){.num = atoll(str + i), .count = 1};
			while (isdigit(str[i + 1])) i++;
		}
	}

	return stones;
}

int32_t CompareStone(const void* a, const void* b) {
	const Stone* a_ = (Stone*)a;
	const Stone* b_ = (Stone*)b;

	if (a_->num < b_->num) return -1;
	if (a_->num > b_->num) return 1;
	return 0;
}

uint64_t GetNextCount(const Stone* stones, uint64_t count) {
	uint64_t nextCount = count;
	for (uint64_t i = 0; i < count; i++) {
		if (GetDigitsCount(stones[i].num) % 2 == 0) nextCount++;
	}
	return nextCount;
}

Stone* Blink(const Stone* stones, uint64_t count, uint64_t* nextCount) {
	if (!nextCount) return NULL;
	if (!stones) return NULL;

	uint64_t tmp = GetNextCount(stones, count);
	if (!tmp) return NULL;

	Stone* next = malloc(sizeof(Stone) * tmp);
	if (!next) return NULL;
	*nextCount = tmp;

	for (uint64_t i = 0, j = 0; i < count; i++) {
		if (!stones[i].num) {
			next[j].num = 1;
		} else if (GetDigitsCount(stones[i].num) % 2 == 0) {
			uint64_t diviser = GetPow10(GetDigitsCount(stones[i].num) / 2);
			next[j].num = stones[i].num / diviser;
			next[j++].count = stones[i].count;
			next[j].num = stones[i].num % diviser;
		} else {
			next[j].num = stones[i].num * 2024;
		}
		next[j++].count = stones[i].count;
	}

	qsort(next, *nextCount, sizeof(Stone), CompareStone);

	uint64_t realCount = 0;

	for (uint32_t i = 1; i < *nextCount; i++) {
		if (next[i].num == next[realCount].num) {
			next[realCount].count += next[i].count;
		} else {
			next[++realCount] = next[i];
		}
	}

	*nextCount = realCount + 1;
	return next;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char* input = OpenInputAsBuffer(NULL);
	if (!input) exit(1);

	uint64_t count = 0;
	Stone* stones = ParseStones(input, &count);
	free(input);
	if (!stones) exit(1);

	for (uint32_t i = 0; i < 75; i++) {
		uint64_t nextCount = 0;
		Stone* nextStones = Blink(stones, count, &nextCount);
		if (!nextStones) {
			free(stones);
			exit(1);
		}
		free(stones);
		stones = nextStones;
		count = nextCount;

		uint64_t realStoneCount = 0;
		for (uint64_t j = 0; j < count; j++) {
			realStoneCount += stones[j].count;
		}
		if (i == 24 || i == 74) {
			printf("After %u blink there are now %lu stones (but only %lu distinct numbers)\n",
				   i + 1, realStoneCount, count);
		}
	}

	free(stones);

	return 0;
}
