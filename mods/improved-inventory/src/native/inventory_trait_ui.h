/* Search-row and Stats/Effects popup geometry, shared by hit testing and hover
 * feedback (generated controls in tools/build_filter_assets.py). */
#pragma once
#include "inventory_trait_match.h"
enum { IQ_STAT_CHOICES=1+2*IQ_STAT_COUNT };
#define IQ_SEARCH_WIDTH 315
#define IQ_SEARCH_CLEAR_X 277
#define IQ_FXBAR_STATS_X 323
#define IQ_FXBAR_EFFECTS_X 451
#define IQ_FXBAR_WIDTH 120
#define IQ_STATS_POPUP_W 330
#define IQ_STATS_POPUP_H 397
#define IQ_EFFECTS_POPUP_W 430
#define IQ_EFFECTS_POPUP_H 456
enum { IQ_EFFECT_GAP=-1, IQ_EFFECT_CLEAR=-2, IQ_EFFECT_DONE=-3 };
/* Stats popup choice at a local point: 0 Any, 1..7 bonus, 8..14 malus, -1 gap. */
static int iq_stat_choice(double x,double y) {
    if(x>=10 && x<320 && y>=43 && y<79)return 0;
    if(y<86 || y>=86+IQ_STAT_COUNT*43)return -1;
    int row=(int)((y-86)/43);
    if(y-(86+row*43)>=36)return -1;
    if(x>=10 && x<160)return 1+row;
    if(x>=170 && x<320)return 1+IQ_STAT_COUNT+row;
    return -1;
}
/* Feedback rectangle order matches the generated sprite: Any, then each row's
   bonus and malus buttons. */
static void iq_stat_rect(int index,double* r) {
    if(!index){r[0]=10;r[1]=43;r[2]=310;r[3]=36;return;}
    int row=(index-1)/2,column=(index-1)%2;
    r[0]=column?170:10;r[1]=86+row*43;r[2]=150;r[3]=36;
}
/* Effects popup: toggles in reading order (two columns), Clear and Done. */
static int iq_effect_choice(double x,double y) {
    if(y>=410 && y<446)return x>=10 && x<100?IQ_EFFECT_CLEAR:x>=330 && x<420?IQ_EFFECT_DONE:IQ_EFFECT_GAP;
    if(y<43)return IQ_EFFECT_GAP;
    int row=(int)((y-43)/40),column=x>=10 && x<210?0:x>=220 && x<420?1:-1;
    if(column<0 || y-(43+row*40)>=36)return IQ_EFFECT_GAP;
    int index=row*2+column;
    return index<IQ_EFFECT_COUNT?index:IQ_EFFECT_GAP;
}
static void iq_effect_rect(int index,double* r) {
    if(index>=IQ_EFFECT_COUNT){r[0]=index==IQ_EFFECT_COUNT?10:330;r[1]=410;r[2]=90;r[3]=36;return;}
    r[0]=10+(index%2)*210;r[1]=43+(index/2)*40;r[2]=200;r[3]=36;
}
