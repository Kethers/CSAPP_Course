#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "cachelab.h"

#define ARCH_BITS 64U;

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long long ull;

uint g_verboseMode = 0;
uint g_setIndexBits = 0;
uint g_setNum = 0;
uint g_associativity = 0;
uint g_blockBits = 0;
uint g_blockSize = 0;
uint g_totalCacheSize = 0;
uint g_tagBits = 0;
char* g_traceFileName = NULL;
uint g_hitTimes = 0;
uint g_missTimes = 0;
uint g_evictionTimes = 0;

FILE* traceFile;

typedef struct cacheLine
{
	ull tag;
	struct cacheLine* prev;
	struct cacheLine* next;
	byte isValid;
} CacheLineStru;

typedef struct cacheSet
{
	CacheLineStru* cacheLines;
} CacheSetStru;

enum cacheHitStatus
{
	MISS = 0,
	HIT = 1,
	EVICTION
};

void GetOptsFromInput(int argc, char** argv)
{
	int opt;
	while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
	{
		switch (opt)
		{
		case 'h':
		{
			printf(
"Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n\
Options:\n\
  -h         Print this help message.\n\
  -v         Optional verbose flag.\n\
  -s <num>   Number of set index bits.\n\
  -E <num>   Number of lines per set.\n\
  -b <num>   Number of block offset bits.\n\
  -t <file>  Trace file.\n\
\n\
Examples:\n\
  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n\
  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
			exit(0);
			break;
		}

		case 'v':
		{
			g_verboseMode = 1;
			break;
		}
			
		case 's':
		{
			g_setIndexBits = atoi(optarg);
			g_setNum = 1 << g_setIndexBits;
			break;
		}

		case 'E':
		{
			g_associativity = atoi(optarg);
			break;
		}

		case 'b':
		{
			g_blockBits = atoi(optarg);
			g_blockSize = 1 << g_blockBits;
			break;
		}

		case 't':
		{
			g_traceFileName = optarg;
			if (!(traceFile = fopen(optarg, "r"))) {
				printf("%s: Error: Couldn't open %s\n", argv[0], optarg);
				exit(8);
			}
			break;
		}
		
		default:
			break;
		}
	}

	g_totalCacheSize = g_setNum * g_associativity * g_blockSize;
	g_tagBits = 64 - g_setIndexBits - g_blockBits;
}

uint IsCacheHit(CacheSetStru* cacheSet, uint tagIdx)
{
	for (CacheLineStru* cLine = cacheSet->cacheLines; cLine != NULL; cLine = cLine->next)
	{
		if (tagIdx != cLine->tag)
			continue;

		if (cLine->isValid)
		{
			return HIT;
		}
	}
	return MISS;
}

/**
 * @return 0 means no eviction and 1 means eviction
*/
uint UpdateCacheBlock(CacheSetStru* cacheSet, uint tagIdx)
{
	uint retVal = 1;
	CacheLineStru *targetLine = NULL, *tailLine = NULL;
	uint findInvalid = 0;
	for (CacheLineStru* cLine = cacheSet->cacheLines; cLine != NULL; cLine = cLine->next)
	{
		if (cLine->tag == tagIdx)	// Hit
		{
			targetLine = cLine;
			retVal = 0;
		}
		if (retVal && !findInvalid && cLine->isValid == 0)
		{
			targetLine = cLine;
			findInvalid = 1;
			retVal = 0;
		}
		if (cLine->next == NULL)
			tailLine = cLine;
	}

	// LRU update
	if (targetLine == NULL)		// Miss and has not empty line
		targetLine = cacheSet->cacheLines;
	targetLine->isValid = 1;
	targetLine->tag = tagIdx;

	if (targetLine->next != NULL)	// is not tail
	{
		targetLine->next->prev = targetLine->prev;
		if (targetLine->prev != NULL)	// is not head
		{
			targetLine->prev->next = targetLine->next;
		}
		else
		{
			cacheSet->cacheLines = targetLine->next;
		}
		targetLine->prev = tailLine;
		targetLine->next = NULL;
		tailLine->next = targetLine;
	}

	return retVal;
}

uint GetSetIndexFromAddr(ull addr)
{
	return (addr << g_tagBits) >> (g_tagBits + g_blockBits);
}

uint GetTagIndexFromAddr(ull addr)
{
	return addr >> (g_setIndexBits + g_blockBits);
}

void TestTraceFile(CacheSetStru* cacheSets)
{
	char cmd[100];
	while (fgets(cmd, 100, traceFile) != NULL)
	{
		if (cmd[0] != ' ')
			continue;
		
		size_t cmdLen = strlen(cmd);
		cmd[cmdLen-1] = '\0';
		if (g_verboseMode)
			printf("%s ", &cmd[1]);

		byte action;
		ull addr;
		sscanf(&cmd[1], "%c %llx", &action, &addr);
		
		uint setIdx = GetSetIndexFromAddr(addr);
		uint tagIdx = GetTagIndexFromAddr(addr);
		CacheSetStru* set = &cacheSets[setIdx];
		uint isHit = IsCacheHit(set, tagIdx);

		switch (action)
		{
		case 'L':
		case 'S':
		{
			uint hasEviction = UpdateCacheBlock(set, tagIdx);
			if (isHit)
			{
				++g_hitTimes;
				if (g_verboseMode)
					printf("hit\n");
			}
			else
			{
				++g_missTimes;
				g_evictionTimes += hasEviction;
				if (g_verboseMode)
					printf("miss%s\n", hasEviction ? " eviction" : "");
			}
			break;
		}

		case 'M':
		{
			uint hasEviction = UpdateCacheBlock(set, tagIdx);
			if (isHit)
			{
				g_hitTimes += 2;
				if (g_verboseMode)
					printf("hit hit\n");
			}
			else
			{
				++g_missTimes;
				++g_hitTimes;
				g_evictionTimes += hasEviction;
				if (g_verboseMode)
					printf("miss%s%s\n", hasEviction == 1 ? " eviction" : "", " hit");
			}
			break;
		}
		
		default:
			break;
		}
	}
}

void InitCache(CacheSetStru** cacheSets, CacheLineStru** cacheLines)
{
	CacheSetStru* cacheSetSpace = (CacheSetStru*)malloc(g_setNum * sizeof(CacheSetStru));
	*cacheSets = cacheSetSpace;
	CacheLineStru* cacheLinesSpace = (CacheLineStru*)malloc(g_setNum * g_associativity * sizeof(CacheLineStru));
	*cacheLines = cacheLinesSpace;
	CacheLineStru* cacheLineStart = cacheLinesSpace;

	for (size_t i = 0; i < g_setNum; i++, cacheLineStart += g_associativity)
	{
		cacheSetSpace[i].cacheLines = cacheLineStart;
		for (size_t j = 0; j < g_associativity; j++)
		{
			CacheLineStru* cLine = &(cacheSetSpace[i].cacheLines[j]);
			cLine->isValid = 0;
			cLine->tag = 0;
			cLine->next = NULL;

			if (j > 0)
			{
				(cLine - 1)->next = cLine;
				cLine->prev = (cLine - 1);
			}
			else
			{
				cLine->prev = NULL;
			}
		}
	}
}

void FreeCache(CacheSetStru** cacheSet, CacheLineStru** cacheLines)
{
	free(*cacheSet);
	*cacheSet = NULL;
	free(*cacheLines);
	*cacheLines = NULL;
}

int main(int argc, char* argv[])
{
	GetOptsFromInput(argc, argv);

	CacheLineStru* cacheLines = NULL;
	CacheSetStru* cacheSets = NULL;
	InitCache(&cacheSets, &cacheLines);
	TestTraceFile(cacheSets);
	printSummary(g_hitTimes, g_missTimes, g_evictionTimes);
	FreeCache(&cacheSets, &cacheLines);

	return 0;
}
