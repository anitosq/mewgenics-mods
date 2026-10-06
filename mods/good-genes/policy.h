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

typedef enum { KEEP_MUTATION, VANILLA_MUTATION, CHOOSE_MUTATION } MutationDecision;

/* Only specials need consent. Stat-only changes require a clear improvement. */
static MutationDecision mutation_decision(MutationQuality old, MutationQuality next) {
    if (old.kind == UNMUTATED) return VANILLA_MUTATION;
    if (old.kind == UNKNOWN || next.kind == UNKNOWN || next.kind == UNMUTATED) return KEEP_MUTATION;
    if (old.kind == SPECIAL_MUTATION || next.kind == SPECIAL_MUTATION) return CHOOSE_MUTATION;
    old.kind = next.kind = STAT_MUTATION;
    return improves(old, next) ? VANILLA_MUTATION : KEEP_MUTATION;
}

#endif
