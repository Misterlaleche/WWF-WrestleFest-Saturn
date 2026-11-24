/* Direct inflate_table differential, independent of the decompressor. */
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zutil.h"
#include "inftrees.h"

extern int wf_c23_reference_table(codetype, unsigned short *, unsigned,
                                  code **, unsigned *, unsigned short *);

enum { GUARD = 32, SHIFT = 7, MAX_CODES = 288, TABLE_ENTRIES = ENOUGH };
typedef struct {
    unsigned char before[GUARD];
    code entry[TABLE_ENTRIES+SHIFT];
    unsigned char after[GUARD];
} table_buffer_t;
typedef struct {
    unsigned char before[GUARD];
    unsigned short value[MAX_CODES+SHIFT];
    unsigned char after[GUARD];
} short_buffer_t;
_Static_assert(sizeof(code) == 4 && _Alignof(code) >= 4, "aligned code contract");

static uint64_t cases, type_cases[3], results[3][3], roots[16];
static uint64_t valid_capacity_returns, capacity_after_writes;
static unsigned max_entries, seen_length[3][16], seen_count[3][MAX_CODES+1];
static unsigned current_type, current_codes, current_root;
static const char *current_kind;

static void failure(const char *what) {
    fprintf(stderr, "MISMATCH %s case=%" PRIu64 " type=%u codes=%u root=%u kind=%s\n",
            what, cases, current_type, current_codes, current_root, current_kind);
    exit(90);
}

static void require(int yes, const char *what) { if (!yes) failure(what); }

static int uniform(const void *memory, size_t bytes, unsigned char value) {
    const unsigned char *p = memory;
    for (size_t i = 0; i < bytes; ++i) if (p[i] != value) return 0;
    return 1;
}

/* Independent Kraft classification. A valid tree may still exceed the
 * implementation's table capacity for a requested root. */
static int invalid_tree(codetype type, const unsigned short *lens, unsigned codes) {
    unsigned counts[16] = {0}, maximum = 0;
    for (unsigned i = 0; i < codes; ++i) {
        ++counts[lens[i]];
        if (lens[i] > maximum) maximum = lens[i];
    }
    if (!maximum) return 0;
    int available = 1;
    for (unsigned bits = 1; bits <= 15; ++bits) {
        available = available*2-(int)counts[bits];
        if (available < 0) return 1;
    }
    return available > 0 && (type == CODES || maximum != 1);
}

static size_t table_offset(code *result, code *start) {
    uintptr_t first = (uintptr_t)start, last = first+TABLE_ENTRIES*sizeof(code);
    uintptr_t address = (uintptr_t)result;
    require(address >= first && address <= last && (address-first)%sizeof(code) == 0,
            "table pointer range/alignment");
    return (address-first)/sizeof(code);
}

