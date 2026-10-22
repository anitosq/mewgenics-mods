/* Pure predicates over the game's verified display metadata. */
#include <string.h>
/* Categories first, then the equipment slots from each item's "kind". */
enum { IQ_ALL, IQ_WEAPON_CONSUMABLES, IQ_ITEM_CONSUMABLES, IQ_WORN, IQ_BROKEN,
       IQ_REUSABLE_WEAPONS, IQ_HEAD, IQ_FACE, IQ_NECK, IQ_WEAPON, IQ_TRINKET, IQ_TYPE_COUNT };
enum { IQ_FIRST_SLOT_TYPE=IQ_HEAD };
/* slot: 0 unknown or other kind, then head, face, neck, weapon, trinket. */
typedef struct { int type; int rarity; } IQFilter;
typedef struct { int consumable, weapon, limited, condition, slot; } IQItemTraits;
static int iq_slot_index(const char* kind) {
    static const char* kinds[]={"head","face","neck","weapon","trinket"};
    for(int i=0;i<5;i++)if(kind && !strcmp(kind,kinds[i]))return i+1;
    return 0;
}
/* Item type popup: categories in the left column, slots in the right. */
static void iq_type_rect(int index,double* r) {
    int slot=index>=IQ_FIRST_SLOT_TYPE,row=slot?index-IQ_FIRST_SLOT_TYPE:index;
    r[0]=slot?280:10;r[1]=43+row*43;r[2]=260;r[3]=36;
}
static int iq_type_choice(double x,double y) {
    if(y<43)return -1;
    int row=(int)((y-43)/43),slot=x>=280 && x<540?1:x>=10 && x<270?0:-1;
    if(slot<0 || y-(43+row*43)>=36)return -1;
    int index=slot?IQ_FIRST_SLOT_TYPE+row:row;
    return index<(slot?IQ_TYPE_COUNT:IQ_FIRST_SLOT_TYPE)?index:-1;
}
static int iq_last_row(int items,int columns,int rows) {
    if(columns<=0 || rows<=0 || items<=columns*rows)return 0;
    return (items+columns-1)/columns-rows;
}
static int iq_max_row(int items,int columns) {return iq_last_row(items,columns,columns);}
static int iq_rarity_group(int rank) {
    switch(rank) {
        case 1: case 2:return 1;
        case 3: case 4:return 2;
        case 5: case 6:return 3;
        case 7: case 8:return 4;
        default:return 0;
    }
}
/* Popup geometry shared by hit-testing and hover feedback; -1 is a gap. */
static int iq_rarity_choice(double x,double y) {
    if(x>=10 && x<320 && y>=61 && y<61+36/0.70)return 0;
    if(x<10 || x>=320 || y<126 || y>=436)return -1;
    int col=(int)((x-10)/160),row=(int)((y-126)/160);
    if(x-(10+col*160)>=150 || y-(126+row*160)>=150)return -1;
    return 1+row*2+col;
}
static int iq_filter_match(IQFilter filter,IQItemTraits item,int rarity_rank) {
    int match=0;
    switch(filter.type) {
        case IQ_ALL:match=1;break;
        case IQ_WEAPON_CONSUMABLES:match=item.weapon==1 && item.limited==1;break;
        case IQ_ITEM_CONSUMABLES:match=item.consumable==1;break;
        case IQ_WORN:match=item.condition==3 || item.condition==4;break;
        case IQ_BROKEN:match=item.condition==5;break;
        case IQ_REUSABLE_WEAPONS:match=item.weapon==1 && item.limited==0;break;
        default:match=filter.type>=IQ_FIRST_SLOT_TYPE && item.slot==filter.type-IQ_FIRST_SLOT_TYPE+1;break;
    }
    return match && (!filter.rarity || iq_rarity_group(rarity_rank)==filter.rarity);
}
