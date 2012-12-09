#ifndef CK_PHYS_H
#define CK_PHYS_H

typedef struct CK_objPhysData
{
	// Unit cliping box
	int unitX1;
	int unitY1;
	int unitX2;
	int unitY2;
	int unitXmid;


	// Tile clipping box
	int tileX1;
	int tileY1;
	int tileX2;
	int tileY2;
	int tileXmid;
} CK_objPhysData;


typedef struct CK_object CK_object;
typedef struct CK_action CK_action;

void CK_PhysUpdateNormalObj(CK_object *obj);
void CK_PhysUpdateSimpleObj(CK_object *obj);
void CK_PhysPushX(CK_object *pushee, CK_object *pusher);
void CK_PhysPushY(CK_object *pushee, CK_object *pusher);
void CK_SetAction(CK_object *obj, CK_action *act);
void CK_SetAction2(CK_object *obj, CK_action *act);
void CK_PhysGravityHigh(CK_object *obj);
void CK_PhysGravityMid(CK_object *obj);
void CK_PhysGravityLow(CK_object *obj);
void CK_PhysDampHorz(CK_object *obj);
void CK_PhysAccelHorz(CK_object *obj, int accX, int velLimit);

void CK_ResetClipRects(CK_object *obj);
#endif
