/* Compare GON tokens so formatting cannot disable the native half of the item. */
#include <shellapi.h>
#include <ctype.h>
#include <limits.h>

typedef struct { const char* text; size_t pos, length; int invalid; } Gon;
typedef struct { const char* text; size_t length; } Token;

static Token next_token(Gon* g) {
    const char* s = g->text;
    while (g->pos < g->length) {
        size_t i = g->pos;
        if (isspace((unsigned char)s[i]) || s[i] == ',' || s[i] == ';') { g->pos++; continue; }
        if (i + 1 < g->length && s[i] == '/' && s[i + 1] == '/') {
            while (g->pos < g->length && s[g->pos] != '\n' && s[g->pos] != '\r') g->pos++;
            continue;
        }
        if (i + 1 < g->length && s[i] == '/' && s[i + 1] == '*') {
            g->pos += 2;
            while (g->pos + 1 < g->length && !(s[g->pos] == '*' && s[g->pos + 1] == '/')) g->pos++;
            if (g->pos + 1 >= g->length) { g->invalid = 1; break; }
            g->pos += 2; continue;
        }
        break;
    }
    size_t start = g->pos;
    if (g->invalid || start == g->length) return (Token){NULL, 0};
    char c = s[g->pos++];
    if (!c) { g->invalid = 1; return (Token){NULL, 0}; }
    if (c == '"') {
        while (g->pos < g->length) {
            c = s[g->pos++];
            if (!c) break;
            if (c == '"') return (Token){s + start, g->pos - start};
            if (c == '\\') {
                if (g->pos == g->length || !s[g->pos]) break;
                g->pos++;
            }
        }
        g->invalid = 1; return (Token){NULL, 0};
    }
    if (!strchr("{}[]", c)) {
        while (g->pos < g->length) {
            c = s[g->pos];
            if (!c || isspace((unsigned char)c) || strchr("{}[],;\"/", c)) break;
            g->pos++;
        }
    }
    return (Token){s + start, g->pos - start};
}
static int token_is(Token t, const char* value) {
    return t.length == strlen(value) && t.text && !memcmp(t.text, value, t.length);
}
static int skip_container(Gon* g, char close) {
    char closes[64] = {close}; unsigned depth = 1;
    while (depth) {
        Token t = next_token(g);
        if (!t.length) return 0;
        if (token_is(t, "{") || token_is(t, "[")) {
            if (depth == sizeof(closes)) return 0;
            closes[depth++] = t.text[0] == '{' ? '}' : ']';
        } else if (token_is(t, "}") || token_is(t, "]")) {
            if (t.text[0] != closes[--depth]) return 0;
        }
    }
    return !g->invalid;
}
static int skip_passives(Gon* g) {
    return token_is(next_token(g), "{") && skip_container(g, '}');
}
static int item_text_matches(const char* text, size_t length) {
    if (memchr(text, 0, length)) return 0;
    size_t start = length >= 3 && !memcmp(text, "\xef\xbb\xbf", 3) ? 3 : 0;
    Gon g = {text, start, length, 0};
    if (!token_is(next_token(&g), "SoulWell") || !token_is(next_token(&g), "{")) return 0;
    unsigned required = 0;
    for (;;) {
        Token key = next_token(&g);
        if (token_is(key, "}")) break;
        if (!key.length || strchr("{}[]\"", key.text[0])) return 0;
        Token value = next_token(&g);
        if (!value.length || token_is(value, "}") || token_is(value, "]")) return 0;
        unsigned bit = token_is(key, "kind") ? 1 : token_is(key, "ability") ? 2 : token_is(key, "aux") ? 4 : 0;
        if (bit) {
            const char* expected = bit == 1 ? "trinket" : bit == 2 ? "tk_SoulWell" : "-1";
            if ((required & bit) || !token_is(value, expected)) return 0;
            required |= bit;
        } else if (token_is(value, "{") || token_is(value, "[")) {
            if (!skip_container(&g, value.text[0] == '{' ? '}' : ']')) return 0;
        }
    }
    return !next_token(&g).length && !g.invalid && required == 7;
}
static int positive_range(Token t) {
    unsigned value = 0;
    if (!t.length) return 0;
    for (size_t i = 0; i < t.length; i++) {
        unsigned digit = (unsigned char)t.text[i] - (unsigned)'0';
        if (digit > 9 || value > ((unsigned)INT_MAX - digit) / 10) return 0;
        value = value * 10 + digit;
    }
    return value > 0;
}
static int data_text_matches(const char* text, size_t length, const char* expected, int editable) {
    if (memchr(text, 0, length)) return 0;
    size_t start = length >= 3 && !memcmp(text, "\xef\xbb\xbf", 3) ? 3 : 0;
    Gon actual = {text, start, length, 0}, model = {expected, 0, strlen(expected), 0};
    for (;;) {
        Token a = next_token(&actual), b = next_token(&model);
        if (actual.invalid || model.invalid || a.length != b.length) return 0;
        if (!a.length) return 1;
        if (memcmp(a.text, b.text, a.length)) return 0;
        if (editable && token_is(b, "max_range")) {
            if (!positive_range(next_token(&actual)) || !positive_range(next_token(&model))) return 0;
        } else if (editable && token_is(b, "additional_passives")) {
            if (!skip_passives(&actual) || !skip_passives(&model)) return 0;
        }
    }
}
static int ability_text_matches(const char* text, size_t length) {
    return data_text_matches(text, length, ABILITY_TEXT, 1);
}
static int data_file_matches(const wchar_t* path, const char* expected, int editable) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    DWORD size = GetFileSize(file, NULL), got = 0;
    char* text = size < 65536 ? malloc((size_t)size + 1) : NULL;
    int ok = text && ReadFile(file, text, size, &got, NULL) && got == size &&
        (editable == 2 ? item_text_matches(text, size) :
         editable == 1 ? ability_text_matches(text, size) : data_text_matches(text, size, expected, 0));
    free(text); CloseHandle(file);
    return ok;
}

