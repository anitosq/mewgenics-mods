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
    assert(mutation_decision(empty, unknown) == INVALID_MUTATION);
    assert(mutation_decision(special, defect) == CHOOSE_MUTATION);
    assert(mutation_decision(plain, tradeoff) == CHOOSE_MUTATION);
    assert(mutation_decision(tradeoff, plain) == CHOOSE_MUTATION);
    assert(mutation_decision(plain, different) == CHOOSE_MUTATION);
    assert(mutation_decision(plain, plain) == KEEP_MUTATION);
    assert(mutation_decision(plain, stronger) == VANILLA_MUTATION);
    assert(mutation_decision(stronger, plain) == KEEP_MUTATION);
    assert(mutation_decision(plain, special) == CHOOSE_MUTATION);
    assert(mutation_decision(special, plain) == CHOOSE_MUTATION);
    assert(mutation_decision(special, special) == CHOOSE_MUTATION); /* Different IDs; identical IDs skip before policy. */
    assert(mutation_decision(defect, plain) == VANILLA_MUTATION);
    assert(mutation_decision(bad, less_bad) == VANILLA_MUTATION);
    assert(mutation_decision(plain, defect) == KEEP_MUTATION);
    assert(mutation_decision(unknown, plain) == INVALID_MUTATION);
    assert(mutation_decision(plain, unknown) == INVALID_MUTATION);
    assert(mutation_decision(special, empty) == INVALID_MUTATION);
    /* A paired roll must offer consent in either part order, but never for unknowns. */
    MutationDecision choose = mutation_decision(special, plain);
    MutationDecision reject = mutation_decision(stronger, plain);
    MutationDecision invalid = mutation_decision(unknown, plain);
    assert(combine_mutation_decisions(choose, reject) == CHOOSE_MUTATION);
    assert(combine_mutation_decisions(reject, choose) == CHOOSE_MUTATION);
    assert(combine_mutation_decisions(choose, invalid) == INVALID_MUTATION);
    assert(combine_mutation_decisions(invalid, choose) == INVALID_MUTATION);
    assert(combine_mutation_decisions(combine_mutation_decisions(choose, invalid), reject) == INVALID_MUTATION);
    assert(combine_mutation_decisions(VANILLA_MUTATION, reject) == KEEP_MUTATION);
    assert(combine_mutation_decisions(reject, VANILLA_MUTATION) == KEEP_MUTATION);
    assert(combine_mutation_decisions(VANILLA_MUTATION, VANILLA_MUTATION) == VANILLA_MUTATION);
    /* eyes.750 (+1 INT) + eyes.303 (+1 INT/+1 CHA) must not collapse for free. */
    MutationQuality int_eye = {STAT_MUTATION, {0,0,0,1,0,0,0}};
    MutationQuality int_cha_eye = {STAT_MUTATION, {0,0,0,1,0,1,0}};
    MutationQuality two_int_cha_eye = {STAT_MUTATION, {0,0,0,2,0,1,0}};
    MutationBonus before[] = {{6,750,int_eye}, {6,303,int_cha_eye}};
    MutationBonus after[] = {{6,303,int_cha_eye}, {6,303,int_cha_eye}};
    assert(mutation_decision(int_eye, int_cha_eye) == VANILLA_MUTATION);
    assert(effective_stats_decision(before, after, 2) == KEEP_MUTATION);
    MutationBonus reverse[] = {before[1], before[0]};
    assert(effective_stats_decision(reverse, after, 2) == KEEP_MUTATION);
    after[0] = after[1] = (MutationBonus){6,999,two_int_cha_eye};
    assert(effective_stats_decision(before, after, 2) == KEEP_MUTATION); /* Equal effective totals. */
    after[0].quality.stats[5] = after[1].quality.stats[5] = 2;
    assert(effective_stats_decision(before, after, 2) == VANILLA_MUTATION); /* Genuine combined improvement. */
    before[0] = before[1] = (MutationBonus){6,750,int_eye};
    after[0] = (MutationBonus){6,303,int_cha_eye};
    after[1] = before[1];
    assert(effective_stats_decision(before, after, 2) == VANILLA_MUTATION); /* Split a matching pair. */
    after[1] = after[0];
    assert(effective_stats_decision(before, after, 2) == VANILLA_MUTATION); /* Matching pair counts once. */
    before[1].quality = unknown;
    assert(effective_stats_decision(before, after, 2) == INVALID_MUTATION); /* Unknown counterpart is protected. */
    before[0] = (MutationBonus){3,300,plain};
    before[1] = (MutationBonus){4,300,plain};
    after[0] = before[0];
    after[1] = (MutationBonus){4,301,stronger};
    assert(effective_stats_decision(before, after, 2) == VANILLA_MUTATION); /* Arms and legs count separately. */
    before[0] = (MutationBonus){8,-2,defect};
    before[1] = before[0];
    after[0] = after[1] = (MutationBonus){8,300,plain};
    assert(effective_stats_decision(before, after, 2) == VANILLA_MUTATION); /* Defined negative IDs participate. */
    /* An inactive side must not suppress its active counterpart's bonus. */
    before[0] = (MutationBonus){6,750,empty};
    before[1] = (MutationBonus){6,750,int_eye};
    MutationQuality total;
    assert(effective_stats(before, 2, &total) && total.stats[3] == 1);
    /* Restore one of three CHA penalties at the cost of a STR bonus. */
    MutationBonus stacked[3] = {
        {0,400,{STAT_MUTATION,{2,0,0,0,0,-1,0}}},
        {1,401,{STAT_MUTATION,{0,2,0,0,0,-1,0}}},
        {2,402,{STAT_MUTATION,{0,0,2,0,0,-1,0}}}
    };
    MutationBonus restored[3] = {stacked[0], stacked[1], stacked[2]};
    restored[0] = (MutationBonus){0,300,plain};
    assert(effective_stats(stacked, 3, &total) && 7+total.stats[5] == 4);
    assert(effective_stats(restored, 3, &total) && 7+total.stats[5] == 5);
    assert(effective_stats_decision(stacked, restored, 3) == CHOOSE_MUTATION);
    /* Local trade-offs do not prompt when the combined result only loses stats. */
    before[0] = (MutationBonus){6,300,plain};
    before[1] = (MutationBonus){6,301,different};
    after[0] = after[1] = before[1];
    assert(mutation_decision(plain, different) == CHOOSE_MUTATION);
    assert(effective_stats_decision(before, after, 2) == KEEP_MUTATION);
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