static void one(codetype type, const unsigned short *input, unsigned count,
                unsigned root, const char *kind) {
    table_buffer_t reference_table, candidate_table;
    short_buffer_t reference_work, candidate_work, reference_lens, candidate_lens;
    current_type = (unsigned)type; current_codes = count; current_root = root;
    current_kind = kind; ++cases; ++type_cases[type]; ++roots[root];
    unsigned offset = (unsigned)cases & SHIFT;
    unsigned char tp = (unsigned char)(0xa5u ^ cases), wp = (unsigned char)(0x59u ^ cases);
    unsigned char lp = (unsigned char)(0x96u ^ cases);
    require(count <= (type == CODES ? 19u : type == LENS ? 288u : 32u), "code count contract");
    seen_count[type][count] = 1;
    for (unsigned i = 0; i < count; ++i) {
        require(input[i] <= (type == CODES ? 7u : 15u), "length contract");
        seen_length[type][input[i]] = 1;
    }
    memset(&reference_table, tp, sizeof reference_table);
    candidate_table = reference_table;
    memset(&reference_work, wp, sizeof reference_work);
    candidate_work = reference_work;
    memset(&reference_lens, lp, sizeof reference_lens);
    memcpy(reference_lens.value+offset, input, count*sizeof *input);
    candidate_lens = reference_lens;
    code *ref = reference_table.entry+offset, *got = candidate_table.entry+offset;
    unsigned ref_bits = root, got_bits = root;
    int expected = wf_c23_reference_table(type, reference_lens.value+offset, count,
                                          &ref, &ref_bits, reference_work.value+offset);
    int actual = inflate_table(type, candidate_lens.value+offset, count,
                               &got, &got_bits, candidate_work.value+offset);
    require(actual == expected, "return value");
    require(got_bits == ref_bits, "root bits");
    size_t ref_used = table_offset(ref, reference_table.entry+offset);
    size_t got_used = table_offset(got, candidate_table.entry+offset);
    require(got_used == ref_used, "table pointer advancement");
    require(!memcmp(&reference_table, &candidate_table, sizeof reference_table), "table bytes");
    require(!memcmp(&reference_work, &candidate_work, sizeof reference_work), "work bytes");
    require(!memcmp(&reference_lens, &candidate_lens, sizeof reference_lens), "lens bytes");
    require(!memcmp(candidate_lens.value+offset, input, count*sizeof *input), "lens modified");
    require(actual >= -1 && actual <= 1, "return domain");
    ++results[type][actual+1];
    if (got_used > max_entries) max_entries = (unsigned)got_used;

    /* Check both implementations' external guards independently, plus the
     * unused prefixes and tails inside the allocated objects. Comparing the
     * two objects alone could hide the same accidental overrun on both sides. */
    table_buffer_t *tables[2] = {&reference_table, &candidate_table};
    short_buffer_t *works[2] = {&reference_work, &candidate_work};
    short_buffer_t *lenses[2] = {&reference_lens, &candidate_lens};
    for (unsigned i = 0; i < 2; ++i) {
        require(uniform(tables[i]->before, GUARD, tp) && uniform(tables[i]->after, GUARD, tp), "table canaries");
        require(uniform(tables[i]->entry, offset*sizeof(code), tp), "table prefix canary");
        require(uniform(tables[i]->entry+offset+TABLE_ENTRIES,
                        (SHIFT-offset)*sizeof(code), tp), "table suffix canary");
        require(uniform(works[i]->before, GUARD, wp) && uniform(works[i]->after, GUARD, wp), "work canaries");
        require(uniform(works[i]->value, offset*sizeof(unsigned short), wp), "work prefix canary");
        require(uniform(works[i]->value+offset+count,
                        (MAX_CODES+SHIFT-offset-count)*sizeof(unsigned short), wp), "work tail canary");
        require(uniform(lenses[i]->before, GUARD, lp) && uniform(lenses[i]->after, GUARD, lp), "lens canaries");
        require(uniform(lenses[i]->value, offset*sizeof(unsigned short), lp), "lens prefix canary");
        require(uniform(lenses[i]->value+offset+count,
                        (MAX_CODES+SHIFT-offset-count)*sizeof(unsigned short), lp), "lens tail canary");
    }
    int invalid = invalid_tree(type, input, count);
    require(invalid ? actual == -1 : actual >= 0, "independent Kraft classification");
    if (actual == -1) {
        require(got_used == 0 && got_bits == root, "invalid scalar preservation");
        require(uniform(&candidate_table, sizeof candidate_table, tp), "invalid table preservation");
        require(uniform(&candidate_work, sizeof candidate_work, wp), "invalid work preservation");
    }
    if (actual == 1) {
        ++valid_capacity_returns;
        capacity_after_writes += !uniform(&candidate_table, sizeof candidate_table, tp);
        require(got_used == 0 && got_bits == root, "capacity scalar preservation");
    }
    if (actual == 0) {
        require(got_bits >= 1 && got_bits <= 15, "successful root range");
        require(uniform(candidate_table.entry+offset+got_used,
                        (TABLE_ENTRIES-got_used)*sizeof(code), tp), "successful unused table tail");
    }
}

static void all_roots(codetype type, const unsigned short *lens, unsigned count, const char *kind) {
    for (unsigned root = 0; root <= 15; ++root) one(type, lens, count, root, kind);
}

static uint32_t next_random(uint32_t *state) {
    *state ^= *state << 13; *state ^= *state >> 17; *state ^= *state << 5;
    return *state;
}

/* Splitting a leaf replaces weight 2^-d with two weights 2^-(d+1).
 * This constructs complete trees without using inflate_table's algorithm. */
static void split_tree(unsigned short *leaves, unsigned target, unsigned max_length,
                       unsigned strategy, uint32_t seed) {
    leaves[0] = leaves[1] = 1;
    for (unsigned count = 2; count < target; ++count) {
        unsigned chosen = count;
        if (strategy == 2) {
            do { chosen = next_random(&seed)%count; } while (leaves[chosen] == max_length);
        } else {
            for (unsigned i = 0; i < count; ++i) {
                if (leaves[i] == max_length) continue;
                if (chosen == count || (strategy == 0 ? leaves[i] < leaves[chosen] : leaves[i] > leaves[chosen]))
                    chosen = i;
            }
        }
        require(chosen < count, "split generator capacity");
        leaves[count] = ++leaves[chosen];
    }
}

static int boundary(unsigned leaves, unsigned maximum) {
    return leaves <= 8 || leaves+1 >= maximum || !(leaves & (leaves-1)) || !((leaves+1) & leaves);
}

