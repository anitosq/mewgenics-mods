/* Follow Auto Furniture's manager path gate, allowing passive-body edits. */
#include <shellapi.h>

static int canonical_path(const wchar_t* input, wchar_t* output) {
    DWORD n = GetFullPathNameW(input, MAX_PATH, output, NULL);
    if (!n || n >= MAX_PATH) return 0;
    for (DWORD i = 0; i < n; i++) if (output[i] == L'/') output[i] = L'\\';
    while (n > 3 && output[n - 1] == L'\\') output[--n] = 0;
    return 1;
}

static int ability_text_matches(char* text, size_t length) {
    /* Text editors may change LF to CRLF or add a UTF-8 BOM. */
    size_t start = length >= 3 && !memcmp(text, "\xef\xbb\xbf", 3) ? 3 : 0;
    size_t n = 0;
    for (size_t i = start; i < length; i++) if (text[i] != '\r') text[n++] = text[i];
    text[n] = 0;
    const size_t prefix = sizeof(ABILITY_PREFIX) - 1, suffix = sizeof(ABILITY_SUFFIX) - 1;
    if (n < prefix + suffix || memcmp(text, ABILITY_PREFIX, prefix)) return 0;
    /* Do not let a brace in a quoted string or comment close the editable body. */
    unsigned depth = 1;
    int quote = 0, line = 0, block = 0;
    for (size_t i = prefix; i < n; i++) {
        char c = text[i], next = i + 1 < n ? text[i + 1] : 0;
        if (line) { if (c == '\n') line = 0; continue; }
        if (block) { if (c == '*' && next == '/') { block = 0; i++; } continue; }
        if (quote) {
            if (c == '\\') { if (++i == n) return 0; }
            else if (c == '"') quote = 0;
            continue;
        }
        if (c == '"') { quote = 1; continue; }
        if (c == '/' && next == '/') { line = 1; i++; continue; }
        if (c == '/' && next == '*') { block = 1; i++; continue; }
        if (c == '{') depth++;
        if (c == '}' && --depth == 0)
            return n - i == suffix && !memcmp(text + i, ABILITY_SUFFIX, suffix);
        if (!c) return 0;
    }
    return 0;
}

static int ability_file_matches(const wchar_t* path) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    DWORD size = GetFileSize(file, NULL), got = 0;
    char* text = size < 65536 ? malloc((size_t)size + 1) : NULL;
    int ok = text && ReadFile(file, text, size, &got, NULL) && got == size &&
        ability_text_matches(text, size);
    free(text);
    CloseHandle(file);
    return ok;
}

__declspec(dllexport) int SoulWellValidateAssetsW(const wchar_t* dll_path, const wchar_t* command) {
    wchar_t parent[MAX_PATH], sibling[MAX_PATH], actual[MAX_PATH];
    if (!dll_path || !command || !canonical_path(dll_path, parent)) return 0;
    wchar_t* slash = wcsrchr(parent, L'\\');
    if (!slash) return 0;
    if (slash == parent + 2) slash[1] = 0; else *slash = 0;
    if (wcslen(parent) + 10 >= MAX_PATH) return 0;
    wcscpy(sibling, parent); wcscat(sibling, L"\\SoulWell");
    if (!canonical_path(sibling, actual)) return 0;
    wcscpy(sibling, actual);
    int argc = 0, enabled = 0, paths = 0;
    wchar_t** argv = CommandLineToArgvW(command, &argc);
    if (!argv) return 0;
    for (int i = 1; i < argc; i++) {
        if (!wcscmp(argv[i], L"-modpaths")) { paths = 1; continue; }
        if (argv[i][0] == L'-') { paths = 0; continue; }
        if (paths && argv[i][0] && canonical_path(argv[i], actual)) {
            if (!_wcsicmp(parent, actual)) enabled |= 1;
            if (!_wcsicmp(sibling, actual)) enabled |= 2;
        }
    }
    LocalFree(argv);
    if (enabled != 1 && enabled != 2) return 0;
    const wchar_t* root = enabled == 1 ? parent : sibling;
    if (wcslen(root) + 64 >= MAX_PATH) return 0;
    wcscpy(actual, root); wcscat(actual, L"\\data\\items\\trinkets.gon.merge");
    if (!file_matches(actual, EXPECTED_ITEM_SHA256)) return 0;
    wcscpy(actual, root); wcscat(actual, L"\\data\\shops\\tracy_house_shop.gon.patch");
    if (!file_matches(actual, EXPECTED_SHOP_SHA256)) return 0;
    wcscpy(actual, root); wcscat(actual, L"\\data\\abilities\\item_abilities.gon.merge");
    return ability_file_matches(actual);
}
