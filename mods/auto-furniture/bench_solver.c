/* Offline solver benchmark on synthetic side-view rooms (floor, side walls, tables,
   wall decorations, utility boxes). Not game data: use it to compare solver changes.
   Usage: bench_solver [seconds=1.2] [items=150] [max_iterations=3000]
   BIG=1 uses 30-42 x 16 rooms; V=1 prints each case. Higher TOTAL is better:
   per case, lowest selected stat + 0.05 * sum of selected stats. */
#include "solver.h"
#include <stdio.h>
#include <time.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static double deadline;
static int stopper(void* c){(void)c;return now()>=deadline;}
static uint32_t rs;static int rnd(int n){rs^=rs<<13;rs^=rs>>17;rs^=rs<<5;return (int)(rs%(unsigned)n);}
static void make(AFProblem* p,int seed,int items,int W,int H,unsigned sel){
  memset(p,0,sizeof(*p));rs=seed*2654435761u+1;
  p->width=W;p->height=H;p->selected=sel;
  for(int x=0;x<W;x++)p->grid[x]=2;           /* floor surface */
  for(int y=1;y<H;y++){p->grid[y*W]=6;p->grid[y*W+W-1]=6;} /* side walls */
  p->count=items;
  for(int i=0;i<items;i++){AFShape*s=&p->shape[i];int kind=rnd(10);
    int w=1+rnd(4),h=1+rnd(3);
    if(kind<2){ /* wall decoration */ w=1+rnd(2);h=1+rnd(2);
      for(int y=0;y<h;y++)for(int x=0;x<w;x++)s->cells[s->count++]=(AFCell){x,y,5};
      s->solid=w*h;
    } else {
      for(int x=0;x<w;x++)s->cells[s->count++]=(AFCell){x,0,3};
      for(int y=1;y<=h;y++)for(int x=0;x<w;x++)s->cells[s->count++]=(AFCell){x,y,(kind<4&&y==h)?2:1};
      s->solid=w*h;
    }
    int ns=1+rnd(2);for(int k=0;k<ns;k++)s->stats[rnd(5)]+=1+rnd(6);
    if(rnd(8)==0)s->stats[rnd(5)]-=1+rnd(3);
    if(rnd(6)==0){memset(s->stats,0,sizeof s->stats);s->utility=1;}
  }
}
static int filled(const AFProblem*p,const AFSearch*s){unsigned char g[AF_CELLS];AFLayout L=s->best;af_rebuild(p,&L,g,0);
  int f=0;for(int i=0;i<p->width*p->height;i++)f+=(g[i]==1||g[i]==2||g[i]==5)&&p->grid[i]!=2;return f;}
int main(int argc,char**argv){
  double budget=argc>1?atof(argv[1]):1.2; int items=argc>2?atoi(argv[2]):150;
  static AFProblem p;static AFSearch s;
  unsigned sels[3]={1|2,AF_ALL_STATS,16};
  double total=0;long tf=0;int verbose=getenv("V")!=NULL;for(int seed=1;seed<=12;seed++)for(int k=0;k<3;k++){
    make(&p,seed,items,getenv("BIG")?30+seed%3*6:18+seed%3*4,getenv("BIG")?16:9,sels[k]);
    af_start(&p,&s,NULL);double t0=now();deadline=t0+budget-0.1;s.stop=stopper;
    while(s.iterations<(unsigned)(argc>3?atoi(argv[3]):3000)&&!af_stopped(&s))af_step(&p,&s);
    deadline=t0+budget;af_fill_utilities(&p,&s);
    {double mn=1e9,sum=0;for(int j=0;j<5;j++)if(sels[k]&(1u<<j)){if(s.best.stats[j]<mn)mn=s.best.stats[j];sum+=s.best.stats[j];}total+=mn+0.05*sum;}tf+=filled(&p,&s);
    if(verbose)printf("seed %d sel %2u: it %4u stats %5.1f %5.1f %5.1f %5.1f %5.1f  filled %d\n",seed,sels[k],s.iterations,
      s.best.stats[0],s.best.stats[1],s.best.stats[2],s.best.stats[3],s.best.stats[4],filled(&p,&s));
  }
  printf("TOTAL %.1f filled %ld\n",total,tf);
}
