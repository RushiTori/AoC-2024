#include <math.h>
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

typedef struct CPU {
	uint64_t regA;
	uint64_t regB;
	uint64_t regC;
	uint64_t regPC;
	uint8_t* memory;
	uint64_t memorySize;
	uint64_t output;
	uint64_t outCalls;
	uint64_t quine;
} CPU;

CPU InitCPU(char** lines) {
	CPU cpu = {0};
	cpu.regA = atoi(strrchr(lines[0], ' ') + 1);
	cpu.regB = atoi(strrchr(lines[1], ' ') + 1);
	cpu.regC = atoi(strrchr(lines[2], ' ') + 1);

	cpu.memorySize++;
	for (uint32_t i = 0; lines[3][i]; i++) {
		if (lines[3][i] == ',') cpu.memorySize++;
	}

	cpu.memory = malloc(sizeof(uint8_t) * cpu.memorySize);
	if (!cpu.memory) return (CPU){0};

	for (char* ptr = strchr(lines[3], ' '); *ptr; ptr++) {
		ptr++;
		cpu.memory[cpu.regPC++] = *ptr - '0';
	}
	cpu.regPC = 0;

	return cpu;
}

void ResetCPU(CPU* cpu, uint64_t regA) {
	cpu->regA = regA;
	cpu->regB = 0;
	cpu->regC = 0;
	cpu->regPC = 0;
	cpu->output = 0;
	cpu->outCalls = 0;
}

void FreeCPU(CPU* cpu) { free(cpu->memory); }

void PrintCPUOutput(CPU* cpu) {
	char buffer[32] = {0};
	uint64_t num = cpu->output;
	buffer[0] = '0' + (num % 8);
	num /= 8;
	uint32_t idx = 1;
	while (num) {
		buffer[idx++] = ',';
		buffer[idx++] = '0' + (num % 8);
		num /= 8;
	}
	idx--;
	for (uint32_t i = 0; i < idx; i++, idx--) {
		char tmp = buffer[i];
		buffer[i] = buffer[idx];
		buffer[idx] = tmp;
	}
	printf("CPU output : %s\n", buffer);
}

uint64_t FetchLiteral(CPU* cpu) {
	if (cpu->regPC >= cpu->memorySize) return 0;
	return cpu->memory[cpu->regPC++];
}

uint64_t FetchCombo(CPU* cpu) {
	if (cpu->regPC >= cpu->memorySize) return 0;
	uint8_t combo = cpu->memory[cpu->regPC++];
	if (combo < 4) return combo;
	if (combo == 4) return cpu->regA;
	if (combo == 5) return cpu->regB;
	if (combo == 6) return cpu->regC;
	return 0;
}

void ADV(CPU* cpu) { cpu->regA /= 1 << FetchCombo(cpu); }

void BXL(CPU* cpu) { cpu->regB ^= FetchLiteral(cpu); }

void BST(CPU* cpu) { cpu->regB = FetchCombo(cpu) & 0b111; }

void JNZ(CPU* cpu) {
	uint64_t jmp = FetchLiteral(cpu);
	if (cpu->regA != 0) cpu->regPC = jmp;
}

void BXC(CPU* cpu) {
	FetchLiteral(cpu);
	cpu->regB = cpu->regB ^ cpu->regC;
}

void OUT(CPU* cpu) {
	cpu->outCalls++;
	cpu->output = cpu->output * 8 + FetchCombo(cpu) % 8;
}

void BDV(CPU* cpu) { cpu->regB = cpu->regA / (1 << FetchCombo(cpu)); }

void CDV(CPU* cpu) { cpu->regC = cpu->regA / (1 << FetchCombo(cpu)); }

void RunCPU(CPU* cpu) {
	while (cpu->regPC < cpu->memorySize) {
		switch (FetchLiteral(cpu)) {
			case 0: ADV(cpu); break;
			case 1: BXL(cpu); break;
			case 2: BST(cpu); break;
			case 3: JNZ(cpu); break;
			case 4: BXC(cpu); break;
			case 5: OUT(cpu); break;
			case 6: BDV(cpu); break;
			case 7: CDV(cpu); break;
			default: break;
		}
	}
}

uint64_t log8Mask(uint64_t p) {
	static const uint64_t masks[] = {
		0LLU,
		07LLU,
		077LLU,
		0777LLU,
		07777LLU,
		077777LLU,
		0777777LLU,
		07777777LLU,
		077777777LLU,
		0777777777LLU,
		07777777777LLU,
		077777777777LLU,
		0777777777777LLU,
		07777777777777LLU,
		077777777777777LLU,
		0777777777777777LLU,
		07777777777777777LLU,
		077777777777777777LLU,
		0777777777777777777LLU,
		07777777777777777777LLU,
		077777777777777777777LLU,
		0777777777777777777777LLU,
		(uint64_t)-1,
	};
	return masks[p];
}

bool FindQuine(CPU* cpu, uint64_t currA, uint64_t expected, uint64_t expectedCalls) {
	currA *= 8;

	for (uint8_t i = 0; i < 8; i++) {
		ResetCPU(cpu, currA);
		RunCPU(cpu);

		if (cpu->output == (expected & log8Mask(cpu->outCalls))) {
			if (cpu->outCalls == expectedCalls) {
				cpu->quine = currA;
				return true;
			} else if (FindQuine(cpu, currA, expected, expectedCalls)) {
				return true;
			}
		}

		currA++;
	}
	return false;
}

int32_t main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
	char** input = OpenInputAsStringArray();
	if (!input) exit(1);

	CPU cpu = InitCPU(input);
	for (uint32_t i = 0; input[i]; i++) {
		free(input[i]);
	}
	free(input);
	if (!cpu.memory) exit(1);

	RunCPU(&cpu);

	PrintCPUOutput(&cpu);

	uint64_t cpuInput = 0;
	for (uint32_t i = 0; i < cpu.memorySize; i++) {
		cpuInput = cpuInput * 8 + cpu.memory[i];
	}
	
	if (FindQuine(&cpu, 0, cpuInput, cpu.memorySize)) {
		printf("This program's quine is : %lu\n", cpu.quine);
	} else {
		printf("Couldn't find a quine for this program !\n");
	}
	FreeCPU(&cpu);

	return 0;
}
