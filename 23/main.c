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

#define NODE_HASH_COUNT ((('z' - 'a') * 26 + ('z' - 'a')) + 1)
#define HashNode(node) (((node)[0] - 'a') * 26 + ((node)[1] - 'a'))

void ReverseNodeHash(char* buffer, uint32_t hash) {
	buffer[0] = 'a' + (hash / 26);
	buffer[1] = 'a' + (hash % 26);
}

uint32_t GetTripletCount(bool* nodeLinks) {
	uint32_t tripletCount = 0;

	const uint32_t tStart = HashNode("ta");
	const uint32_t tEnd = HashNode("tz");

	for (uint32_t i = tStart; i <= tEnd; i++) {
		for (uint32_t j = 0; j < NODE_HASH_COUNT; j++) {
			if (j == tStart) j = i + 1;
			if (!nodeLinks[i + j * NODE_HASH_COUNT]) continue;

			for (uint32_t k = j + 1; k < NODE_HASH_COUNT; k++) {
				if (k == tStart) {
					k = i + 1;
					if (k <= j) k = j + 1;
				}

				if (!nodeLinks[i + k * NODE_HASH_COUNT]) continue;
				if (!nodeLinks[j + k * NODE_HASH_COUNT]) continue;
				tripletCount++;
			}
		}
	}

	return tripletCount;
}

typedef struct VertexNode {
	struct VertexNode* next;
	struct VertexNode* prev;
	char name[2];
	uint16_t hash;
} VertexNode;

VertexNode* CreateVertexNode(char name[2], VertexNode* prev) {
	VertexNode* node = malloc(sizeof(VertexNode));
	if (!node) return NULL;
	node->next = NULL;
	node->prev = prev;
	*((uint16_t*)(node->name)) = *((uint16_t*)(name));
	node->hash = HashNode(name);
	return node;
}

void FreeVertexNodes(VertexNode* nodes) {
	while (nodes) {
		VertexNode* next = nodes->next;
		free(nodes);
		nodes = next;
	}
}

VertexNode* CopyVertexNodes(VertexNode* nodes) {
	if (!nodes) return NULL;
	VertexNode* copy = CreateVertexNode(nodes->name, NULL);
	nodes = nodes->next;

	VertexNode* curr = copy;
	while (nodes) {
		curr->next = CreateVertexNode(nodes->name, curr);
		if (!curr->next) {
			FreeVertexNodes(copy);
			return NULL;
		}
		nodes = nodes->next;
		curr = curr->next;
	}

	return copy;
}

bool ContainsVertexNodes(VertexNode* nodes, VertexNode* toFind) {
	if (!toFind) return false;
	while (nodes) {
		if (nodes->hash == toFind->hash) return true;
		nodes = nodes->next;
	}
	return false;
}

VertexNode* EraseVertexNodes(VertexNode* nodes, VertexNode* toErase) {
	if (!toErase) return nodes;
	if (!nodes) return NULL;

	VertexNode* res = NULL;

	uint32_t eraseHash = toErase->hash;

	while (nodes) {
		if (nodes->hash == eraseHash) {
			if (nodes->next) nodes->next->prev = nodes->prev;
			if (nodes->prev) nodes->prev->next = nodes->next;
			VertexNode* next = nodes->next;
			free(nodes);
			nodes = next;
			continue;
		}
		if (!res) res = nodes;
		nodes = nodes->next;
	}

	return res;
}

VertexNode* EraseNonLinkedVertexNodes(VertexNode* nodes, VertexNode* toErase, bool* nodeLinks) {
	if (!toErase) return nodes;
	if (!nodes) return NULL;

	VertexNode* res = NULL;

	while (nodes) {
		if (!nodeLinks[nodes->hash + toErase->hash * NODE_HASH_COUNT]) {
			if (nodes->next) nodes->next->prev = nodes->prev;
			if (nodes->prev) nodes->prev->next = nodes->next;
			VertexNode* next = nodes->next;
			free(nodes);
			nodes = next;
			continue;
		}
		if (!res) res = nodes;
		nodes = nodes->next;
	}

	return res;
}

VertexNode* AddVertexNodes(VertexNode* nodes, VertexNode* toAdd) {
	if (!toAdd) return nodes;
	if (!nodes) return CreateVertexNode(toAdd->name, NULL);

	VertexNode* last = nodes;
	while (last->next) last = last->next;
	last->next = CreateVertexNode(toAdd->name, last);

	return nodes;
}

