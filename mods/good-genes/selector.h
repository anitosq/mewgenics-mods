/* Current-executable native YesNoPrompt/MenuPanel integration. No OS dialogs. */
#include "ui-font.h"

typedef struct { union { wchar_t local[8]; const wchar_t *pointer; }; size_t size, capacity; } WideString;
typedef struct { byte storage[56]; void *callable; } GameFunction;
typedef struct { void **vtable; uint64_t ticket; int action; } Choice;
_Static_assert(sizeof(wchar_t) == 2 && sizeof(GameFunction) == 64 && sizeof(Choice) <= 56, "Native ABI mismatch");
typedef struct { int part, old; unsigned offset; } PartSnapshot;
typedef struct {
    void *cat;
    byte *origin, *manager;
    uint64_t pair, generation, ticket;
    byte identity[8];
    PartSnapshot parts[15];
    unsigned count;
} Offer;

static void (*original_panel)(void *, GameString *);
static void (*original_close)(void *, GameFunction *);
static void *(*original_destroy)(void *, unsigned);
static void (*original_cat_destroy)(void *);
static void (*original_button_update)(void *);
static _Thread_local int creating_selector;
/* ponytail: bounded FIFO keeps hooks allocation-free; raise only for proven larger effects. */
static Offer offers[64], active;
static unsigned queue_head, queue_count, view_part, view_page;
static unsigned view_indices[15], view_count;
static int navigation_needed;
/* Attached clips are owned by the modal's display tree, never the live cat. */
static byte *part_art[2][9];
static byte *stat_art[2][MUTATION_STATS];
static uint64_t next_ticket;
static byte *active_prompt;
static int pumping, closing;
static DWORD owner_thread;
static byte *intro_button;
static ULONGLONG intro_started;
static float intro_matrix[6];
static float panel_center_y;

static GameString small_string(const char *text) {
    GameString s = {.capacity = 15};
    FN(void (*)(GameString *, const char *, size_t), 0x520d0)(&s, text, strlen(text));
    return s;
}

static WideString wide_string(const wchar_t *text) {
    WideString s = {.capacity = 7};
    FN(void (*)(WideString *, const wchar_t *, size_t), 0x5b150)(&s, text, wcslen(text));
    return s;
}

static int copy_wide(const void *source, wchar_t *out, size_t capacity) {
    WideString s;
    if (!read_bytes(source, &s, sizeof(s)) || s.size >= capacity || s.capacity < s.size) return 0;
    if (s.capacity <= 7) memcpy(out, s.local, s.size * 2);
    else if (!read_bytes(s.pointer, out, s.size * 2)) return 0;
    out[s.size] = 0;
    return 1;
}

static int offer_alive(const Offer *offer) {
    byte identity[8], dead, leaving;
    uint64_t generation;
    uintptr_t begin, end;
    if (!offer->cat || !offer->origin || !offer->manager ||
        !read_bytes(offer->manager, &begin, 8) || !read_bytes(offer->manager + 8, &end, 8) ||
        end < begin || (end-begin) % 8 || end-begin > 8192) return 0;
    int found = 0;
    for (uintptr_t p = begin; p < end; p += 8) {
        byte *scene;
        if (!read_bytes((void *)p, &scene, 8)) return 0;
        if (scene == offer->origin) { found = 1; break; }
    }
    return found && read_bytes(offer->origin - 8, &generation, 8) && generation == offer->generation &&
        read_bytes(offer->origin + 0x4b0, &dead, 1) && !dead &&
        read_bytes(offer->origin + 0x4db, &leaving, 1) && !leaving &&
        read_bytes(offer->cat, identity, sizeof(identity)) && !memcmp(identity, offer->identity, sizeof(identity));
}

