#ifndef GOOD_GENES_POLICY_H
#define GOOD_GENES_POLICY_H

typedef enum { UNKNOWN, UNMUTATED, BIRTH_DEFECT, STAT_MUTATION, SPECIAL_MUTATION } MutationKind;
enum { MUTATION_STATS = 7 };
typedef struct {
    MutationKind kind;
    double stats[MUTATION_STATS];
} MutationQuality;

/* -1 leaves selection to the native roll (or native fallback for unknowns). */
static int inherited_parent(int first, int second) {
    if (first < 0 || second < 0) return -1;
    if (first && !second) return 0;
    if (second && !first) return 1;
    return -1;
}

/* No stat may decrease; at least one must increase. Empty parts start at zero. */
static int improves(MutationQuality old, MutationQuality next) {
    if (next.kind != STAT_MUTATION) return 0;
    if (old.kind != UNMUTATED && old.kind != BIRTH_DEFECT && old.kind != STAT_MUTATION) return 0;
    int better = 0;
    for (int i = 0; i < MUTATION_STATS; ++i) {
        double previous = old.kind == UNMUTATED ? 0 : old.stats[i];
        if (!(next.stats[i] >= previous)) return 0;
        if (next.stats[i] > previous) better = 1;
    }
    return better;
}

typedef enum { KEEP_MUTATION, VANILLA_MUTATION, CHOOSE_MUTATION, INVALID_MUTATION } MutationDecision;

/* Only specials need consent. Stat-only changes require a clear improvement. */
static MutationDecision mutation_decision(MutationQuality old, MutationQuality next) {
    if (next.kind == UNKNOWN) return INVALID_MUTATION;
    if (old.kind == UNMUTATED) return VANILLA_MUTATION;
    if (old.kind == UNKNOWN || next.kind == UNMUTATED) return INVALID_MUTATION;
    if (old.kind == SPECIAL_MUTATION || next.kind == SPECIAL_MUTATION) return CHOOSE_MUTATION;
    old.kind = next.kind = STAT_MUTATION;
    return improves(old, next) ? VANILLA_MUTATION : KEEP_MUTATION;
}

typedef struct {
    int group, id;
    MutationQuality quality;
} MutationBonus;

/* The game counts each (part group, mutation ID) once, not once per side. */
static int effective_stats_improve(const MutationBonus *before, const MutationBonus *after, unsigned count) {
    MutationQuality totals[2] = {{STAT_MUTATION, {0}}, {STAT_MUTATION, {0}}};
    const MutationBonus *states[] = {before, after};
    for (unsigned side = 0; side < 2; ++side) {
        for (unsigned i = 0; i < count; ++i) {
            const MutationBonus *part = &states[side][i];
            if (part->quality.kind == UNKNOWN) return 0;
            if (part->quality.kind == UNMUTATED) continue;
            unsigned j = 0;
            while (j < i && (states[side][j].group != part->group || states[side][j].id != part->id)) ++j;
            if (j < i) continue;
            for (unsigned stat = 0; stat < MUTATION_STATS; ++stat)
                totals[side].stats[stat] += part->quality.stats[stat];
        }
    }
    return improves(totals[0], totals[1]);
}

/* One native roll changes all affected parts; a choice may accept stat trade-offs. */
static MutationDecision combine_mutation_decisions(MutationDecision a, MutationDecision b) {
    if (a == INVALID_MUTATION || b == INVALID_MUTATION) return INVALID_MUTATION;
    if (a == CHOOSE_MUTATION || b == CHOOSE_MUTATION) return CHOOSE_MUTATION;
    if (a == KEEP_MUTATION || b == KEEP_MUTATION) return KEEP_MUTATION;
    return VANILLA_MUTATION;
}

#endif
