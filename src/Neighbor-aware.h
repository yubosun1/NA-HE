#pragma once
#include "common.h" 
#include <stdint.h>
#define MAX(x,y) (x>y?x:y)

bitmask_t en_neibourAware(bitmask_t &prevCode, int &iterStartPos, halfmask_t GridX, halfmask_t GridY, halfmask_t nextX, halfmask_t nextY, int k);
void de_neibourAware(halfmask_t &prevLon, halfmask_t &prevLat, int &iterStartPos, halfmask_t &currentLon, halfmask_t &currentLat, bitmask_t currentCode, bitmask_t nextCode, int k);

extern int en_update_number;
extern int en_coding_number;
extern int de_update_number;
extern int de_coding_number;
