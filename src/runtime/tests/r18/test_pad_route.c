#include <assert.h>
#include "wf_r18_pad_route.h"

static void check(unsigned first, unsigned second,
                  const unsigned expected[4][2])
{
    wf_r18_pad_location_t locations[4];
    wf_r18_pad_route(first, second, locations);
    for (unsigned i = 0; i < 4; ++i) {
        assert(locations[i].port == expected[i][0]);
        assert(locations[i].slot == expected[i][1]);
    }
}

int main(void)
{
    check(1, 1, (const unsigned[4][2]){{1,0},{2,0},{0,0},{0,0}});
    check(4, 0, (const unsigned[4][2]){{1,0},{1,1},{1,2},{1,3}});
    check(2, 2, (const unsigned[4][2]){{1,0},{2,0},{1,1},{2,1}});
    check(1, 3, (const unsigned[4][2]){{1,0},{2,0},{2,1},{2,2}});
    check(0, 4, (const unsigned[4][2]){{2,0},{2,1},{2,2},{2,3}});
    return 0;
}
