#ifndef GOOD_GENES_SKILLS_H
#define GOOD_GENES_SKILLS_H
#include <string.h>

enum { INHERITED_SKILL_CAPACITY = 8, INHERITED_SKILL_NAME = 128 };
typedef struct {
    char names[INHERITED_SKILL_CAPACITY][INHERITED_SKILL_NAME];
    unsigned count;
} InheritedSkills;

/* Active names must first pass through the game's upgrade normalizer. */
static int add_inherited_skill(InheritedSkills *pool, const char *name, int passive) {
    size_t length = strlen(name);
    if (!length || !strcmp(name, "None") || (passive && !strcmp(name, "SkillShare"))) return 1;
    if (length >= INHERITED_SKILL_NAME) return 0;
    for (unsigned i = 0; i < pool->count; ++i)
        if (!strcmp(pool->names[i], name)) return 1;
    if (pool->count == INHERITED_SKILL_CAPACITY) return 0;
    memcpy(pool->names[pool->count++], name, length+1);
    return 1;
}

/* Pick uniformly from unique skills, excluding the first active on pick two. */
static int inherited_skill_index(unsigned count, int excluded, double roll) {
    if (!count || count > INHERITED_SKILL_CAPACITY || excluded < -1 ||
        (excluded >= 0 && (unsigned)excluded >= count) || !(roll >= 0 && roll < 1)) return -1;
    unsigned available = count - (excluded >= 0);
    if (!available) return -1;
    unsigned index = (unsigned)(roll * available);
    if (excluded >= 0 && index >= (unsigned)excluded) ++index;
    return (int)index;
}
#endif
