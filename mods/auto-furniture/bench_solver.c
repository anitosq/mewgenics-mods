/* Offline solver benchmark on synthetic side-view rooms (floor, side walls, tables,
   wall decorations, utility boxes). Not game data: use it to compare solver changes.
   Usage: bench_solver [seconds=1.2] [items=150] [max_iterations=3000]
   BIG=1 uses 30-42 x 16 rooms; V=1 prints each case. Higher TOTAL is better:
   per case, lowest selected stat + 0.05 * sum of selected stats. */
#include "solver.h"
#include <stdio.h>
#include <time.h>
#define SEEDS 12
#define UTILITY_RESERVE 0.1 /* seconds, as in the planner */
static double now(void) {
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec+t.tv_nsec*1e-9;
}
static double deadline;
static int past_deadline(void* context) {(void)context;return now()>=deadline;}
static uint32_t rs;
static int rnd(int n) {rs^=rs<<13;rs^=rs>>17;rs^=rs<<5;return (int)(rs%(unsigned)n);}
static void add_cell(AFShape* s,int x,int y,int value) {
    s->cells[s->count++]=(AFCell){(unsigned char)x,(unsigned char)y,(unsigned char)value};
}
/* Tables (cell value 2 on top) and other floor furniture rest on a surface (value 3). */
static void make_shape(AFShape* s) {
    int kind=rnd(10),w=1+rnd(4),h=1+rnd(3);
    if(kind<2) {
        w=1+rnd(2);h=1+rnd(2);
        for(int y=0;y<h;y++)for(int x=0;x<w;x++)add_cell(s,x,y,5);
    } else {
        int table=kind<4;
        for(int x=0;x<w;x++)add_cell(s,x,0,3);
        for(int y=1;y<=h;y++)for(int x=0;x<w;x++)add_cell(s,x,y,table&&y==h?2:1);
    }
    s->solid=w*h;
    int boosts=1+rnd(2);
    for(int k=0;k<boosts;k++)s->stats[rnd(5)]+=1+rnd(6);
    if(rnd(8)==0)s->stats[rnd(5)]-=1+rnd(3);
    if(rnd(6)==0){memset(s->stats,0,sizeof s->stats);s->utility=1;}
}
static void make_room(AFProblem* p,int seed,int items,int width,int height,unsigned selected) {
    memset(p,0,sizeof(*p));rs=seed*2654435761u+1;
    p->width=width;p->height=height;p->selected=selected;
    for(int x=0;x<width;x++)p->grid[x]=2;
    for(int y=1;y<height;y++){p->grid[y*width]=6;p->grid[y*width+width-1]=6;}
    p->count=items;
    for(int i=0;i<items;i++)make_shape(&p->shape[i]);
}
/* Occupied cells outside the starting floor. */
static int filled(const AFProblem* p,const AFSearch* s) {
    unsigned char grid[AF_CELLS];AFLayout layout=s->best;af_rebuild(p,&layout,grid,0);
    int count=0;
    for(int i=0;i<p->width*p->height;i++)count+=(grid[i]==1||grid[i]==2||grid[i]==5)&&p->grid[i]!=2;
    return count;
}
static double score(const double stats[AF_STATS],unsigned selected) {
    double lowest=1e9,sum=0;
    for(int j=0;j<AF_STATS;j++)if(selected&(1u<<j)) {
        if(stats[j]<lowest)lowest=stats[j];
        sum+=stats[j];
    }
    return lowest+0.05*sum;
}
int main(int argc,char** argv) {
    double budget=argc>1?atof(argv[1]):1.2;
    int items=argc>2?atoi(argv[2]):150;
    unsigned max_iterations=(unsigned)(argc>3?atoi(argv[3]):3000);
    int big=getenv("BIG")!=NULL,verbose=getenv("V")!=NULL;
    static AFProblem p;static AFSearch s;
    const unsigned selections[3]={1|2,AF_ALL_STATS,16};
    double total=0;long total_filled=0;
    for(int seed=1;seed<=SEEDS;seed++)for(int k=0;k<3;k++) {
        int width=big?30+seed%3*6:18+seed%3*4,height=big?16:9;
        make_room(&p,seed,items,width,height,selections[k]);
        af_start(&p,&s,NULL);
        double start=now();deadline=start+budget-UTILITY_RESERVE;s.stop=past_deadline;
        while(s.iterations<max_iterations&&!af_stopped(&s))af_step(&p,&s);
        deadline=start+budget;af_fill_utilities(&p,&s);
        total+=score(s.best.stats,selections[k]);
        int room_filled=filled(&p,&s);total_filled+=room_filled;
        if(verbose)printf("seed %d sel %2u: it %4u stats %5.1f %5.1f %5.1f %5.1f %5.1f  filled %d\n",
            seed,selections[k],s.iterations,s.best.stats[0],s.best.stats[1],s.best.stats[2],
            s.best.stats[3],s.best.stats[4],room_filled);
    }
    printf("TOTAL %.1f filled %ld\n",total,total_filled);
}
