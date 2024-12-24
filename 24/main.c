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

#define WIRE_COUNT ((35 * 36 * 36 + 35 * 36 + 35) + 1)

uint32_t HashWireLabel(char label[3]) {
	uint32_t a = ((label[0] - '0') > 9) ? (10 + (label[0] - 'a')) : (label[0] - '0');
	uint32_t b = ((label[1] - '0') > 9) ? (10 + (label[1] - 'a')) : (label[1] - '0');
	uint32_t c = ((label[2] - '0') > 9) ? (10 + (label[2] - 'a')) : (label[2] - '0');
	return a * 36 * 36 + b * 36 + c;
}

enum OP {
	OP_ERROR = 0,
	OP_ASSIGN,
	OP_AND,
	OP_OR,
	OP_XOR,
};

typedef struct Gate {
	uint8_t operation; // enum OP
	bool evaluated;
	uint16_t lhs;
	uint16_t rhs;
	uint16_t value;
} Gate;

struct Wire {
	Gate gate;
	char label[3];
	bool exists;
} wires[WIRE_COUNT];

void ParseWires(char** lines) {
	memset(wires, 0, sizeof(wires));

	while (*lines) {
		uint32_t wireA = HashWireLabel(*lines);
		if ((*lines)[3] == ':') {
			wires[wireA].gate.lhs = (*lines)[5] - '0';
			wires[wireA].gate.operation = OP_ASSIGN;
			wires[wireA].exists = true;

			memcpy(wires[wireA].label, *lines, 3);
		} else {
			uint32_t wireB = HashWireLabel(strrchr(*lines, '-') - 4);
			uint32_t wireC = HashWireLabel(strrchr(*lines, ' ') + 1);

			wires[wireC].gate.lhs = wireA;
			wires[wireC].gate.rhs = wireB;
			wires[wireC].exists = true;

			if ((*lines)[4] == 'A') wires[wireC].gate.operation = OP_AND;
			if ((*lines)[4] == 'O') wires[wireC].gate.operation = OP_OR;
			if ((*lines)[4] == 'X') wires[wireC].gate.operation = OP_XOR;
			memcpy(wires[wireC].label, strrchr(*lines, ' ') + 1, 3);
		}
		lines++;
	}
}

uint16_t EvaluateGate(Gate* gate);

uint16_t GetWireSignal(uint32_t wireID) {
	if (wireID >= WIRE_COUNT) {
		printf("Invalid wire\n");
		return 0;
	}
	if (!wires[wireID].exists) {
		printf("Wire ID #%u does not have a signal\n", wireID);
		return 0;
	}
	if (!wires[wireID].gate.evaluated) return EvaluateGate(&(wires[wireID].gate));
	return wires[wireID].gate.value;
}

uint16_t EvaluateGate(Gate* gate) {
	if (gate->evaluated) return gate->value;

	switch (gate->operation) {
		default:
		case OP_ERROR: printf("Unknown operation %d\n", gate->operation); break;
		case OP_ASSIGN: gate->value = gate->lhs; break;
		case OP_AND: gate->value = GetWireSignal(gate->lhs) & GetWireSignal(gate->rhs); break;
		case OP_OR: gate->value = GetWireSignal(gate->lhs) | GetWireSignal(gate->rhs); break;
		case OP_XOR: gate->value = GetWireSignal(gate->lhs) ^ GetWireSignal(gate->rhs); break;
	}
	gate->evaluated = true;

	return gate->value;
}

uint32_t GetNextXYZWire(uint32_t zHash) {
	if (zHash % 36 == 9) return zHash + 36 - 9;
	return zHash + 1;
}

uint64_t GetZValue() {
	uint64_t value = 0;
	uint64_t bit = 0;

	uint32_t hash = HashWireLabel("z00");

	while (wires[hash].exists) {
		// printf("%.3s", wires[hash].label);
		// printf(": %u\n", GetWireSignal(hash));
		value |= ((uint64_t)GetWireSignal(hash) & 1) << bit;
		bit++;
		hash = GetNextXYZWire(hash);
	}

	return value;
}