/* Match every write of SetPiece, not just one side of a paired part. */
static MutationDecision snapshot_offer(Offer *offer) {
    static const struct { int part; unsigned offset; } slots[] = {
        {0,0x90}, {1,0xe4}, {2,0x138}, {11,0x18c}, {12,0x1e0},
        {13,0x234}, {14,0x288}, {15,0x2dc}, {16,0x330}, {17,0x384},
        {18,0x3d8}, {19,0x42c}, {20,0x480}, {9,0x4d4}, {10,0x78}
    };
    int part = (int)(uint32_t)offer->pair, id = (int)(offer->pair >> 32);
    if (!offer->cat || part < 0 || part > 20 || id < 0) return KEEP_MUTATION;
    int changed = 0;
    offer->count = 0;
    MutationDecision result = VANILLA_MUTATION;
    for (unsigned i = 0; i < COUNT(slots); ++i) {
        int p = slots[i].part;
        int selected = part == 10 || part == p ||
            (part == 3 && (p == 11 || p == 12)) || (part == 4 && (p == 13 || p == 14)) ||
            (part == 5 && p >= 11 && p <= 14) || (part == 6 && (p == 15 || p == 16)) ||
            (part == 7 && (p == 17 || p == 18)) || (part == 8 && (p == 19 || p == 20));
        if (!selected) continue;
        PartSnapshot *slot = &offer->parts[offer->count++];
        slot->part = part == 10 ? 10 : p;
        slot->offset = slots[i].offset + (part == 10 && p != 10 ? 4 : 0);
        if (!read_bytes((byte *)offer->cat + slot->offset, &slot->old, 4)) return KEEP_MUTATION;
        if (slot->old == id) continue;
        changed = 1;
        MutationQuality old = quality(slot->part, slot->old);
        MutationDecision decision = mutation_decision(old, quality(slot->part, id));
        if (decision == KEEP_MUTATION) result = KEEP_MUTATION;
        else if (decision == CHOOSE_MUTATION && result != KEEP_MUTATION) result = CHOOSE_MUTATION;
    }
    if (!changed || !offer->count) return KEEP_MUTATION;
    return result;
}

static int snapshot_unchanged(void) {
    if (!offer_alive(&active)) return 0;
    for (unsigned i = 0; i < active.count; ++i) {
        int old;
        if (!read_bytes((byte *)active.cat + active.parts[i].offset, &old, 4) || old != active.parts[i].old) return 0;
    }
    return 1;
}

static const wchar_t *part_name(int part) {
    static const wchar_t *names[] = {L"Body",L"Head",L"Tail",L"Arms",L"Legs",L"Limbs",L"Eyes",L"Eyebrows",
        L"Ears",L"Mouth",L"Fur",L"Left arm",L"Right arm",L"Left leg",L"Right leg",L"Left eye",L"Right eye",
        L"Left eyebrow",L"Right eyebrow",L"Left ear",L"Right ear"};
    return part >= 0 && part < (int)COUNT(names) ? names[part] : L"Mutation";
}

static void append_text(wchar_t *out, size_t capacity, const wchar_t *format, ...) {
    size_t n = wcslen(out);
    if (n + 1 >= capacity) return;
    va_list args;
    va_start(args, format);
    vswprintf(out + n, capacity - n, format, args);
    va_end(args);
    out[capacity - 1] = 0;
}

static byte *definition_field(byte *definition, const char *key) {
    GameString name = small_string(key);
    byte *value = FN(byte *(*)(void *, GameString *), 0x942da0)(definition, &name);
    FN(void (*)(GameString *), 0x52780)(&name);
    return value;
}

static int describe_mutation(int part, int id, wchar_t out[4096]) {
    out[0] = 0;
    MutationQuality q = quality(part, id);
    if (q.kind == UNMUTATED) { wcscpy(out, L"No mutation"); return 1; }
    if (q.kind == UNKNOWN) { wcscpy(out, L"Unknown mutation"); return 0; }
    /* Private markers are replaced by native icons before text reaches the UI. */
    for (unsigned i = 0; i < MUTATION_STATS; ++i)
        if (q.stats[i]) append_text(out, 4096, L"%+.6g %lc\n", q.stats[i], (wint_t)(0xe000+i));
    byte *definition = definition_for(part, id);
    if (!definition) return 0;
    int described = 1;
    static const char *keys[] = {"shield", "divine_shield"};
    static const wchar_t *labels[] = {L"Shield", L"Divine Shield"};
    for (unsigned i = 0; i < COUNT(keys); ++i) {
        byte *value = definition_field(definition, keys[i]);
        int type;
        double number;
        if (value && read_bytes(value + 0xa8, &type, 4) && type == 2 &&
            read_bytes(value + 0x58, &number, 8) && isfinite(number))
            append_text(out, 4096, L"%+.6g %ls\n", number, labels[i]);
    }
    byte *desc = definition_field(definition, "desc");
    char key[128];
    int type;
    if (desc && read_bytes(desc + 0xa8, &type, 4) && type == 1 && game_string(desc + 0x68, key)) {
        GameString name = small_string(key);
        WideString localized = {.capacity = 7};
        FN(void (*)(void *, WideString *, GameString *), 0x962430)(base + 0x13c5530, &localized, &name);
        wchar_t text[3072];
        if (copy_wide(&localized, text, COUNT(text))) append_text(out, 4096, L"%ls%ls", *out ? L"\n" : L"", text);
        else { append_text(out, 4096, L"\nDescription unavailable."); described = 0; }
        FN(void (*)(WideString *), 0x52290)(&localized);
        FN(void (*)(GameString *), 0x52780)(&name);
    }
    if (q.kind == BIRTH_DEFECT) append_text(out, 4096, L"\nBirth defect");
    return described;
}