enum StartupStatus {
    SW_PATH = 0, SW_READY = 1, SW_DISABLED, SW_AMBIGUOUS, SW_ITEM, SW_SHOP, SW_ABILITY,
    SW_LOADER, SW_GAME, SW_ENTRY, SW_HOOK
};
static const char* startup_reason(int status) {
    switch (status) {
        case SW_READY: return "Ready.";
        case SW_PATH: return "The Soul Well installation path could not be read.";
        case SW_DISABLED: return "Soul Well data is not enabled in -modpaths. Enable Soul Well in your manager and launch through it.";
        case SW_AMBIGUOUS: return "Two possible Soul Well data folders are enabled. Keep one installation.";
        case SW_ITEM: return "data/items/trinkets.gon.merge is missing or incompatible. Keep one SoulWell definition with kind trinket, ability tk_SoulWell and aux -1. Other item fields may change. Check braces and values, or restore it from this beta ZIP.";
        case SW_SHOP: return "data/shops/tracy_house_shop.gon.patch is missing or incompatible. Restore it from this beta ZIP.";
        case SW_ABILITY: return "data/abilities/item_abilities.gon.merge is missing or incompatible. max_range must be a positive integer. Only max_range and additional_passives may change; restore other settings from this beta ZIP.";
        case SW_LOADER: return "Mewjector API 3 or newer is required. Install or enable Mewjector.";
        case SW_GAME: return "This DLL supports Mewgenics 1.1.21239 (Steam build 25143593) only. The executable does not match.";
        case SW_ENTRY: return "A required game function differs from the supported build, possibly because another DLL changed it. Check the log for its address.";
        case SW_HOOK: return "A required native hook could not be installed. Check mod_logs/chainloader.log for the loader error.";
        default: return "Unknown startup failure.";
    }
}
static int canonical_path(const wchar_t* input, wchar_t* output) {
    DWORD n = GetFullPathNameW(input, MAX_PATH, output, NULL);
    if (!n || n >= MAX_PATH) return 0;
    for (DWORD i = 0; i < n; i++) if (output[i] == L'/') output[i] = L'\\';
    while (n > 3 && output[n - 1] == L'\\') output[--n] = 0;
    return 1;
}
__declspec(dllexport) int SoulWellAssetStatusW(const wchar_t* dll_path, const wchar_t* command) {
    wchar_t parent[MAX_PATH], sibling[MAX_PATH], actual[MAX_PATH];
    if (!dll_path || !command || !canonical_path(dll_path, parent)) return SW_PATH;
    wchar_t* slash = wcsrchr(parent, L'\\');
    if (!slash) return SW_PATH;
    if (slash == parent + 2) slash[1] = 0; else *slash = 0;
    if (wcslen(parent) + 10 >= MAX_PATH) return SW_PATH;
    wcscpy(sibling, parent); wcscat(sibling, L"\\SoulWell");
    if (!canonical_path(sibling, actual)) return SW_PATH;
    wcscpy(sibling, actual);
    int argc = 0, enabled = 0, paths = 0;
    wchar_t** argv = CommandLineToArgvW(command, &argc);
    if (!argv) return SW_PATH;
    for (int i = 1; i < argc; i++) {
        if (!wcscmp(argv[i], L"-modpaths")) { paths = 1; continue; }
        if (argv[i][0] == L'-') { paths = 0; continue; }
        if (paths && argv[i][0] && canonical_path(argv[i], actual)) {
            if (!_wcsicmp(parent, actual)) enabled |= 1;
            if (!_wcsicmp(sibling, actual)) enabled |= 2;
        }
    }
    LocalFree(argv);
    if (!enabled) return SW_DISABLED;
    if (enabled == 3) return SW_AMBIGUOUS;
    const wchar_t* root = enabled == 1 ? parent : sibling;
    if (wcslen(root) + 64 >= MAX_PATH) return SW_PATH;
    wcscpy(actual, root); wcscat(actual, L"\\data\\items\\trinkets.gon.merge");
    if (!data_file_matches(actual, ITEM_TEXT, 2)) return SW_ITEM;
    wcscpy(actual, root); wcscat(actual, L"\\data\\shops\\tracy_house_shop.gon.patch");
    if (!data_file_matches(actual, SHOP_TEXT, 0)) return SW_SHOP;
    wcscpy(actual, root); wcscat(actual, L"\\data\\abilities\\item_abilities.gon.merge");
    return data_file_matches(actual, ABILITY_TEXT, 1) ? SW_READY : SW_ABILITY;
}
__declspec(dllexport) int SoulWellValidateAssetsW(const wchar_t* dll_path, const wchar_t* command) {
    return SoulWellAssetStatusW(dll_path, command) == SW_READY;
}

