/* Deferred policy checks. Not executed for the baseline build. */
#include <assert.h>
#include "policy.h"

int main(void) {
    assert(inherited_parent(1, 0) == 0);
    assert(inherited_parent(0, 1) == 1);
    assert(inherited_parent(1, 1) == -1);
    assert(inherited_parent(0, 0) == -1);
    assert(inherited_parent(-1, 1) == -1);
    assert(inherited_parent(1, -1) == -1);
    /* STR, DEX, CON, INT, SPD, CHA, LCK. */
    MutationQuality plain = {STAT_MUTATION, {1,0,0,0,0,0,0}};
    MutationQuality tradeoff = {STAT_MUTATION, {2,0,0,-1,0,0,0}};
    MutationQuality stronger = {STAT_MUTATION, {2,0,0,0,0,0,0}};
    MutationQuality different = {STAT_MUTATION, {0,0,0,1,0,0,0}};
    MutationQuality bad = {STAT_MUTATION, {-2,0,0,1,0,0,0}};
    MutationQuality less_bad = {STAT_MUTATION, {-1,0,0,1,0,0,0}};
    MutationQuality empty = {UNMUTATED, {0}};
    MutationQuality defect = {BIRTH_DEFECT, {-2,0,0,0,0,0,0}};
    MutationQuality special = {SPECIAL_MUTATION, {0}};
    MutationQuality unknown = {UNKNOWN, {0}};
    assert(improves(plain, stronger));
    assert(!improves(plain, tradeoff));
    assert(!improves(tradeoff, plain));
    assert(!improves(tradeoff, tradeoff));
    assert(improves(tradeoff, stronger));
    assert(!improves(stronger, tradeoff));
    assert(!improves(plain, different));
    assert(!improves(different, plain));
    assert(!improves(bad, plain)); /* Losing the INT bonus is still a downgrade. */
    assert(improves(bad, less_bad));
    assert(improves(defect, plain));
    assert(improves(empty, plain));
    assert(!improves(empty, bad));
    assert(!improves(empty, tradeoff));
    assert(!improves(defect, tradeoff));
    assert(!improves(plain, special));
    assert(!improves(special, stronger));
    assert(!improves(unknown, stronger));
    assert(!improves(stronger, unknown));
    assert(!improves(empty, special));
    assert(mutation_decision(empty, special) == VANILLA_MUTATION);
    assert(mutation_decision(empty, tradeoff) == VANILLA_MUTATION);
    assert(mutation_decision(empty, defect) == VANILLA_MUTATION);
    assert(mutation_decision(plain, tradeoff) == KEEP_MUTATION);
    assert(mutation_decision(tradeoff, plain) == KEEP_MUTATION);
    assert(mutation_decision(plain, plain) == KEEP_MUTATION);
    assert(mutation_decision(plain, stronger) == VANILLA_MUTATION);
    assert(mutation_decision(stronger, plain) == KEEP_MUTATION);
    assert(mutation_decision(plain, special) == CHOOSE_MUTATION);
    assert(mutation_decision(special, plain) == CHOOSE_MUTATION);
    assert(mutation_decision(special, special) == CHOOSE_MUTATION); /* Different IDs; identical IDs skip before policy. */
    assert(mutation_decision(defect, plain) == VANILLA_MUTATION);
    assert(mutation_decision(bad, less_bad) == VANILLA_MUTATION);
    assert(mutation_decision(plain, defect) == KEEP_MUTATION);
    assert(mutation_decision(unknown, plain) == KEEP_MUTATION);
    assert(mutation_decision(plain, unknown) == KEEP_MUTATION);
    assert(mutation_decision(special, empty) == KEEP_MUTATION);
    for (int i = 0; i < MUTATION_STATS; ++i) {
        MutationQuality positive = {STAT_MUTATION, {0}};
        positive.stats[i] = 1;
        assert(improves(empty, positive));
        assert(!improves(positive, positive));
        for (int j = 0; j < MUTATION_STATS; ++j) {
            MutationQuality candidate = positive;
            candidate.stats[j] -= 1;
            assert(!improves(positive, candidate));
        }
    }
    return 0;
}