bool IsNumericLabel(char label[3], char wireName) {
	if (label[0] != wireName) return false;
	if (label[1] < '0' || label[1] > '9') return false;
	if (label[2] < '0' || label[2] > '9') return false;
	return true;
}

bool IsXYLabel(char label[3]) { return IsNumericLabel(label, 'x') || IsNumericLabel(label, 'y'); }

bool IsXYZLabel(char label[3]) { return IsXYLabel(label) || IsNumericLabel(label, 'z'); }

void PrintIncorrectWires() {
	/* Rules for correct wires, thanks to u/RazarTuk :
	   https://www.reddit.com/r/adventofcode/comments/1hl698z/comment/m3mwy38/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button

		1) All XOR gates must include x##, y##, or z##
		2) Except for z00, the output of x## XOR y## must be the input to another XOR gate
		3) Except for z45, no OR gates can have z## as an output
		4) No AND gates can have z## as an output
		5) Except for x00 AND y00, the output of an AND gate must be the input to an OR gate
	*/

	const uint16_t x00Hash = HashWireLabel("x00");
	const uint16_t y00Hash = HashWireLabel("y00");
	const uint16_t z00Hash = HashWireLabel("z00");
	const uint16_t z45Hash = HashWireLabel("z45");

	bool firstPrint = true;

	printf("Incorrect wires : ");

	for (uint32_t i = 0; i < WIRE_COUNT; i++) {
		if (!wires[i].exists) continue;

		bool isCorrect = true;
		Gate gate = wires[i].gate;

		if (gate.operation == OP_XOR) {
			if (!IsXYZLabel(wires[i].label) && !IsXYZLabel(wires[gate.lhs].label) &&
				!IsXYZLabel(wires[gate.rhs].label)) {
				isCorrect = false; // rule 1
			}

			if (IsXYLabel(wires[gate.lhs].label) && IsXYLabel(wires[gate.rhs].label)) {
				bool isInOfXOR = false;

				for (uint32_t j = 0; j < WIRE_COUNT; j++) {
					if (!wires[j].exists) continue;
					if (wires[j].gate.operation != OP_XOR) continue;
					if (wires[j].gate.lhs != i && wires[j].gate.rhs != i) continue;
					isInOfXOR = true;
					break;
				}

				if ((isInOfXOR && i == z00Hash) || (!isInOfXOR && i != z00Hash)) {
					isCorrect = false; // rule 2
				}
			}

		} else if (gate.operation == OP_OR) {
			if (IsNumericLabel(wires[i].label, 'z') && i != z45Hash) isCorrect = false; // rule 3
		} else if (gate.operation == OP_AND) {
			if (IsNumericLabel(wires[i].label, 'z')) isCorrect = false; // rule 4

			if (!((gate.lhs == x00Hash || gate.lhs == y00Hash) &&
				  (gate.rhs == x00Hash || gate.rhs == y00Hash))) {
				bool isInOfOR = false;
				for (uint32_t j = 0; j < WIRE_COUNT; j++) {
					if (!wires[j].exists) continue;
					if (wires[j].gate.operation != OP_OR) continue;
					if (wires[j].gate.lhs != i && wires[j].gate.rhs != i) continue;
					isInOfOR = true;
					break;
				}
				if (!isInOfOR) isCorrect = false; // rule 5
			}
		}

		if (!isCorrect) {
			if (!firstPrint) printf(",");
			firstPrint = false;
			printf("%.3s", wires[i].label);
		}
	}

	printf("\n");
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	ParseWires(input);

	for (uint32_t i = 0; i < WIRE_COUNT; i++) {
		if (wires[i].exists) GetWireSignal(i);
	}

	printf("Value of concatenated zxx wires : %lu\n", GetZValue());
	PrintIncorrectWires();

	for (uint32_t i = 0; input[i]; i++) {
		free(input[i]);
	}
	free(input);

	return 0;
}
