/* Pure predicates over the game's verified display metadata. */
enum { IQ_ALL, IQ_WEAPON_CONSUMABLES, IQ_ITEM_CONSUMABLES, IQ_WORN, IQ_BROKEN,
       IQ_REUSABLE_WEAPONS, IQ_TYPE_COUNT };
typedef struct { int type; int rarity; } IQFilter;
typedef struct { int consumable, weapon, limited, condition; } IQItemTraits;
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
    }
    return match && (!filter.rarity || iq_rarity_group(rarity_rank)==filter.rarity);
}
