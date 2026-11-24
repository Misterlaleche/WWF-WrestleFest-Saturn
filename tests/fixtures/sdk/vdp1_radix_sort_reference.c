/* Original Jo Engine radix sorter before E02 equal-key shortcut.
 * Extracted verbatim from vdp1_command_pipeline.c SHA256
 * aa65c6c4e5e68ec5b1d0f1efea62e3d3f2d49d6af8955f91669183950fe2a01a.
 * Used as a frozen differential reference; histogram scratch is not an API.
 */
static unsigned short                   *jo_vdp1_radix_sort(const jo_vdp1_frame * const frame)
{
    unsigned short                      *low;
    unsigned short                      *high;
    const unsigned short                *keys;
    register unsigned int               i;
    unsigned int                        n;
    unsigned int                        sum;
    unsigned int                        tmp;
#ifdef JO_VDP1_SORT_IN_C
    unsigned short                      key;
#endif
    unsigned short                      *order;

    low = __jo_vdp1_radix_low;
    high = __jo_vdp1_radix_high;
    keys = frame->keys;
    n = frame->count;
    jo_memset(low, 0, sizeof(__jo_vdp1_radix_low));
    jo_memset(high, 0, sizeof(__jo_vdp1_radix_high));
#ifdef JO_VDP1_SORT_IN_C
    for (JO_ZERO(i); i < n; ++i)
    {
        key = keys[i];
        ++low[key & (JO_VDP1_RADIX - 1)];
        ++high[JO_DIV_BY_256(key)];
    }
#else
    if (n > 0)
        __jo_vdp1_radix_count(keys, keys + n, low, high);
#endif
    // Low byte (buckets in descending order: the farthest first)
    order = __jo_vdp1_sort_a;
    if (n > 0 && low[keys[0] & (JO_VDP1_RADIX - 1)] == n)
    {
        for (JO_ZERO(i); i < n; ++i)
            __jo_vdp1_sort_a[i] = i;
    }
    else
    {
        for (i = JO_VDP1_RADIX, JO_ZERO(sum); i-- > 0;)
        {
            tmp = low[i];
            low[i] = (unsigned short)sum;
            sum += tmp;
        }
#ifdef JO_VDP1_SORT_IN_C
        for (JO_ZERO(i); i < n; ++i)
            __jo_vdp1_sort_a[low[keys[i] & (JO_VDP1_RADIX - 1)]++] = i;
#else
        __jo_vdp1_radix_scatter_low(keys, keys + n, low, __jo_vdp1_sort_a);
#endif
    }
    // High byte
    if (n > 0 && high[JO_DIV_BY_256(keys[0])] == n)
        return (order);
    for (i = JO_VDP1_RADIX, JO_ZERO(sum); i-- > 0;)
    {
        tmp = high[i];
        high[i] = (unsigned short)sum;
        sum += tmp;
    }
#ifdef JO_VDP1_SORT_IN_C
    for (JO_ZERO(i); i < n; ++i)
        __jo_vdp1_sort_b[high[JO_DIV_BY_256(keys[__jo_vdp1_sort_a[i]])]++] = __jo_vdp1_sort_a[i];
#else
    __jo_vdp1_radix_scatter_high(keys, __jo_vdp1_sort_a, __jo_vdp1_sort_a + n, high, __jo_vdp1_sort_b);
#endif
    return (__jo_vdp1_sort_b);
}