static double glyph_width(wchar_t code) {
    if (code >= 0xe000 && code < 0xe000+MUTATION_STATS) return 26;
    size_t lo = 0, hi = COUNT(glyphs);
    while (lo < hi) {
        size_t mid = lo + (hi-lo)/2;
        if (glyphs[mid].code < (unsigned)code) lo = mid + 1; else hi = mid;
    }
    return (lo < COUNT(glyphs) && glyphs[lo].code == (unsigned)code ? glyphs[lo].width : 1.0) * 20;
}

/* Four 26px rows at 20px text size; full effects remain available by paging. */
static unsigned text_page(const wchar_t *text, unsigned wanted, wchar_t out[4096]) {
    unsigned line = 0;
    size_t used = 0;
    out[0] = 0;
    while (*text) {
        const wchar_t *start = text, *space = NULL;
        double width = 0;
        while (*text && *text != L'\n' && width + glyph_width(*text) <= 252) {
            width += glyph_width(*text);
            if (*text == L' ') space = text;
            ++text;
        }
        if (*text && *text != L'\n' && space && space > start) text = space;
        if (text == start && *text && *text != L'\n') ++text;
        if (line / 4 == wanted) {
            size_t n = (size_t)(text - start);
            if (used + n + 2 < 4096) {
                memcpy(out + used, start, n * sizeof(wchar_t));
                used += n;
                out[used++] = L'\n';
                out[used] = 0;
            }
        }
        if (*text == L'\n') ++text;
        else while (*text == L' ') ++text;
        ++line;
    }
    return line ? (line + 3)/4 : 1;
}

static void panel_text(const char *name, const wchar_t *text) {
    if (!active_prompt) return;
    byte *panel = *(byte **)(active_prompt + 0x40);
    GameString key = small_string(name);
    WideString value = wide_string(text);
    FN(void (*)(void *, GameString *, WideString *), 0x97c6f0)(panel, &key, &value);
}

static byte *display_child(byte *clip, const char *name) {
    if (!clip) return NULL;
    GameString key = small_string(name);
    byte *child = FN(byte *(*)(void *, GameString *), 0x99a0e0)(clip, &key);
    FN(void (*)(GameString *), 0x52780)(&key);
    return child;
}

static byte *panel_child(const char *name) {
    byte *panel = *(byte **)(active_prompt + 0x40);
    byte *renderer = *(byte **)(panel + 0x38);
    return display_child(*(byte **)(renderer + 0x80), name);
}

static int fit_clip(byte *clip, float x, float y, float width, float height, int stretch) {
    float bounds[4]; /* Native AABB: min X, max X, min Y, max Y. */
    void **vtable = *(void ***)clip;
    /* Slot 16 clips texture bounds to masks and excludes hidden overlays.
       Slot 15 includes them, making small body parts appear miniature. */
    ((float *(*)(void *, float *))vtable[16])(clip, bounds);
    for (unsigned i = 0; i < COUNT(bounds); ++i) if (!isfinite(bounds[i])) return 0;
    double w = bounds[1] - bounds[0], h = bounds[3] - bounds[2];
    if (w <= 0 || h <= 0) return 0;
    float sx = width/(float)w, sy = height/(float)h;
    if (!stretch) sx = sy = fminf(sx, sy);
    float matrix[6] = {sx, sy, 0, 0,
        x - sx*(bounds[0]+bounds[1])/2, y - sy*(bounds[2]+bounds[3])/2};
    memcpy(clip + 0x60, matrix, sizeof(matrix));
    clip[8] |= 0x60;
    return 1;
}

static int fit_art(byte *clip, float x, float y, float width, float height) {
    return fit_clip(clip, x, y, width, height, 0);
}

static byte *attach_art(byte *holder, const char *symbol) {
    if (!holder) return NULL;
    GameString name = small_string(symbol);
    byte *clip = FN(byte *(*)(void *, GameString *), 0x9bb890)(NULL, &name);
    if (clip) {
        FN(void (*)(void *, void *, byte), 0x999b90)(holder, clip, 1);
        void **vtable = *(void ***)clip;
        ((void (*)(void *))vtable[3])(clip);
        clip[8] &= (byte)~0x20;
    }
    return clip;
}

