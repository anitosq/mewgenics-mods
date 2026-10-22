/* Player-selected grid widths, one per house container (0 Storage, 1 Trash).
 * A size caps the native column count: the grid grows up to it, then scrolls.
 * Trash also offers 0 ("Auto"), which keeps the native growing grid. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#define IQ_GRID_DEFAULT 6
#define IQ_GRID_BUTTON_X 333
#define IQ_GRID_BUTTON_PITCH 120
#define IQ_GRID_BUTTON_WIDTH 118
#define IQ_GRID_BUTTON_HEIGHT 22
static const int iq_grid_sizes[]={5,6,7,8,10,0};
static int iq_grid_cap[2]={IQ_GRID_DEFAULT,IQ_GRID_DEFAULT};
static int iq_grid_columns(int native,int cap) {return cap>0 && native>cap?cap:native;}
/* Storage has a fixed game capacity and never offers Auto. */
static int iq_grid_size_count(int side) {return (int)(sizeof(iq_grid_sizes)/sizeof(*iq_grid_sizes))-(side==0);}
static int iq_grid_index(int side,int cap) {
    for(int i=0;i<iq_grid_size_count(side);i++)if(iq_grid_sizes[i]==cap)return i;
    return -1;
}
static int iq_grid_next(int side,int cap) {
    int i=iq_grid_index(side,cap);
    return i<0?IQ_GRID_DEFAULT:iq_grid_sizes[(i+1)%iq_grid_size_count(side)];
}
/* Equipment keeps its toolbar row: six columns show five rows. */
static int iq_grid_equipment_rows(int columns) {return columns>5?columns*5/6:columns;}
static void iq_grid_label(wchar_t* out,size_t capacity,int side,int cap) {
    const wchar_t* name=side?L"Trash":L"Storage";
    if(cap>0)swprintf(out,capacity,L"%ls %dx%d",name,cap,cap);
    else swprintf(out,capacity,L"%ls Auto",name);
}
static const char* iq_grid_keys[2]={"storage_columns=","trash_columns="};
/* settings.ini holds "storage_columns=<size>" and "trash_columns=<size|auto>"
   lines; anything else, including a missing or damaged file, keeps 6. */
static int iq_grid_parse(const char* text,int side) {
    const char* key=iq_grid_keys[side];
    for(const char* line=text;line && *line;) {
        while(*line==' ' || *line=='\t' || *line=='\r' || *line=='\n')line++;
        if(!strncmp(line,key,strlen(key))) {
            const char* value=line+strlen(key);
            if(side && !strncmp(value,"auto",4) && (!value[4] || value[4]=='\r' || value[4]=='\n'))return 0;
            char* end=NULL;long cap=strtol(value,&end,10);
            if(end!=value && (!*end || *end=='\r' || *end=='\n') && cap>0 && iq_grid_index(side,(int)cap)>=0)return (int)cap;
            return IQ_GRID_DEFAULT;
        }
        line=strchr(line,'\n');
    }
    return IQ_GRID_DEFAULT;
}
static int iq_grid_format(char* out,size_t capacity,const int caps[2]) {
    char trash[8];
    if(caps[1]>0)snprintf(trash,sizeof(trash),"%d",caps[1]);else strcpy(trash,"auto");
    return snprintf(out,capacity,"%s%d\r\n%s%s\r\n",iq_grid_keys[0],caps[0],iq_grid_keys[1],trash);
}
