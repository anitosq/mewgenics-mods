/* Pure layout search. No game pointers, ownership changes, or platform calls. */
#ifndef AF_SOLVER_H
#define AF_SOLVER_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#define AF_ITEMS 2048
#define AF_CELLS 4096
#define AF_STATS 5
#define AF_ALL_STATS ((1u<<AF_STATS)-1)
typedef struct { unsigned char x,y,value; } AFCell;
typedef struct {
    AFCell cells[576];
    int count,solid,utility;
    double stats[AF_STATS];
} AFShape;
typedef struct { int x,y,sx,sy,used; } AFPlacement;
#define AF_EVALUATIONS 100000
#define AF_RESTART 8 /* one fresh start per this many repairs */
typedef struct {
    int width,height,count;
    unsigned selected,targets,caps;
    double minimum[AF_STATS],maximum[AF_STATS],base[AF_STATS];
    unsigned char grid[AF_CELLS];
    AFShape shape[AF_ITEMS];
} AFProblem;
typedef struct {
    AFPlacement placement[AF_ITEMS];
    double stats[AF_STATS];
    int used;
} AFLayout;
typedef struct {
    AFLayout best,trial;
    unsigned char grid[AF_CELLS];
    int order[AF_ITEMS];
    double priority[AF_ITEMS];
    double area_divisor[5][AF_ITEMS];
    short surface[2][AF_CELLS];
    int shape_class[AF_ITEMS];
    unsigned failed[AF_ITEMS],version;
    int (*stop)(void*);
    void* stop_context;
    uint32_t random;
    unsigned iterations;
} AFSearch;
static int af_stopped(const AFSearch* s) {return s&&s->stop&&s->stop(s->stop_context);}
/* Preserve every support and occupied-cell overlap beneath fixed objects. */
static void af_fix_dependencies(const AFProblem* p,const AFLayout* layout,unsigned char fixed[AF_ITEMS]) {
    int changed=1;
    while(changed) {
        changed=0;
        for(int i=0;i<p->count;i++)if(fixed[i]&&layout->placement[i].used) {
            AFPlacement a=layout->placement[i];const AFShape* sa=&p->shape[i];
            for(int j=0;j<p->count;j++)if(!fixed[j]&&layout->placement[j].used) {
                AFPlacement b=layout->placement[j];const AFShape* sb=&p->shape[j];int needed=0;
                for(int ca=0;ca<sa->count&&!needed;ca++) {
                    AFCell ac=sa->cells[ca];if(ac.value==4)continue;
                    int x=a.x+ac.x*a.sx,y=a.y+ac.y*a.sy;
                    for(int cb=0;cb<sb->count;cb++) {
                        AFCell bc=sb->cells[cb];
                        if((ac.value==3?bc.value==2:(bc.value<=2||bc.value==5))&&
                           x==b.x+bc.x*b.sx&&y==b.y+bc.y*b.sy){needed=1;break;}
                    }
                }
                if(needed){fixed[j]=1;changed=1;}
            }
        }
    }
}
static void af_sort(double* values,int n) {
    for(int i=1;i<n;i++)for(int j=i;j>0&&values[j]<values[j-1];j--) {
        double t=values[j];values[j]=values[j-1];values[j-1]=t;
    }
}
static int af_parse_bound(const char* text,double* value) {
    *value=0;if(!*text)return 0;
    char* end=NULL;*value=strtod(text,&end);
    return end==text||*end||!isfinite(*value)||*value<-1000000||*value>1000000?-1:1;
}
static double af_correct_max(double minimum,double maximum) {
    return maximum<=minimum?minimum+1:maximum;
}
static int af_within_caps(const AFProblem* p,const double stats[AF_STATS]) {
    for(int i=0;i<AF_STATS;i++)if((p->caps&(1u<<i))&&stats[i]>p->maximum[i]+1e-8)return 0;
    return 1;
}
/* Caps, minimum targets, selected stats, then unselected stats. */
static int af_compare(const AFProblem* p,const double a[AF_STATS],const double b[AF_STATS]) {
    double av[AF_STATS],bv[AF_STATS];int n=0;
    for(int i=0;i<AF_STATS;i++)if(p->caps&(1u<<i)) {
        av[n]=fmax(0,a[i]-p->maximum[i]);bv[n++]=fmax(0,b[i]-p->maximum[i]);
    }
    af_sort(av,n);af_sort(bv,n);
    for(int i=n-1;i>=0;i--)if(fabs(av[i]-bv[i])>1e-8)return av[i]<bv[i]?1:-1;
    n=0;
    for(int i=0;i<AF_STATS;i++)if((p->targets&p->selected)&(1u<<i)) {
        av[n]=fmax(0,p->minimum[i]-a[i]);bv[n++]=fmax(0,p->minimum[i]-b[i]);
    }
    af_sort(av,n);af_sort(bv,n);
    for(int i=n-1;i>=0;i--)if(fabs(av[i]-bv[i])>1e-8)return av[i]<bv[i]?1:-1;
    n=0;
    for(int i=0;i<AF_STATS;i++)if(p->selected&(1u<<i)){av[n]=a[i];bv[n++]=b[i];}
    af_sort(av,n);af_sort(bv,n);
    for(int i=0;i<n;i++)if(fabs(av[i]-bv[i])>1e-8)return av[i]>bv[i]?1:-1;
    n=0;
    for(int i=0;i<AF_STATS;i++)if(!(p->selected&(1u<<i))){av[n]=a[i];bv[n++]=b[i];}
    af_sort(av,n);af_sort(bv,n);
    for(int i=0;i<n;i++)if(fabs(av[i]-bv[i])>1e-8)return av[i]>bv[i]?1:-1;
    return 0;
}
static int af_valid_problem(const AFProblem* p) {
    if(p->width<1||p->height<1||p->width>64||p->height>64||p->count<0||p->count>AF_ITEMS||
       !p->selected||(p->selected&~AF_ALL_STATS)||(p->targets&~p->selected)||(p->caps&~p->selected))return 0;
    for(int j=0;j<AF_STATS;j++) {
        /* Bounds may be negative: some furniture lowers stats, and players may want that. */
        if(!isfinite(p->base[j])||!isfinite(p->minimum[j])||!isfinite(p->maximum[j]))return 0;
        if((p->targets&p->caps&(1u<<j))&&p->maximum[j]<=p->minimum[j])return 0;
    }
    for(int i=0;i<p->width*p->height;i++)if(p->grid[i]>8)return 0;
    for(int i=0;i<p->count;i++) {
        const AFShape* s=&p->shape[i];
        if(s->count<1||s->count>576)return 0;
        for(int j=0;j<AF_STATS;j++)if(!isfinite(s->stats[j]))return 0;
        unsigned char seen[576]={0};
        for(int j=0;j<s->count;j++) {
            AFCell c=s->cells[j];
            if(c.x>=24||c.y>=24||c.value<1||c.value>5||seen[c.y*24+c.x]++)return 0;
        }
    }
    return 1;
}
static int af_fits(const AFProblem* p,const unsigned char* grid,const AFShape* s,AFPlacement v) {
    /* Geometry checks also read legacy inverted layouts; search only creates upright ones. */
    if((v.sx!=1&&v.sx!=-1)||(v.sy!=1&&v.sy!=-1))return 0;
    for(int i=0;i<s->count;i++) {
        AFCell c=s->cells[i];int x=v.x+c.x*v.sx,y=v.y+c.y*v.sy;
        if(x<0||x>=p->width||y<0||y>=p->height)return 0;
        unsigned char g=grid[y*p->width+x];
        if(g>=6)return 0;
        if(c.value<=2&&(g==1||g==2||g==5))return 0;
        if(c.value==3&&g!=2)return 0;
        if(c.value==5&&(g==2||g==5))return 0;
    }
    return 1;
}
static void af_stamp(const AFProblem* p,unsigned char* grid,const AFShape* s,AFPlacement v) {
    for(int i=0;i<s->count;i++) {
        AFCell c=s->cells[i];
        if(c.value<=2||c.value==5)grid[(v.y+c.y*v.sy)*p->width+v.x+c.x*v.sx]=c.value;
    }
}
/* Dependency order is discovered by repeated placement; unsupported cycles fail. */
static int af_rebuild_checked(const AFProblem* p,AFLayout* layout,unsigned char* grid,int prune,const AFSearch* search) {
    unsigned char done[AF_ITEMS]={0};int remaining=0;
    memcpy(grid,p->grid,(size_t)p->width*p->height);memcpy(layout->stats,p->base,sizeof(p->base));layout->used=0;
    for(int i=0;i<p->count;i++)remaining+=!!layout->placement[i].used;
    while(remaining) {
        int progress=0;
        for(int i=0;i<p->count;i++) {
            if(!(i%64)&&af_stopped(search))return 0;
            if(layout->placement[i].used&&!done[i]&&af_fits(p,grid,&p->shape[i],layout->placement[i])) {
                af_stamp(p,grid,&p->shape[i],layout->placement[i]);done[i]=1;remaining--;progress++;layout->used++;
                for(int j=0;j<AF_STATS;j++)layout->stats[j]+=p->shape[i].stats[j];
            }
        }
        if(!progress) {
            if(!prune)return 0;
            for(int i=0;i<p->count;i++)if(!done[i])layout->placement[i].used=0;
            break;
        }
    }
    return 1;
}
static int af_rebuild(const AFProblem* p,AFLayout* layout,unsigned char* grid,int prune) {
    return af_rebuild_checked(p,layout,grid,prune,NULL);
}
static uint32_t af_random(AFSearch* s) {
    uint32_t x=s->random;x^=x<<13;x^=x>>17;x^=x<<5;return s->random=x;
}
static int af_start(const AFProblem* p,AFSearch* search,const AFLayout* initial) {
    if(!af_valid_problem(p))return 0;
    memset(search,0,sizeof(*search));search->random=0x9145ab13;
    for(int phase=0;phase<5;phase++)for(int i=0;i<p->count;i++)
        search->area_divisor[phase][i]=pow(fmax(1,p->shape[i].solid),0.3+phase*0.2);
    for(int i=0;i<p->count;i++) {
        int j=0;
        while(j<i&&(p->shape[j].count!=p->shape[i].count||
              memcmp(p->shape[j].cells,p->shape[i].cells,(size_t)p->shape[i].count*sizeof(AFCell))))j++;
        search->shape_class[i]=j;
    }
    if(initial)search->best=*initial;
    if(!af_rebuild(p,&search->best,search->grid,0))return 0;
    /* Old builds could invert furniture. Do not seed new layouts with those pieces. */
    for(int i=0;i<p->count;i++)if(search->best.placement[i].sy==-1)search->best.placement[i].used=0;
    return af_rebuild(p,&search->best,search->grid,1);
}
static void af_keep(const AFProblem* p,AFSearch* s) {
    int cmp=af_compare(p,s->trial.stats,s->best.stats);
    if(cmp>0||(cmp==0&&s->trial.used<s->best.used))s->best=s->trial;
}
static void af_place(const AFProblem* p,AFSearch* s,int fill_utilities) {
    unsigned evaluations=0;
    int progress=1,surfaces=0,surfaces_ready=0;
    /* Placing pieces only removes space, except new surfaces. A shape that found no spot stays
       impossible, together with every identical piece, until another surface appears. */
    s->version++;
    while(progress) {
        progress=0;
        for(int at=0;at<p->count;at++) {
            if(!(at%64)&&af_stopped(s))return;
            int i=s->order[at];if(s->trial.placement[i].used)continue;
            const AFShape* shape=&p->shape[i];AFPlacement choice={0};
            if(fill_utilities) {
                if(!shape->utility)continue;
                int safe=1;
                for(int j=0;j<AF_STATS;j++)if(shape->stats[j]<0||
                    ((p->caps&(1u<<j))&&s->trial.stats[j]+shape->stats[j]>p->maximum[j]+1e-8))safe=0;
                if(!safe)continue;
            }
            if(s->failed[s->shape_class[i]]==s->version)continue;
            int flip=(int)(af_random(s)%2),reverse=(int)(af_random(s)%2);
            int anchor=-1;unsigned options=0;
            for(int c=0;c<shape->count&&anchor<0;c++)if(shape->cells[c].value==3)anchor=c;
            if(anchor>=0&&!surfaces_ready) {
                /* Surface cells by row, left to right and right to left, matching the full scan. */
                surfaces=0;
                for(int y=0,k=0;y<p->height;y++) {
                    for(int x=0;x<p->width;x++)if(s->grid[y*p->width+x]==2)s->surface[0][surfaces++]=(short)(y*p->width+x);
                    for(int x=p->width-1;x>=0;x--)if(s->grid[y*p->width+x]==2)s->surface[1][k++]=(short)(y*p->width+x);
                }
                surfaces_ready=1;
            }
            for(int orient=0;orient<2;orient++) {
                int sx=((orient+flip)&1)?-1:1,sy=1;
                int xmin=-23,xmax=p->width+22,ymin=-23,ymax=p->height+22;
                for(int c=0;c<shape->count;c++) {
                    int dx=shape->cells[c].x*sx,dy=shape->cells[c].y*sy;
                    if(-dx>xmin)xmin=-dx;
                    if(p->width-1-dx<xmax)xmax=p->width-1-dx;
                    if(-dy>ymin)ymin=-dy;
                    if(p->height-1-dy<ymax)ymax=p->height-1-dy;
                }
                /* A piece that rests on a surface can only sit where its first resting cell
                   meets one, so try those surface cells instead of every cell in the room. */
                int total=anchor>=0?surfaces:(ymax-ymin+1)*(xmax-xmin+1);
                for(int k=0;k<total;k++) {
                    int x,y;
                    if(anchor>=0) {
                        int cell=s->surface[reverse][k];
                        x=cell%p->width-shape->cells[anchor].x*sx;y=cell/p->width-shape->cells[anchor].y;
                        if(x<xmin||x>xmax||y<ymin||y>ymax)continue;
                    } else {
                        int xx=xmin+k%(xmax-xmin+1);y=ymin+k/(xmax-xmin+1);
                        x=reverse?xmax-(xx-xmin):xx;
                    }
                    /* Keep a single trial bounded even with huge modded inventories. */
                    if(++evaluations>AF_EVALUATIONS)goto found;
                    if(!(evaluations%256)&&af_stopped(s))return;
                    AFPlacement v={x,y,sx,sy,1};
                    if(!af_fits(p,s->grid,shape,v))continue;
                    if(fill_utilities) {
                        /* Do not overwrite existing type-1 occupancy with a type-5 overlay. */
                        int overlap=0;
                        for(int c=0;c<shape->count;c++)if(shape->cells[c].value==5&&
                            s->grid[(y+shape->cells[c].y)*p->width+x+shape->cells[c].x*sx]==1)overlap=1;
                        if(overlap)continue;
                        choice=v;goto found;
                    }
                    /* Uniform over all fitting spots: measured better than first-fit or
                       compact packing, because varied placements feed the repair search. */
                    options++;
                    if(!choice.used||af_random(s)%options==0)choice=v;
                }
            }
            if(!choice.used){s->failed[s->shape_class[i]]=s->version;continue;}
found:
            if(!choice.used)continue;
            s->trial.placement[i]=choice;af_stamp(p,s->grid,shape,choice);s->trial.used++;progress++;
            surfaces_ready=0;
            for(int c=0;c<shape->count;c++)if(shape->cells[c].value==2){s->version++;break;}
            for(int j=0;j<AF_STATS;j++)s->trial.stats[j]+=shape->stats[j];
            if(fill_utilities)s->best=s->trial;else af_keep(p,s);
        }
    }
}
/* Stable order preserves the old search's tie breaking and random sequence. */
static int af_order(AFSearch* s,int count) {
    int scratch[AF_ITEMS];
    for(int width=1;width<count;width*=2) {
        if(af_stopped(s))return 0;
        for(int lo=0;lo<count;lo+=2*width) {
            int mid=lo+width<count?lo+width:count,hi=lo+2*width<count?lo+2*width:count;
            int left=lo,right=mid;
            for(int out=lo;out<hi;out++) {
                if(left<mid&&(right==hi||s->priority[s->order[left]]>=s->priority[s->order[right]]))
                    scratch[out]=s->order[left++];
                else scratch[out]=s->order[right++];
            }
        }
        memcpy(s->order,scratch,(size_t)count*sizeof(*scratch));
    }
    return !af_stopped(s);
}
/* ponytail: bounded multi-start/local repair finds good layouts, not a proof of optimality.
   Replace with branch-and-bound only if proven optima become a requirement. */