static void render_effects(unsigned side, const wchar_t *text) {
    static const char *symbols[] = {"FontIcon_str", "FontIcon_dex", "FontIcon_con", "FontIcon_int",
                                    "FontIcon_spd", "FontIcon_cha", "FontIcon_lck"};
    static const wchar_t *labels[] = {L"STR", L"DEX", L"CON", L"INT", L"SPD", L"CHA", L"LCK"};
    for (unsigned i = 0; i < MUTATION_STATS; ++i)
        if (stat_art[side][i]) stat_art[side][i][8] &= (byte)~0x20;
    for (unsigned row = 0; row < 4; ++row) {
        wchar_t line[4096] = {0};
        unsigned length = 0;
        unsigned row_icons = 0;
        double advance = 0;
        while (*text && *text != L'\n' && length+4 < COUNT(line)) {
            wchar_t code = *text++;
            if (code >= 0xe000 && code < 0xe000+MUTATION_STATS) {
                unsigned stat = (unsigned)(code-0xe000);
                byte **icon = &stat_art[side][stat];
                if (!*icon) *icon = attach_art(panel_child(side ? "new_art" : "old_art"), symbols[stat]);
                if (*icon) FN(void (*)(void *, double, double, double, double), 0x9bf090)
                    (*icon+0x78, 0.0, 0.0, 0.0, 1.0);
                if (!*icon || !fit_art(*icon, (side ? 664 : 344)+(float)advance+11,
                                      381+row*26, 22, 22)) {
                    /* Keep the effect readable if another mod removes an icon. */
                    wcscpy(line+length, labels[stat]);
                    length += 3;
                    for (const wchar_t *p = labels[stat]; *p; ++p) advance += glyph_width(*p);
                } else {
                    row_icons |= 1u << stat;
                    advance += 22;
                }
            } else {
                line[length++] = code;
                advance += glyph_width(code);
            }
        }
        if (*text == L'\n') ++text;
        char field[8];
        snprintf(field, sizeof(field), "%s%u", side ? "inc" : "cur", row);
        panel_text(field, line);
        /* Center the whole line, including any separately rendered stat icon. */
        float shift = (272-(float)advance)/2;
        byte *text_clip = panel_child(field);
        if (text_clip) *(float *)(text_clip+0x70) = shift;
        for (unsigned stat = 0; stat < MUTATION_STATS; ++stat)
            if (row_icons & (1u << stat)) *(float *)(stat_art[side][stat]+0x70) += shift;
    }
}

static int art_frame(byte *clip, int id) {
    if (!clip || id < 1) return 0;
    void **vtable = *(void ***)clip;
    int type = ((int (*)(void *))vtable[0])(clip);
    byte *definition;
    int frames;
    if ((type != 1 && type != 2) || !read_bytes(clip + 0xd0, &definition, 8) ||
        !read_bytes(definition, &frames, 4) || id > frames) return 0;
    /* CatPart uses one-based IDs, unlike the zero-based MovieClip API. */
    FN(void (*)(void *, int), 0x9a88a0)(clip, id - 1);
    clip[9] &= (byte)~2;
    ((void (*)(void *))vtable[3])(clip);
    return 1;
}

static byte *first_clip(byte *clip) {
    if (!clip || !*(unsigned *)(clip+0xac)) return NULL;
    byte **children = *(unsigned *)(clip+0xa8) > 4 ? *(byte ***)(clip+0xb0) : (byte **)(clip+0xb0);
    return children[0];
}

/* Match DefineSprite::instantiate: pooled MovieClip plus shared timeline data. */
static byte *clone_clip(byte *holder, byte *source) {
    if (!holder || !source) return NULL;
    void **vtable = *(void ***)source;
    int type = ((int (*)(void *))vtable[0])(source);
    if (type != 1 && type != 2) return NULL;
    byte *timeline = NULL;
    int frames = 0;
    if (!read_bytes(source+0xd0, &timeline, sizeof(timeline)) ||
        !timeline || !read_bytes(timeline, &frames, sizeof(frames)) || frames < 1) return NULL;
    /* MovieClip+0xd0 is raw timeline data, NOT the polymorphic library asset.
       Native factory a583d0 allocates from this pool and calls this constructor. */
    void *storage = FN(void *(*)(void *), 0x9694c0)(base+0x1420030);
    if (!storage) return NULL;
    byte *clip = FN(byte *(*)(void *, void *), 0x9a7c00)(storage, timeline);
    if (clip) {
        FN(void (*)(void *, void *, byte), 0x999b90)(holder, clip, 1);
        art_frame(clip, 1);
    }
    return clip;
}

