#ifndef ID_US_H
#define ID_US_H

#include <stdbool.h>

// In ck_quit.c, as it may be customized by individual games.
void Quit(const char *msg);

// id_us_1.c: UI functions
void US_Print(const char *str);
void US_PrintF(const char *str, ...);
void US_CPrintLine(const char *str);
void US_CPrint(const char *str);
void US_CPrintF(const char *str, ...);
void US_ClearWindow();
void US_DrawWindow(int x, int y, int w, int h);
void US_CenterWindow(int w, int h);

void US_InitRndT(bool randomize);
int US_RndT();
void US_SetRndI(int index);
int US_GetRndI();



#endif //ID_US_H
