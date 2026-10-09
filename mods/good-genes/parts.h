#ifndef GOOD_GENES_PARTS_H
#define GOOD_GENES_PARTS_H

typedef struct { int id; unsigned char enabled; } MutationPart;
static const struct { int part; unsigned offset; } mutation_slots[] = {
    {0,0x90}, {1,0xe4}, {2,0x138}, {11,0x18c}, {12,0x1e0},
    {13,0x234}, {14,0x288}, {15,0x2dc}, {16,0x330}, {17,0x384},
    {18,0x3d8}, {19,0x42c}, {20,0x480}, {9,0x4d4}, {10,0x78}
};

static int comparison_group(int part) {
    if (part >= 11 && part <= 14) return 3 + (part-11)/2;
    if (part >= 15 && part <= 20) return 6 + (part-15)/2;
    return part;
}

static int selects_part(int scope, int part) {
    return scope == part || (scope == 5 && part >= 11 && part <= 14) ||
        (scope >= 3 && scope <= 8 && scope == comparison_group(part));
}

/* caa70: an absent facial pair contributes -2 once; an absent single side is 0. */
static int effective_part_id(const MutationPart parts[15], unsigned index) {
    if (parts[index].enabled) return parts[index].id;
    if (index >= 7 && index <= 12 && !parts[7+((index-7)^1)].enabled) return -2;
    return 0;
}

#endif