static byte *selector_button(const char *name) {
    byte *panel = *(byte **)(active_prompt+0x40);
    GameString key = small_string(name);
    /* Only called for names already registered by MenuPanel. */
    byte **entry = FN(byte **(*)(void *, GameString *), 0x299e50)(panel+0x40, &key);
    FN(void (*)(GameString *), 0x52780)(&key);
    return entry ? *entry : NULL;
}

static void skin_selector(unsigned effect_rows) {
    /* Reserve only the rows this offer needs; keep the footer stable while paging. */
    float effects_bottom = 368 + effect_rows*26;
    float button_y = effects_bottom + (navigation_needed ? 41 : 20);
    float height = button_y + 30 + 20 - 170;
    panel_center_y = 170 + height/2;
    static const char *actions[] = {"no", "yes", "no_caption", "yes_caption"};
    for (unsigned i = 0; i < COUNT(actions); ++i) {
        byte *child = panel_child(actions[i]);
        if (child) *(float *)(child+0x74) += button_y-512;
    }
    static const char *navigation[] = {"previous", "next", "page"};
    for (unsigned i = 0; i < COUNT(navigation); ++i) {
        byte *child = panel_child(navigation[i]);
        if (child) *(float *)(child+0x74) += effects_bottom+8-479;
    }
    byte *divider = panel_child("effect_divider");
    if (divider) {
        float scale = (effects_bottom-248)/224;
        *(float *)(divider+0x64) = scale;
        *(float *)(divider+0x74) = 248*(1-scale);
    }
    byte *fallback = panel_child("fallback_paper");
    if (fallback) {
        *(float *)(fallback+0x64) = height/380;
        *(float *)(fallback+0x74) = 170*(1-height/380);
    }
    byte *holder = panel_child("native_template");
    byte *sample = attach_art(holder, "ConfirmationBox");
    if (sample) art_frame(sample, 1);
    if (holder) holder[8] &= (byte)~0x20;
    byte *paper = clone_clip(panel_child("native_paper"), first_clip(first_clip(sample)));
    if (paper && fit_clip(paper, 640, panel_center_y, 640, height, 1)) {
        if (fallback) fallback[8] &= (byte)~0x20;
    }
    for (unsigned i = 0; i < 4; ++i) {
        if (i >= 2 && !navigation_needed) continue;
        const char *name = i == 0 ? "no" : i == 1 ? "yes" : i == 2 ? "previous" : "next";
        byte *button = selector_button(name);
        if (!button) continue;
        const char *sound = "Event_ChoiceButton";
        FN(void (*)(GameString *, const char *, size_t), 0x520d0)
            ((GameString *)(button+0x1f8), sound, strlen(sound));
        if (i == 0) intro_button = button;
    }
}

static void selector_intro(void) {
    if (!active_prompt || !intro_button) return;
    float t = (float)(GetTickCount64()-intro_started)/160;
    if (t > 1) t = 1;
    float scale = 1 - 0.12f*(1-t)*(1-t)*(1-t);
    byte *panel = *(byte **)(active_prompt+0x40);
    byte *root = *(byte **)(*(byte **)(panel+0x38)+0x80);
    float matrix[6];
    memcpy(matrix, intro_matrix, sizeof(matrix));
    for (unsigned i = 0; i < 4; ++i) matrix[i] *= scale;
    matrix[4] += (1-scale)*(intro_matrix[0]*640 + intro_matrix[3]*panel_center_y);
    matrix[5] += (1-scale)*(intro_matrix[2]*640 + intro_matrix[1]*panel_center_y);
    memcpy(root+0x60, matrix, sizeof(matrix));
    if (t == 1) intro_button = NULL;
}

static void selector_button_update(void *button) {
    if (button == intro_button) selector_intro();
    original_button_update(button);
}

