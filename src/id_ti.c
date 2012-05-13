#include "id_ca.h"

#include <stdint.h>

// Tileinfo

//TODO: Split this up into several arrays? That would probably be _very_ smart.
uint8_t *ti_tileInfo;

int8_t TI_BackAnimTile(int tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16;
	return ti_tileInfo[index];
}

uint8_t TI_BackAnimTime(int tile)
{
	return ti_tileInfo[tile];
}

uint8_t TI_ForeTop(int tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16*2;
	return ti_tileInfo[index];
}

uint8_t TI_ForeBottom(int tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16*2 + ca_gfxInfoE.numTiles16m*2;
	return ti_tileInfo[index];
}

uint8_t TI_ForeLeft(int tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16*2 + ca_gfxInfoE.numTiles16m*3;
	return ti_tileInfo[index];
}

uint8_t TI_ForeRight(int tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16*2 + ca_gfxInfoE.numTiles16m;
	return ti_tileInfo[index];
}

uint8_t TI_ForeMisc(int tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16*2 + ca_gfxInfoE.numTiles16m*5;
	return ti_tileInfo[index];
}

int8_t TI_ForeAnimTile(int tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16*2 + ca_gfxInfoE.numTiles16m*4;
	return ti_tileInfo[index];
}

uint8_t TI_ForeAnimTime(int tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16*2 + ca_gfxInfoE.numTiles16m*6;
	return ti_tileInfo[index];
}


