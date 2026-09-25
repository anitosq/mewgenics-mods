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

static int iq_assets_enabled(const wchar_t* dll_path,const wchar_t* command) {
    wchar_t root[MAX_PATH],wanted[MAX_PATH],actual[MAX_PATH];
    if(!iq_canonical_path(dll_path,root))return 0;
    wchar_t* slash=wcsrchr(root,L'\\');
    if(!slash || (size_t)(slash-root)+32>=MAX_PATH)return 0;
    wcscpy(slash+1,L"ImprovedInventory");
    if(!iq_canonical_path(root,wanted))return 0;
    int argc=0,enabled=0,paths=0;
    wchar_t** argv=CommandLineToArgvW(command,&argc);
    if(!argv)return 0;
    for(int i=1;i<argc;i++) {
        if(!wcscmp(argv[i],L"-modpaths")) {paths=1;continue;}
        if(argv[i][0]==L'-') {paths=0;continue;}
        if(paths && iq_canonical_path(argv[i],actual) && !_wcsicmp(wanted,actual))enabled=1;
    }
    LocalFree(argv);
    if(!enabled)return 0;
    if(wcslen(root)+40>=MAX_PATH)return 0;
    wcscpy(actual,root);wcscat(actual,L"\\swfs\\improved_inventory.swf");
    if(!iq_file_matches(actual,EXPECTED_UI_SHA256))return 0;
    wcscpy(actual,root);wcscat(actual,L"\\swfs\\swflist.gon.append");
    return iq_file_matches(actual,EXPECTED_APPEND_SHA256);
}

__declspec(dllexport) int ImprovedInventoryValidateAssetsW(const wchar_t* dll_path,const wchar_t* command) {
    return dll_path && command && iq_assets_enabled(dll_path,command);
}
