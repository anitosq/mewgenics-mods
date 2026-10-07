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

static MutationDecision stat_decision(MutationQuality old, MutationQuality next) {
    old.kind = next.kind = STAT_MUTATION;
    if (improves(old, next)) return VANILLA_MUTATION;
    int gain = 0, loss = 0;
    for (unsigned i = 0; i < MUTATION_STATS; ++i) {
        gain |= next.stats[i] > old.stats[i];
        loss |= next.stats[i] < old.stats[i];
    }
    return gain && loss ? CHOOSE_MUTATION : KEEP_MUTATION;
}

/* Specials and stat trade-offs need consent; pure improvements remain automatic. */
static MutationDecision mutation_decision(MutationQuality old, MutationQuality next) {
    if (next.kind == UNKNOWN) return INVALID_MUTATION;
    if (old.kind == UNMUTATED) return VANILLA_MUTATION;
    if (old.kind == UNKNOWN || next.kind == UNMUTATED) return INVALID_MUTATION;
    if (old.kind == SPECIAL_MUTATION || next.kind == SPECIAL_MUTATION) return CHOOSE_MUTATION;
    return stat_decision(old, next);
}

typedef struct {
    int group, id;
    MutationQuality quality;
} MutationBonus;

/* The game counts each (part group, mutation ID) once, not once per side. */
static int effective_stats(const MutationBonus *parts, unsigned count, MutationQuality *total) {
    *total = (MutationQuality){STAT_MUTATION, {0}};
    for (unsigned i = 0; i < count; ++i) {
        const MutationBonus *part = &parts[i];
        if (part->quality.kind == UNKNOWN) return 0;
        if (part->quality.kind == UNMUTATED) continue;
        unsigned j = 0;
        while (j < i && (parts[j].quality.kind == UNMUTATED ||
               parts[j].group != part->group || parts[j].id != part->id)) ++j;
        if (j < i) continue;
        for (unsigned stat = 0; stat < MUTATION_STATS; ++stat)
            total->stats[stat] += part->quality.stats[stat];
    }
    return 1;
}

static MutationDecision effective_stats_decision(const MutationBonus *before, const MutationBonus *after, unsigned count) {
    MutationQuality old, next;
    if (!effective_stats(before, count, &old) || !effective_stats(after, count, &next)) return INVALID_MUTATION;
    return stat_decision(old, next);
}

/* One native roll changes all affected parts; a choice may accept stat trade-offs. */
static MutationDecision combine_mutation_decisions(MutationDecision a, MutationDecision b) {
    if (a == INVALID_MUTATION || b == INVALID_MUTATION) return INVALID_MUTATION;
    if (a == CHOOSE_MUTATION || b == CHOOSE_MUTATION) return CHOOSE_MUTATION;
    if (a == KEEP_MUTATION || b == KEEP_MUTATION) return KEEP_MUTATION;
    return VANILLA_MUTATION;
}

#endif
