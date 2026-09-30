#include "solver.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
int main(void) {
    AFProblem* p=calloc(1,sizeof(*p));AFSearch* s=calloc(1,sizeof(*s));assert(p&&s);
    p->selected=7;
    double a[AF_STATS]={90,45,30,0},b[AF_STATS]={60,45,30,999},c[AF_STATS]={60,50,40,0};
    assert(af_compare(p,a,b)>0&&af_compare(p,c,a)>0);
    p->targets=1;p->minimum[0]=80;
    assert(af_compare(p,a,c)>0);
    p->minimum[0]=0;assert(af_compare(p,c,a)>0);
    p->width=4;p->height=3;p->count=3;p->selected=3;p->targets=0;
    for(int x=0;x<4;x++)p->grid[x]=2;
    for(int i=0;i<3;i++) {
        p->shape[i].count=2;p->shape[i].solid=1;
        p->shape[i].cells[0]=(AFCell){0,0,3};p->shape[i].cells[1]=(AFCell){0,1,1};
    }
    p->shape[0].stats[0]=10;p->shape[1].stats[1]=5;p->shape[2].stats[1]=4;
    assert(af_start(p,s,NULL));
    for(int i=0;i<30;i++)af_step(p,s);
    assert(s->best.used==3&&s->best.stats[0]==10&&s->best.stats[1]==9);
    assert(af_rebuild(p,&s->best,s->grid,0));
    assert(!af_fits(p,p->grid,&p->shape[0],(AFPlacement){0,1,1,1,1}));
    assert(!af_fits(p,p->grid,&p->shape[0],(AFPlacement){0,0,1,-1,1}));
    p->grid[4]=6;assert(!af_fits(p,p->grid,&p->shape[0],(AFPlacement){0,0,1,1,1}));
    p->grid[4]=1;p->shape[0].cells[1].value=5;
    assert(af_fits(p,p->grid,&p->shape[0],(AFPlacement){0,0,1,1,1}));
    p->grid[4]=2;assert(!af_fits(p,p->grid,&p->shape[0],(AFPlacement){0,0,1,1,1}));
    AFLayout pinned={0};unsigned char fixed[AF_ITEMS]={0};
    p->shape[0].count=p->shape[1].count=p->shape[2].count=1;
    p->shape[0].cells[0]=(AFCell){0,0,3};p->shape[1].cells[0]=(AFCell){0,0,2};
    p->shape[2].cells[0]=(AFCell){0,0,1};
    pinned.placement[0]=pinned.placement[1]=(AFPlacement){1,1,1,1,1};
    pinned.placement[2]=(AFPlacement){2,1,1,1,1};fixed[0]=1;
    af_fix_dependencies(p,&pinned,fixed);assert(fixed[1]&&!fixed[2]);
    p->minimum[0]=NAN;assert(!af_start(p,s,NULL));
    memset(p,0,sizeof(*p));p->width=2;p->height=2;p->count=3;p->selected=3;p->targets=1;p->minimum[0]=16;
    p->grid[0]=p->grid[1]=2;
    for(int i=0;i<3;i++) {
        p->shape[i].count=2;p->shape[i].solid=1;
        p->shape[i].cells[0]=(AFCell){0,0,3};p->shape[i].cells[1]=(AFCell){0,1,1};
        p->shape[i].stats[0]=i?8:20;p->shape[i].stats[1]=i?8:0;
    }
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.stats[0]==16&&s->best.stats[1]==16);
    p->targets=3;p->minimum[0]=p->minimum[1]=100;
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.stats[0]==16&&s->best.stats[1]==16);
    memset(p,0,sizeof(*p));p->width=4;p->height=2;p->count=4;p->selected=3;
    for(int x=0;x<4;x++)p->grid[x]=2;
    for(int i=0;i<4;i++) {
        p->shape[i].count=2;p->shape[i].solid=1;
        p->shape[i].cells[0]=(AFCell){0,0,3};p->shape[i].cells[1]=(AFCell){0,1,1};
        p->shape[i].stats[i]=5;
    }
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.used==4);
    for(int i=0;i<4;i++)assert(s->best.stats[i]==5);
    double filled[AF_STATS]={5,5,5,5},sacrifice[AF_STATS]={4,5,100,100},empty[AF_STATS]={5,5,0,0};
    assert(af_compare(p,filled,empty)>0&&af_compare(p,filled,sacrifice)>0);
    assert(af_rebuild(p,&s->best,s->grid,0));
    double value;
    assert(af_parse_bound("",&value)==0&&af_parse_bound("0",&value)==1&&value==0);
    assert(af_parse_bound("12.5",&value)==1&&value==12.5);
    assert(af_parse_bound(".",&value)==-1&&af_parse_bound("-1",&value)==-1);
    assert(af_parse_bound("nan",&value)==-1&&af_parse_bound("inf",&value)==-1);
    assert(af_parse_bound("1000001",&value)==-1&&af_parse_bound("5junk",&value)==-1);
    assert(af_correct_max(10,5)==11&&af_correct_max(10,10)==11&&af_correct_max(0,0)==1);
    assert(af_correct_max(10,12)==12&&af_correct_max(999999,0)==1000000);
    p->caps=1;p->maximum[0]=0;
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.stats[0]==0&&s->best.stats[1]==5&&af_within_caps(p,s->best.stats));
    p->caps=0;assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.stats[0]==5);
    p->caps=1;p->maximum[0]=4;p->targets=1;p->minimum[0]=3;
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.stats[0]==0&&af_within_caps(p,s->best.stats));
    p->base[0]=6;assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(!af_within_caps(p,s->best.stats));
    p->maximum[0]=3;assert(!af_start(p,s,NULL));
    p->maximum[0]=NAN;assert(!af_start(p,s,NULL));
    p->maximum[0]=4;p->caps=4;assert(!af_start(p,s,NULL));
    memset(p,0,sizeof(*p));p->width=1;p->height=1;p->count=2;p->selected=16;
    for(int i=0;i<2;i++) {
        p->shape[i].count=p->shape[i].solid=1;p->shape[i].cells[0]=(AFCell){0,0,1};
    }
    p->shape[0].stats[4]=8;p->shape[1].stats[0]=100;p->shape[1].stats[4]=3;
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.stats[4]==8&&s->best.stats[0]==0);
    p->targets=p->caps=16;p->minimum[4]=2;p->maximum[4]=4;
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.stats[4]==3&&af_within_caps(p,s->best.stats));
    p->base[4]=5;assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(!af_within_caps(p,s->best.stats));
    p->base[4]=NAN;assert(!af_start(p,s,NULL));p->base[4]=0;
    p->shape[0].stats[4]=NAN;assert(!af_start(p,s,NULL));p->shape[0].stats[4]=8;
    p->selected=32;assert(!af_start(p,s,NULL));
    p->selected=AF_ALL_STATS;p->targets=p->caps=0;
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.stats[4]==3&&s->best.stats[0]==100);
    /* Only an upside-down L fits this room; it must remain in inventory. */
    memset(p,0,sizeof(*p));p->width=p->height=2;p->count=1;p->selected=1;
    p->grid[0]=6;p->shape[0].count=p->shape[0].solid=3;p->shape[0].stats[0]=10;
    p->shape[0].cells[0]=(AFCell){0,0,1};p->shape[0].cells[1]=(AFCell){1,0,1};
    p->shape[0].cells[2]=(AFCell){0,1,1};
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.used==0);
    AFLayout legacy={0};legacy.placement[0]=(AFPlacement){1,1,-1,-1,1};
    assert(af_start(p,s,&legacy)&&s->best.used==0);
    for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.used==0&&legacy.placement[0].used&&legacy.placement[0].sy==-1);
    /* The upright horizontal mirror is still available. */
    p->grid[0]=0;p->grid[2]=6;
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.used==1&&s->best.placement[0].sx==-1&&s->best.placement[0].sy==1);
    /* Removing a legacy inverted support also removes its dependent seed. */
    memset(p,0,sizeof(*p));p->width=1;p->height=2;p->count=2;p->selected=1;
    p->shape[0].count=p->shape[0].solid=1;p->shape[0].cells[0]=(AFCell){0,0,2};
    p->shape[1].count=2;p->shape[1].solid=1;p->shape[1].stats[0]=5;
    p->shape[1].cells[0]=(AFCell){0,0,3};p->shape[1].cells[1]=(AFCell){0,1,1};
    memset(&legacy,0,sizeof(legacy));legacy.placement[0]=(AFPlacement){0,0,1,-1,1};
    legacy.placement[1]=(AFPlacement){0,0,1,1,1};
    assert(af_start(p,s,&legacy)&&s->best.used==0);
    for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.used==2&&s->best.placement[0].sy==1&&s->best.placement[1].sy==1);
    /* Appeal search drops zero-stat boxes; the final pass fills gaps without moving its result. */
    memset(p,0,sizeof(*p));p->width=3;p->height=1;p->count=4;p->selected=16;
    p->targets=p->caps=16;p->minimum[4]=4;p->maximum[4]=6;
    for(int i=0;i<4;i++) {
        p->shape[i].count=p->shape[i].solid=1;p->shape[i].cells[0]=(AFCell){0,0,1};
    }
    p->shape[0].stats[4]=5;
    assert(af_start(p,s,NULL));for(int i=0;i<100;i++)af_step(p,s);
    assert(s->best.used==1&&s->best.stats[4]==5);
    AFLayout optimized=s->best;
    af_fill_utilities(p,s);assert(s->best.used==1); /* Disabled/non-utility pieces stay unused. */
    p->shape[1].utility=p->shape[2].utility=1;
    af_fill_utilities(p,s);
    assert(s->best.used==3&&s->best.placement[1].used&&s->best.placement[2].used&&!s->best.placement[3].used);
    assert(!memcmp(s->best.stats,optimized.stats,sizeof(optimized.stats)));
    assert(!memcmp(&s->best.placement[0],&optimized.placement[0],sizeof(AFPlacement)));
    assert(af_within_caps(p,s->best.stats)&&af_rebuild(p,&s->best,s->grid,0));
    af_fill_utilities(p,s);assert(s->best.used==3); /* No duplicates. */
    /* Utility fill cannot lower an unselected stat, exceed Max, or displace the optimized item. */
    s->best=optimized;p->shape[1].stats[0]=-1;p->shape[2].stats[4]=2;
    af_fill_utilities(p,s);assert(s->best.used==1);
    p->shape[1].stats[0]=0;p->shape[2].stats[4]=0;p->width=1;
    optimized.placement[0].x=0;s->best=optimized;
    af_fill_utilities(p,s);assert(s->best.used==1&&s->best.placement[0].used);
    /* A utility support can enable another utility on the next pass; reserved cells stay blocked. */
    memset(p,0,sizeof(*p));p->width=2;p->height=2;p->count=2;p->selected=16;
    p->grid[1]=p->grid[3]=6;
    p->shape[0].utility=p->shape[1].utility=1;
    p->shape[0].count=2;p->shape[0].solid=1;
    p->shape[0].cells[0]=(AFCell){0,0,3};p->shape[0].cells[1]=(AFCell){0,1,1};
    p->shape[1].count=p->shape[1].solid=1;p->shape[1].cells[0]=(AFCell){0,0,2};
    assert(af_start(p,s,NULL));af_fill_utilities(p,s);
    assert(s->best.used==2&&af_rebuild(p,&s->best,s->grid,0));
    for(int i=0;i<2;i++)assert(s->best.placement[i].sy==1&&s->best.placement[i].x==0);
    /* A newly filled overlay must not change the occupancy precedence of existing pieces. */
    memset(p,0,sizeof(*p));p->width=p->height=1;p->count=2;p->selected=16;
    p->shape[0].count=p->shape[1].count=p->shape[0].solid=p->shape[1].solid=1;
    p->shape[0].utility=1;p->shape[0].cells[0]=(AFCell){0,0,5};
    p->shape[1].stats[4]=5;p->shape[1].cells[0]=(AFCell){0,0,1};
    memset(&optimized,0,sizeof(optimized));optimized.placement[1]=(AFPlacement){0,0,1,1,1};
    assert(af_start(p,s,&optimized));af_fill_utilities(p,s);
    assert(s->best.used==1&&af_rebuild(p,&s->best,s->grid,0));
    p->width=2;af_fill_utilities(p,s);
    assert(s->best.used==2&&s->best.placement[0].x==1&&af_rebuild(p,&s->best,s->grid,0));
    free(s);free(p);puts("PASS: five-stat scoring, Appeal targets/caps, empty/zero, range correction, scarcity, secondary fill, utility gap fill, support, bounds, reserved cells, overlap, input validation, horizontal-only flips, legacy inverted layouts");
}