static void test_type(codetype type) {
    unsigned maximum = type == CODES ? 19 : type == LENS ? 288 : 32;
    unsigned width = type == CODES ? 7 : 15;
    unsigned short lens[MAX_CODES] = {0}, leaves[MAX_CODES], mutated[MAX_CODES];
    /* Every supported count, including zero. */
    for (unsigned count = 0; count <= maximum; ++count) all_roots(type, lens, count, "empty");

    /* Exhaustive small alphabets cover zero, singleton, incomplete and
     * oversubscribed trees independently of the split generator. */
    unsigned vectors = 1;
    for (unsigned count = 0; count <= 4; ++count) {
        for (unsigned encoded = 0; encoded < vectors; ++encoded) {
            unsigned value = encoded;
            for (unsigned i = 0; i < count; ++i) { lens[i] = (unsigned short)(value & 3); value >>= 2; }
            all_roots(type, lens, count, "exhaustive_small");
        }
        vectors *= 4;
    }

    unsigned single_counts[] = {1, 2, maximum-1, maximum};
    for (unsigned k = 0; k < sizeof single_counts/sizeof single_counts[0]; ++k) {
        unsigned count = single_counts[k], positions[] = {0, count/2, count-1};
        for (unsigned pos = 0; pos < 3; ++pos) {
            memset(lens, 0, sizeof lens);
            for (unsigned bits = 1; bits <= width; ++bits) {
                lens[positions[pos]] = (unsigned short)bits;
                all_roots(type, lens, count, "singleton_all_widths");
            }
        }
    }

    for (unsigned count = 2; count <= maximum; ++count) {
        for (unsigned strategy = 0; strategy < 3; ++strategy) {
            uint32_t seed = 0x692b34adu ^ (type+1)*0x10001u ^ count*97u ^ strategy;
            split_tree(leaves, count, width, strategy, seed);
            for (unsigned layout = 0; layout < 2; ++layout) {
                unsigned codes = layout ? maximum : count;
                unsigned positions[MAX_CODES];
                for (unsigned i = 0; i < codes; ++i) positions[i] = i;
                if (layout) for (unsigned i = codes; i > 1; --i) {
                    unsigned j = next_random(&seed)%i, tmp = positions[i-1];
                    positions[i-1] = positions[j]; positions[j] = tmp;
                }
                memset(lens, 0, sizeof lens);
                for (unsigned i = 0; i < count; ++i) lens[positions[i]] = leaves[i];
                all_roots(type, lens, codes, layout ? "split_sparse_permuted" : "split_contiguous");
                if (!boundary(count, maximum)) continue;
                memcpy(mutated, lens, sizeof lens);
                mutated[positions[0]] = 0;
                all_roots(type, mutated, codes, "remove_leaf");
                for (unsigned i = 0; i < codes; ++i) if (lens[i] > 1) {
                    memcpy(mutated, lens, sizeof lens); --mutated[i];
                    all_roots(type, mutated, codes, "shorten_leaf"); break;
                }
                for (unsigned i = 0; i < codes; ++i) if (lens[i] && lens[i] < width) {
                    memcpy(mutated, lens, sizeof lens); ++mutated[i];
                    all_roots(type, mutated, codes, "lengthen_leaf"); break;
                }
                for (unsigned i = 0; i < codes; ++i) if (!lens[i]) {
                    memcpy(mutated, lens, sizeof lens); mutated[i] = 1;
                    all_roots(type, mutated, codes, "add_leaf"); break;
                }
            }
        }
    }
    uint32_t seed = 0x9857bca1u ^ type;
    for (unsigned sample = 0; sample < 128; ++sample) {
        unsigned count = next_random(&seed)%(maximum+1);
        for (unsigned i = 0; i < count; ++i) lens[i] = (unsigned short)(next_random(&seed)%(width+1));
        all_roots(type, lens, count, "bounded_random");
    }
    for (unsigned count = 0; count <= maximum; ++count) require(seen_count[type][count], "count coverage");
    for (unsigned bits = 0; bits <= width; ++bits) require(seen_length[type][bits], "length coverage");
    require(results[type][0] && results[type][1], "valid/invalid result coverage");
    if (type != CODES) require(results[type][2], "capacity result coverage");
}

int main(void) {
    test_type(CODES); test_type(LENS); test_type(DISTS);
    require(capacity_after_writes != 0, "partial capacity failure coverage");
    printf("{\"status\":\"PASS\",\"cases\":%" PRIu64 ",\"table_entries_per_comparison\":%u,\"max_success_entries\":%u,",
           cases, TABLE_ENTRIES+SHIFT, max_entries);
    printf("\"valid_capacity_returns\":%" PRIu64 ",\"capacity_after_writes\":%" PRIu64 ",\"types\":[",
           valid_capacity_returns, capacity_after_writes);
    for (unsigned type = 0; type < 3; ++type)
        printf("%s{\"type\":%u,\"cases\":%" PRIu64 ",\"return_minus1\":%" PRIu64 ",\"return_zero\":%" PRIu64 ",\"return_plus1\":%" PRIu64 "}",
               type ? "," : "", type, type_cases[type], results[type][0], results[type][1], results[type][2]);
    printf("],\"roots\":[");
    for (unsigned root = 0; root < 16; ++root) printf("%s%" PRIu64, root ? "," : "", roots[root]);
    puts("]}");
    return 0;
}