static int render_part_art(unsigned side, PartSnapshot *part, int id) {
    static const char *symbols[] = {"CatBody", "CatHead", "CatTail", "CatLeg", "CatEye",
                                    "CatEye_Right", "CatEyebrow", "CatEar", "CatMouth"};
    int p = part->part;
    unsigned symbol = p == 10 ? 0 : p <= 2 ? (unsigned)p : p == 9 ? 8 :
        p <= 14 ? 3 : p == 15 ? 4 : p == 16 ? 5 : p <= 18 ? 6 : 7;
    for (unsigned i = 0; i < COUNT(symbols); ++i)
        if (part_art[side][i]) part_art[side][i][8] &= (byte)~0x20;
    byte *holder = panel_child(side ? "new_art" : "old_art");
    if (!holder || !active.cat) return 0;
    byte *clip = part_art[side][symbol];
    if (!clip) {
        clip = attach_art(holder, symbols[symbol]);
        if (!clip) return 0;
        part_art[side][symbol] = clip;
        clip[8] &= (byte)~0x20;
    }
    int shape = id, coat;
    unsigned offset = p == 10 ? 0x90 : part->offset;
    if (p == 10) {
        coat = id;
        if (!read_bytes((byte *)active.cat + offset, &shape, 4)) return 0;
    } else if (!read_bytes((byte *)active.cat + offset + 4, &coat, 4)) return 0;
    if (!art_frame(clip, shape)) return 0;
    byte *texture = display_child(clip, "tex");
    if (texture && !art_frame(texture, coat)) return 0;
    if (p == 10 && !texture) return 0; /* Never substitute a fake coat preview. */
    static const char *details[] = {"scars", "aux", "greyhair", "wrinkles"};
    for (unsigned i = 0; i < COUNT(details); ++i) {
        byte *child = display_child(i < 2 ? clip : texture, details[i]);
        if (child) child[8] &= (byte)~0x20;
    }
    /* Reuse the part's native color transform, but enlarge geometry independently. */
    float color[8];
    if (!read_bytes((byte *)active.cat + offset + 0x30, color, sizeof(color))) return 0;
    for (unsigned i = 0; i < COUNT(color); ++i) if (!isfinite(color[i])) return 0;
    memcpy(clip + 0x78, color, sizeof(color));
    return fit_art(clip, side ? 800 : 480, 319, 157.5f, 69);
}

static unsigned render_comparison(void) {
    PartSnapshot *part = &active.parts[view_indices[view_part]];
    wchar_t current[4096], incoming[4096], a[4096], b[4096], title[256];
    describe_mutation(part->part, part->old, current);
    describe_mutation(part->part, (int)(active.pair >> 32), incoming);
    unsigned pages_a = text_page(current, view_page, a), pages_b = text_page(incoming, view_page, b);
    unsigned pages = pages_a > pages_b ? pages_a : pages_b;
    panel_text("old_art_note", render_part_art(0, part, part->old) ? L"" : L"Preview unavailable");
    panel_text("new_art_note", render_part_art(1, part, (int)(active.pair >> 32)) ? L"" : L"Preview unavailable");
    render_effects(0, a);
    render_effects(1, b);
    swprintf(title, COUNT(title), L"%ls", part_name(part->part));
    panel_text("part", title);
    swprintf(title, COUNT(title), L"Comparison %u/%u  |  Page %u/%u", view_part+1, view_count, view_page+1, pages);
    panel_text("page", navigation_needed ? title : L"");
    return pages;
}

static void show_next_offer(void);
static void choice_invoke(Choice *choice) {
    if (choice->ticket != active.ticket || !active_prompt) return;
    if (choice->action >= 2) {
        unsigned pages = render_comparison();
        if (choice->action == 3) {
            if (++view_page >= pages) { view_page = 0; view_part = (view_part+1) % view_count; }
        } else if (view_page) --view_page;
        else {
            view_part = (view_part + view_count - 1) % view_count;
            view_page = 0;
            view_page = render_comparison() - 1;
        }
        render_comparison();
        return;
    }
    Offer verify = active;
    if (choice->action == 1 && snapshot_unchanged() && snapshot_offer(&verify) != KEEP_MUTATION) {
        original_set(active.cat, active.pair);
        log_message(OWNER, "Accepted mutation: part=%u incoming=%u.", (unsigned)active.pair, (unsigned)(active.pair >> 32));
    } else log_message(OWNER, "Kept existing mutation (declined or source changed).");
    active_prompt = NULL;
    memset(&active, 0, sizeof(active));
    show_next_offer();
}

