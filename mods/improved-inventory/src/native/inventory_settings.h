/* Per-user preferences outside the game and mod-manager folders, so updates,
 * redeploys and Vortex purges leave them alone. Failures keep the defaults. */
static int iq_settings_path(wchar_t* path,int create) {
    DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",path,MAX_PATH);
    if(!n || n+40>=MAX_PATH)return 0;
    wcscat(path,L"\\ImprovedInventory");
    if(create && !CreateDirectoryW(path,NULL) && GetLastError()!=ERROR_ALREADY_EXISTS)return 0;
    wcscat(path,L"\\settings.ini");
    return 1;
}
static void iq_load_settings(void) {
    wchar_t path[MAX_PATH];char text[256]={0};DWORD received=0;
    if(!iq_settings_path(path,0))return;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if(file==INVALID_HANDLE_VALUE)return;
    BOOL ok=ReadFile(file,text,sizeof(text)-1,&received,NULL);CloseHandle(file);
    if(!ok)return;
    text[received]=0;
    for(int side=0;side<2;side++)iq_grid_cap[side]=iq_grid_parse(text,side);
    char msg[100];snprintf(msg,sizeof(msg),"Grid settings loaded: Storage %d, Trash %d columns (0 = auto).",iq_grid_cap[0],iq_grid_cap[1]);report(msg);
}
static void iq_save_settings(void) {
    wchar_t path[MAX_PATH];char text[96];DWORD written=0;
    int n=iq_grid_format(text,sizeof(text),iq_grid_cap);
    if(n<=0 || n>=(int)sizeof(text) || !iq_settings_path(path,1)){report("Grid setting not saved: no settings folder.");return;}
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE){report("Grid setting not saved: cannot write settings.ini.");return;}
    BOOL ok=WriteFile(file,text,(DWORD)n,&written,NULL);CloseHandle(file);
    if(!ok || written!=(DWORD)n)report("Grid setting not saved completely.");
}
