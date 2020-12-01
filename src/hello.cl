typedef float4 my4_t;

#define MAD_4(x, y)     x = mad(y, x, y);   y = mad(x, y, x);   x = mad(y, x, y);   y = mad(x, y, x);
#define MAD_16(x, y)    MAD_4(x, y);        MAD_4(x, y);        MAD_4(x, y);        MAD_4(x, y);
#define MAD_64(x, y)    MAD_16(x, y);       MAD_16(x, y);       MAD_16(x, y);       MAD_16(x, y);


__kernel void hello(
    __global const unsigned int *a,
    __global const unsigned int *b,
    __global unsigned int *c
)
{
#if 0
    int gid = get_global_id(1) * 1024 + get_global_id(0);
    //int gid = mad24(get_global_id(1), (size_t)1024, get_global_id(0));
#else
    int gid = get_global_id(0);
#endif

#if 0
    uint temp_a = a[gid];
    uint temp_b = b[gid];

    uint temp_c;
    temp_c  = ((temp_a & 0x000000FF) + (temp_b & 0x000000FF)) & 0x000000FF;
    temp_c |= ((temp_a & 0x0000FF00) + (temp_b & 0x0000FF00)) & 0x0000FF00;
    temp_c |= ((temp_a & 0x00FF0000) + (temp_b & 0x00FF0000)) & 0x00FF0000;
    temp_c |= ((temp_a & 0xFF000000) + (temp_b & 0xFF000000)) & 0xFF000000;

    c[gid] = temp_c;
#else
    my4_t temp_a = {get_local_id(0), get_local_id(0)+1, get_local_id(0)+2, get_local_id(0)+3};
    my4_t temp_0 = {get_global_id(0), get_global_id(0)+1, get_global_id(0)+2, get_global_id(0)+3};

    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);
    MAD_64(temp_a, temp_0);

    c[gid] = temp_0.s0 + temp_0.s1 + temp_0.s2 + temp_0.s3 + temp_a.s0 + temp_a.s1 + temp_a.s2 + temp_a.s3;
#endif
}