static void af_step(const AFProblem* p,AFSearch* s) {
    if(af_stopped(s))return;
    memset(&s->trial,0,sizeof(s->trial));
    if(s->iterations%AF_RESTART) {
        s->trial=s->best;
        if(af_random(s)%2) {
            /* Clear a vertical band so neighbouring pieces can be repacked together. */
            int band=1+(int)(af_random(s)%(unsigned)(p->width/2+1)),left=(int)(af_random(s)%(unsigned)p->width);
            for(int i=0;i<p->count;i++)if(s->trial.placement[i].used) {
                int x=s->trial.placement[i].x;
                if(x>=left-2&&x<left+band)s->trial.placement[i].used=0;
            }
        } else {
            unsigned remove=15+af_random(s)%70;
            for(int i=0;i<p->count;i++)if(af_random(s)%100<remove)s->trial.placement[i].used=0;
        }
    }
    if(!af_rebuild_checked(p,&s->trial,s->grid,1,s))return;
    af_keep(p,s);
    double weights[AF_STATS];
    for(int j=0;j<AF_STATS;j++)weights[j]=(p->selected&(1u<<j))?(0.1+(af_random(s)%1000)/250.0):0;
    /* Balanced stats improve the lowest first, so most repairs favour the weakest selected
       stat, unmet Min targets, and avoid stats already at their Max. */
    if(s->iterations%2) {
        double low=INFINITY,scale=1;
        for(int j=0;j<AF_STATS;j++)if(p->selected&(1u<<j)) {
            if(s->best.stats[j]<low)low=s->best.stats[j];scale+=fabs(s->best.stats[j]);
        }
        scale=0.15*scale/AF_STATS+1;
        for(int j=0;j<AF_STATS;j++)if(p->selected&(1u<<j)) {
            double gap=s->best.stats[j]-low;
            if((p->targets&(1u<<j))&&s->best.stats[j]<p->minimum[j])gap=0;
            weights[j]*=1/(1+gap/scale);
            if((p->caps&(1u<<j))&&s->best.stats[j]>=p->maximum[j]-1e-8)weights[j]=-weights[j];
        }
    }
    for(int i=0;i<p->count;i++) {
        s->order[i]=i;double value=0;
        for(int j=0;j<AF_STATS;j++)value+=p->shape[i].stats[j]*weights[j];
        /* Fresh starts also explore orders that raw stat value would always reject. */
        s->priority[i]=s->iterations%AF_RESTART?value/s->area_divisor[s->iterations%5][i]:0;
        s->priority[i]+=(af_random(s)%1000)*0.003;
    }
    if(!af_order(s,p->count))return;
    af_place(p,s,0);
    s->iterations++;
}
static void af_fill_utilities(const AFProblem* p,AFSearch* s) {
    if(af_stopped(s))return;
    if(!af_within_caps(p,s->best.stats))return;
    s->trial=s->best;
    if(!af_rebuild_checked(p,&s->trial,s->grid,0,s))return;
    for(int i=0;i<p->count;i++)s->order[i]=i;
    /* Add only: keep every optimized placement and use the same native shape rules. */
    af_place(p,s,1);
}
#endif
