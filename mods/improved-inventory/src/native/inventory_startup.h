/* Release startup gate. Paths are compared as complete Windows arguments;
 * a substring match could activate a disabled mod or a different folder. */
#include <shellapi.h>

static int iq_canonical_path(const wchar_t* input,wchar_t* output) {
    DWORD n=GetFullPathNameW(input,MAX_PATH,output,NULL);
    if(!n || n>=MAX_PATH)return 0;
    for(DWORD i=0;i<n;i++)if(output[i]==L'/')output[i]=L'\\';
    while(n>3 && output[n-1]==L'\\')output[--n]=0;
    return 1;
}

enum IQAssetStatus {
    IQ_ASSETS_INVALID_PATH=0,
    IQ_ASSETS_READY=1,
    IQ_ASSETS_DISABLED=2,
    IQ_ASSETS_MISMATCHED=3,
    IQ_ASSETS_AMBIGUOUS=4
};

static int iq_assets_status(const wchar_t* dll_path,const wchar_t* command) {
    wchar_t parent[MAX_PATH],sibling[MAX_PATH],actual[MAX_PATH];
    if(!dll_path || !command || !iq_canonical_path(dll_path,parent))return IQ_ASSETS_INVALID_PATH;
    wchar_t* slash=wcsrchr(parent,L'\\');
    if(!slash)return IQ_ASSETS_INVALID_PATH;
    /* Preserve a drive root's trailing separator. */
    if(slash==parent+2)slash[1]=0;else *slash=0;
    if(wcslen(parent)+20>=MAX_PATH)return IQ_ASSETS_INVALID_PATH;
    wcscpy(sibling,parent);wcscat(sibling,L"\\ImprovedInventory");
    if(!iq_canonical_path(sibling,actual))return IQ_ASSETS_INVALID_PATH;
    wcscpy(sibling,actual);
    int argc=0,enabled=0,paths=0;
    wchar_t** argv=CommandLineToArgvW(command,&argc);
    if(!argv)return IQ_ASSETS_INVALID_PATH;
    for(int i=1;i<argc;i++) {
        if(!wcscmp(argv[i],L"-modpaths")) {paths=1;continue;}
        if(argv[i][0]==L'-') {paths=0;continue;}
        if(paths && argv[i][0] && iq_canonical_path(argv[i],actual)) {
            if(!_wcsicmp(parent,actual))enabled|=1;
            if(!_wcsicmp(sibling,actual))enabled|=2;
        }
    }
    LocalFree(argv);
    if(!enabled)return IQ_ASSETS_DISABLED;
    /* Never guess between two enabled copies of the UI assets. */
    if(enabled==3)return IQ_ASSETS_AMBIGUOUS;
    const wchar_t* root=enabled==1?parent:sibling;
    if(wcslen(root)+40>=MAX_PATH)return IQ_ASSETS_INVALID_PATH;
    wcscpy(actual,root);wcscat(actual,L"\\swfs\\improved_inventory.swf");
    if(!iq_file_matches(actual,EXPECTED_UI_SHA256))return IQ_ASSETS_MISMATCHED;
    wcscpy(actual,root);wcscat(actual,L"\\swfs\\swflist.gon.append");
    return iq_file_matches(actual,EXPECTED_APPEND_SHA256)?IQ_ASSETS_READY:IQ_ASSETS_MISMATCHED;
}

__declspec(dllexport) int ImprovedInventoryValidateAssetsW(const wchar_t* dll_path,const wchar_t* command) {
    return iq_assets_status(dll_path,command)==IQ_ASSETS_READY;
}

__declspec(dllexport) int ImprovedInventoryAssetStatusW(const wchar_t* dll_path,const wchar_t* command) {
    return iq_assets_status(dll_path,command);
}