void PrintVertexNodes(VertexNode* nodes) {
	while (nodes) {
		putchar(nodes->name[0]);
		putchar(nodes->name[1]);
		if (nodes->next) putchar(',');
		nodes = nodes->next;
	}
	putchar('\n');
}

typedef struct CliqueNode {
	struct CliqueNode* next;
	VertexNode* clique;
	uint32_t cliqueLen;
} CliqueNode;

CliqueNode* AddClique(CliqueNode* nodes, VertexNode* clique) {
	CliqueNode* node = malloc(sizeof(CliqueNode));
	if (!node) return nodes;

	node->next = NULL;
	node->clique = clique;
	node->cliqueLen = 0;
	while (clique) {
		node->cliqueLen++;
		clique = clique->next;
	}

	if (!nodes) return node;

	CliqueNode* last = nodes;
	while (last->next) last = last->next;
	last->next = node;

	return nodes;
}

// Bron–Kerbosch algorithm : Without pivoting
// https://en.wikipedia.org/wiki/Bron-Kerbosch_algorithm

// algorithm BronKerbosch1(R, P, X) is
//  if P and X are both empty then
//      report R as a maximal clique
//  for each vertex v in P do
//      BronKerbosch1(R ⋃ {v}, P ⋂ N(v), X ⋂ N(v))
//      P := P \ {v}
//      X := X ⋃ {v}
void GetAllCliques(bool* nodeLinks, VertexNode* R, VertexNode* P, VertexNode* X,
				   CliqueNode** cliques) {
	if (!P && !X) {
		*cliques = AddClique(*cliques, R);
		return;
	}

	while (P) {
		VertexNode* Rcopy = AddVertexNodes(CopyVertexNodes(R), P);
		VertexNode* Pcopy = EraseNonLinkedVertexNodes(CopyVertexNodes(P), P, nodeLinks);
		VertexNode* Xcopy = EraseNonLinkedVertexNodes(CopyVertexNodes(X), P, nodeLinks);

		GetAllCliques(nodeLinks, Rcopy, Pcopy, Xcopy, cliques);

		X = AddVertexNodes(X, P);
		P = EraseVertexNodes(P, P);
	}

	FreeVertexNodes(R);
	FreeVertexNodes(X);
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	bool* nodeLinks = calloc(NODE_HASH_COUNT * NODE_HASH_COUNT, sizeof(bool));
	if (!nodeLinks) exit(1);

	char** input = OpenInputAsStringArray();
	if (!input) {
		free(nodeLinks);
		exit(1);
	}

	for (uint32_t i = 0; input[i]; i++) {
		uint32_t x = HashNode(input[i]);
		uint32_t y = HashNode(input[i] + 3);

		nodeLinks[x + y * NODE_HASH_COUNT] = true;
		nodeLinks[y + x * NODE_HASH_COUNT] = true;

		free(input[i]);
	}
	free(input);

	VertexNode* vertices = NULL;
	for (uint32_t i = 0; i < NODE_HASH_COUNT; i++) {
		for (uint32_t j = 0; j < NODE_HASH_COUNT; j++) {
			if (nodeLinks[i + j * NODE_HASH_COUNT]) {
				VertexNode tmp = (VertexNode){0};
				tmp.hash = i;
				ReverseNodeHash(tmp.name, i);
				vertices = AddVertexNodes(vertices, &tmp);
				break;
			}
		}
	}

	if (!vertices) {
		free(nodeLinks);
		exit(1);
	}

	CliqueNode* cliques = NULL;
	GetAllCliques(nodeLinks, NULL, vertices, NULL, &cliques);

	if (!cliques) {
		free(nodeLinks);
		exit(1);
	}

	uint32_t largestCliqueLen = 0;
	CliqueNode* curr = cliques;
	while (curr) {
		if (curr->cliqueLen > largestCliqueLen) largestCliqueLen = curr->cliqueLen;
		curr = curr->next;
	}

	uint32_t tripletCount = GetTripletCount(nodeLinks);

	printf("Total triplets count %u\n", tripletCount);
	printf("Largest cliques :\n");

	curr = cliques;
	while (curr) {
		if (curr->cliqueLen == largestCliqueLen) {
			printf("  ");
			PrintVertexNodes(curr->clique);
		}
		CliqueNode* next = curr->next;
		FreeVertexNodes(curr->clique);
		free(curr);
		curr = next;
	}

	free(nodeLinks);
	return 0;
}