/* MSVC's small std::function<void()> ABI. All game copies supply inline storage. */
static void *choice_copy(Choice *source, void *destination) {
    if (!destination) destination = FN(void *(*)(size_t), 0xd3e0a4)(sizeof(Choice));
    memcpy(destination, source, sizeof(Choice));
    return destination;
}
static const void *choice_type(void *self) {
    (void)self;
    static struct { void *vtable, *cached_name; char name[32]; } info = {NULL, NULL, ".?AVGoodGenesMutationChoice@@"};
    /* Type descriptor returned by the native prompt lambda's _Target_type. */
    info.vtable = *(void **)(base + 0x13184f0);
    return &info;
}
static void choice_delete(void *self, byte free_memory) {
    if (free_memory) FN(void (*)(void *, size_t), 0xd3e10c)(self, sizeof(Choice));
}
static void *choice_target(void *self) { return (byte *)self + 8; }
static void *choice_vtable[] = {(void *)choice_copy, (void *)choice_copy, (void *)choice_invoke,
    (void *)choice_type, (void *)choice_delete, (void *)choice_target};

static void init_choice(GameFunction *function, int action) {
    memset(function, 0, sizeof(*function));
    Choice choice = {choice_vtable, active.ticket, action};
    memcpy(function->storage, &choice, sizeof(choice));
    function->callable = function->storage;
}

static void destroy_function(GameFunction *function) {
    if (function->callable) {
        void **vtable = *(void ***)function->callable;
        ((void (*)(void *, byte))vtable[4])(function->callable, function->callable != (void *)function);
        function->callable = NULL;
    }
}

static int selector_available(void) {
    void *library = NULL;
    if (!read_bytes(base + 0x13c4a30, &library, 8) || !library) return 0;
    GameString symbol = small_string("GoodGenesMutationSelector");
    return FN(void *(*)(void *, GameString *), 0x9bb6f0)(library, &symbol) != NULL;
}

static void show_next_offer(void) {
    if (pumping || active_prompt) return;
    pumping = 1;
    while (queue_count && !active_prompt) {
        active = offers[queue_head];
        queue_head = (queue_head + 1) % COUNT(offers);
        --queue_count;
        if (!offer_alive(&active)) continue;
        MutationDecision decision = snapshot_offer(&active);
        if (decision == VANILLA_MUTATION) { original_set(active.cat, active.pair); continue; }
        if (decision != CHOOSE_MUTATION || !selector_available()) {
            log_message(OWNER, "Kept mutation: downgrade, unknown definition, or selector asset unavailable.");
            continue;
        }
        view_count = 0;
        for (unsigned i = 0; i < active.count; ++i) {
            PartSnapshot *part = &active.parts[i];
            if (part->old == (int)(active.pair >> 32)) continue;
            unsigned j = 0;
            while (j < view_count && (active.parts[view_indices[j]].part != part->part ||
                   active.parts[view_indices[j]].old != part->old)) ++j;
            if (j == view_count) view_indices[view_count++] = i;
        }
        int described = 1;
        unsigned effect_rows = 1;
        navigation_needed = view_count > 1;
        for (unsigned i = 0; i < view_count; ++i) {
            wchar_t text[4096], page[4096];
            PartSnapshot *part = &active.parts[view_indices[i]];
            for (unsigned side = 0; side < 2; ++side) {
                if (!describe_mutation(part->part, side ? (int)(active.pair >> 32) : part->old, text)) described = 0;
                if (text_page(text, 0, page) > 1) navigation_needed = 1;
                unsigned rows = 0;
                for (const wchar_t *p = page; *p; ++p) if (*p == L'\n') ++rows;
                if (rows > effect_rows) effect_rows = rows;
            }
        }
        if (!described) { log_message(OWNER, "Kept mutation: full comparison unavailable."); continue; }
        GameString name = small_string("GoodGenesMutationChoice");
        byte *scene = FN(byte *(*)(void *, GameString *), 0x9d4950)(active.manager, &name);
        if (!scene || scene[0x4b0]) continue;
        void *entity = FN(void *(*)(void *), 0x96b3e0)(scene);
        wchar_t cat_name[100], title[180];
        if (!copy_wide((byte *)active.cat + 0x18, cat_name, COUNT(cat_name))) wcscpy(cat_name, L"Cat");
        double width = 0;
        for (unsigned i = 0; cat_name[i]; ++i) {
            width += glyph_width(cat_name[i]) * 21 / 20;
            if (width > 360 && i + 4 < COUNT(cat_name)) { wcscpy(cat_name + i, L"..."); break; }
        }
        swprintf(title, COUNT(title), L"%ls - Mutation", cat_name);
        WideString prompt = wide_string(title);
        GameFunction yes, no;
        init_choice(&yes, 1);
        init_choice(&no, 0);
        creating_selector = 1;
        memset(part_art, 0, sizeof(part_art));
        memset(stat_art, 0, sizeof(stat_art));
        active_prompt = FN(byte *(*)(void *, void *, void **, WideString *, GameFunction *, GameFunction *), 0x77f140)
            (scene, entity, (void **)&active.origin, &prompt, &yes, &no);
        creating_selector = 0;
        FN(void (*)(WideString *), 0x52290)(&prompt);
        destroy_function(&yes);
        destroy_function(&no);
        if (!active_prompt) { scene[0x4db] = 1; continue; }
        byte *panel = *(byte **)(active_prompt + 0x40);
        for (int i = 0; i < 2; ++i) {
            if (!navigation_needed) {
                byte *child = panel_child(i ? "next" : "previous");
                if (child) child[8] &= (byte)~0x20;
                continue;
            }
            GameString child = small_string(i ? "next" : "previous"), label = small_string("");
            GameFunction click;
            init_choice(&click, i ? 3 : 2);
            FN(void *(*)(void *, GameString *, GameString *, GameFunction *), 0x97c070)(panel, &child, &label, &click);
        }
        view_part = view_page = 0;
        render_comparison();
        intro_button = NULL;
        skin_selector(effect_rows);
        byte *root = *(byte **)(*(byte **)(panel+0x38)+0x80);
        *(float *)(root+0x74) += 360-panel_center_y;
        memcpy(intro_matrix, root+0x60, sizeof(intro_matrix));
        intro_started = GetTickCount64();
        selector_intro();
        void *audio = FN(void *(*)(void *), 0x4a300)(active_prompt);
        if (audio) {
            GameString event = small_string("Paper_Appear");
            FN(void (*)(void *, GameString *, double, double, double, byte), 0x95b770)
                (audio, &event, 1.0, 1.0, 0.0, 0);
        }
    }
    pumping = 0;
}

