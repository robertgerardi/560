#ifndef REGION_H
#define REGION_H
#include <stdint.h>

typedef struct {
#if 0
	uint16_t /* unsigned int */ Start;
	uint16_t /* unsigned int */ End;
	char Name[24];
#else
	uint32_t /* unsigned int */ Start;
	uint32_t /* unsigned int */ End;
	char Name[24];
#endif
} REGION_T;

extern const REGION_T RegionTable[];
extern const unsigned NumProfileRegions;
extern volatile unsigned RegionCount[];
extern unsigned SortedRegions[];

#endif
