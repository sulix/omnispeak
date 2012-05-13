#ifndef ID_RF_H
#define ID_RF_H

#include <stdint.h>
#include <stdbool.h>

// Sprite Draw object
typedef struct RF_SpriteDrawEntry
{
	int chunk;
	int zLayer;
	int x, y;
	int sx, sy;
	int sw, sh;
	struct RF_SpriteDrawEntry **prevNextPtr;		// Pointer to the previous entry's 'next' pointer.
	struct RF_SpriteDrawEntry *next;
} RF_SpriteDrawEntry;


void RF_SetScrollBlock(int tileX, int tileY, bool vertical);
void RF_MarkTileGraphics();
void RF_Startup();
void RF_NewMap(int mapNum);
void RF_RenderTile16(int x, int y, int tile);
void RF_RenderTile16m(int x, int y, int tile);
void RF_ReplaceTiles(int16_t *tilePtr, int plane, int dstX, int dstY, int width, int height);
void RF_Reposition(int scrollXunit, int scrollYunit);
void RF_SmoothScroll(int scrollXdelta, int scrollYdelta);
void RF_RemoveSpriteDraw(RF_SpriteDrawEntry **drawEntry);
void RF_AddSpriteDraw(RF_SpriteDrawEntry **drawEntry, int unitX, int unitY, int chunk, bool allWhite, int zLayer);
void RF_Refresh();

#endif //ID_RF_H