static int offer_mutation(void *cat, uint64_t pair) {
    Offer offer = {.cat = cat, .pair = pair};
    MutationDecision decision = snapshot_offer(&offer);
    if (decision == VANILLA_MUTATION) return 0;
    if (decision == KEEP_MUTATION) return 1;
    DWORD thread = GetCurrentThreadId();
    if (owner_thread && owner_thread != thread) return 1;
    owner_thread = thread;
    if (queue_count == COUNT(offers)) { log_message(OWNER, "Selector queue full; kept existing mutation."); return 1; }
    if (!read_bytes(base + 0x13c18f0, &offer.origin, 8) || !offer.origin ||
        !read_bytes(offer.origin, &offer.manager, 8) ||
        !read_bytes(offer.origin - 8, &offer.generation, 8) ||
        !read_bytes(cat, offer.identity, sizeof(offer.identity))) return 1;
    offer.ticket = ++next_ticket;
    offers[(queue_head + queue_count++) % COUNT(offers)] = offer;
    show_next_offer();
    return 1;
}

static void selector_panel(void *panel, GameString *symbol) {
    if (creating_selector) {
        const char *name = "GoodGenesMutationSelector";
        FN(void (*)(GameString *, const char *, size_t), 0x520d0)(symbol, name, strlen(name));
    }
    original_panel(panel, symbol);
}

static void selector_close(void *prompt, GameFunction *choice) {
    if (prompt != active_prompt) { original_close(prompt, choice); return; }
    if (closing) { destroy_function(choice); return; }
    closing = 1;
    intro_button = NULL;
    FN(void (*)(void *, GameFunction *), 0x48340)((byte *)prompt + 0xd8, choice);
    FN(void (*)(void *), 0x97be70)(*(void **)((byte *)prompt + 0x40));
    /* The custom static panel has no AS timeline. Reuse native completion to
       restore paused scenes, navigation/cursor state, and destroy the overlay. */
    void *capture[2] = {NULL, prompt};
    FN(void (*)(void *), 0x77f9c0)(capture);
    destroy_function(choice);
    closing = 0;
}

static void *selector_destroy(void *prompt, unsigned flags) {
    if (prompt == active_prompt) {
        /* External scene teardown cancels all outstanding writes. */
        active_prompt = NULL;
        intro_button = NULL;
        memset(&active, 0, sizeof(active));
        queue_count = 0;
    }
    return original_destroy(prompt, flags);
}

static void selector_cat_destroy(void *cat) {
    if (active.cat == cat) active.cat = NULL;
    for (unsigned i = 0; i < queue_count; ++i) {
        Offer *offer = &offers[(queue_head+i) % COUNT(offers)];
        if (offer->cat == cat) offer->cat = NULL;
    }
    original_cat_destroy(cat);
}
