/* Player-selected grid width. A cap of 0 ("Auto") keeps the native growing
 * grid; otherwise the native grid grows up to the cap, then scrolls. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#define IQ_GRID_DEFAULT 6
#define IQ_GRID_BUTTON_X 455
#define IQ_GRID_BUTTON_WIDTH 116
#define IQ_GRID_BUTTON_HEIGHT 22
static const int iq_grid_sizes[]={5,6,7,8,10,0};
static int iq_grid_cap=IQ_GRID_DEFAULT;
static int iq_grid_columns(int native,int cap) {return cap>0 && native>cap?cap:native;}
static int iq_grid_index(int cap) {
    for(int i=0;i<(int)(sizeof(iq_grid_sizes)/sizeof(*iq_grid_sizes));i++)if(iq_grid_sizes[i]==cap)return i;
    return -1;
}
static int iq_grid_next(int cap) {
    int n=(int)(sizeof(iq_grid_sizes)/sizeof(*iq_grid_sizes)),i=iq_grid_index(cap);
    return i<0?IQ_GRID_DEFAULT:iq_grid_sizes[(i+1)%n];
}
/* Equipment keeps its toolbar row: six columns show five rows. */
static int iq_grid_equipment_rows(int columns) {return columns>5?columns*5/6:columns;}
static void iq_grid_label(wchar_t* out,size_t capacity,int cap) {
    if(cap>0)swprintf(out,capacity,L"Grid: %dx%d",cap,cap);
    else swprintf(out,capacity,L"Grid: Auto");
}
/* settings.ini holds one "grid_columns=<5|6|7|8|10|auto>" line; anything
   else, including a missing or damaged file, keeps the default. */
static int iq_grid_parse(const char* text) {
    const char* key="grid_columns=";
    for(const char* line=text;line && *line;) {
        while(*line==' ' || *line=='\t' || *line=='\r' || *line=='\n')line++;
        if(!strncmp(line,key,strlen(key))) {
            const char* value=line+strlen(key);
            if(!strncmp(value,"auto",4) && (!value[4] || value[4]=='\r' || value[4]=='\n'))return 0;
            char* end=NULL;long cap=strtol(value,&end,10);
            if(end!=value && (!*end || *end=='\r' || *end=='\n') && cap>0 && iq_grid_index((int)cap)>=0)return (int)cap;
            return IQ_GRID_DEFAULT;
        }
        line=strchr(line,'\n');
    }
    return IQ_GRID_DEFAULT;
}
static int iq_grid_format(char* out,size_t capacity,int cap) {
    return cap>0?snprintf(out,capacity,"grid_columns=%d\r\n",cap):snprintf(out,capacity,"grid_columns=auto\r\n");
}