typedef int (WINAPI *Notice)(HWND, LPCSTR, LPCSTR, UINT);
static void display_startup_failure(int status, Notice show) {
    char message[1400];
    snprintf(message, sizeof(message), "Soul Well %s did not activate.\n\n%s\n\nDo not use Soul Well until this is fixed. If you continue, the item may still appear in-game.\n\nDetails: mod_logs/chainloader.log\nClose the game, fix the issue and restart.", SW_VERSION, startup_reason(status));
    show(NULL, message, "Soul Well is inactive", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
}
static HMODULE notice_module;
static DWORD WINAPI failure_worker(void* context) {
    display_startup_failure((int)(INT_PTR)context, MessageBoxA);
    FreeLibraryAndExitThread(notice_module, 0);
}
static void report_startup_failure(int status) {
    if (logger) logger("SoulWell", "INACTIVE [%d]: %s", status, startup_reason(status));
    /* Keep the DLL alive for the notice; never run User32 or wait under loader lock. */
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)(void*)failure_worker, &notice_module)) return;
    HANDLE thread = CreateThread(NULL, 0, failure_worker, (void*)(INT_PTR)status, 0, NULL);
    if (thread) CloseHandle(thread);
    /* A failed CreateThread retains one module reference until process exit. */
    else if (logger) logger("SoulWell", "Could not display startup error (Windows error %lu).", GetLastError());
}
