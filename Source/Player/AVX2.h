// JAGLAVAK CHESS ENGINE (c) 2019 Stuart Riffle
#pragma once

struct simd4_avx2
{
    __m256i vec;

    INLINE simd4_avx2() {}
    INLINE simd4_avx2( u64 s )                                      : vec( _mm256_set1_epi64x( s ) ) {}
    INLINE simd4_avx2( const __m256i& v )                           : vec( v ) {}
    INLINE simd4_avx2( const simd4_avx2& v )                        : vec( v.vec ) {}

    INLINE               operator   __m256i()                 const { return( vec ); }
    INLINE simd4_avx2    operator~  ()                        const { return( _mm256_xor_si256(  vec, _mm256_set1_epi8(  ~0 ) ) ); }
    INLINE simd4_avx2    operator-  ( u64 s )                 const { return( _mm256_sub_epi64(  vec, _mm256_set1_epi64x( s ) ) ); }
    INLINE simd4_avx2    operator+  ( u64 s )                 const { return( _mm256_add_epi64(  vec, _mm256_set1_epi64x( s ) ) ); }
    INLINE simd4_avx2    operator&  ( u64 s )                 const { return( _mm256_and_si256(  vec, _mm256_set1_epi64x( s ) ) ); }
    INLINE simd4_avx2    operator|  ( u64 s )                 const { return( _mm256_or_si256(   vec, _mm256_set1_epi64x( s ) ) ); }
    INLINE simd4_avx2    operator^  ( u64 s )                 const { return( _mm256_xor_si256(  vec, _mm256_set1_epi64x( s ) ) ); }
    INLINE simd4_avx2    operator<< ( int c )                 const { return( _mm256_slli_epi64( vec, c ) ); }
    INLINE simd4_avx2    operator>> ( int c )                 const { return( _mm256_srli_epi64( vec, c ) ); }
    INLINE simd4_avx2    operator<< ( const simd4_avx2& v )   const { return( _mm256_sllv_epi64( vec, v.vec ) ); }
    INLINE simd4_avx2    operator>> ( const simd4_avx2& v )   const { return( _mm256_srlv_epi64( vec, v.vec ) ); }
    INLINE simd4_avx2    operator-  ( const simd4_avx2& v )   const { return( _mm256_sub_epi64(  vec, v.vec ) ); }
    INLINE simd4_avx2    operator+  ( const simd4_avx2& v )   const { return( _mm256_add_epi64(  vec, v.vec ) ); }
    INLINE simd4_avx2    operator&  ( const simd4_avx2& v )   const { return( _mm256_and_si256(  vec, v.vec ) ); }
    INLINE simd4_avx2    operator|  ( const simd4_avx2& v )   const { return( _mm256_or_si256(   vec, v.vec ) ); }
    INLINE simd4_avx2    operator^  ( const simd4_avx2& v )   const { return( _mm256_xor_si256(  vec, v.vec ) ); }
    INLINE simd4_avx2&   operator+= ( const simd4_avx2& v )         { return( vec = _mm256_add_epi64( vec, v.vec ), *this ); }
    INLINE simd4_avx2&   operator&= ( const simd4_avx2& v )         { return( vec = _mm256_and_si256( vec, v.vec ), *this ); }
    INLINE simd4_avx2&   operator|= ( const simd4_avx2& v )         { return( vec = _mm256_or_si256(  vec, v.vec ), *this ); }
    INLINE simd4_avx2&   operator^= ( const simd4_avx2& v )         { return( vec = _mm256_xor_si256( vec, v.vec ), *this ); }
}; 

template<>
struct SimdWidth< simd4_avx2 >
{
    enum { LANES = 4 };
};

INLINE __m256i _mm256_popcnt_epi64_avx2( const __m256i& v )
{
    __m256i mask  = _mm256_set1_epi8( 0x0F );
    __m256i shuf  = _mm256_set1_epi64x( 0x4332322132212110ULL );
    __m256i lo4   = _mm256_shuffle_epi8( shuf, _mm256_and_si256( mask, v ) );
    __m256i hi4   = _mm256_shuffle_epi8( shuf, _mm256_and_si256( mask, _mm256_srli_epi16( v, 4 ) ) );
    __m256i pop8  = _mm256_add_epi8( lo4, hi4 );
    __m256i pop64 = _mm256_sad_epu8( pop8, _mm256_setzero_si256() );

    return( pop64 );
}

INLINE __m256i _mm256_bswap_epi64_avx2( const __m256i& v )
{
    static const __m256i perm = _mm256_setr_epi8( 
         7,  6,  5,  4,  3,  2,  1,  0, 
        15, 14, 13, 12, 11, 10,  9,  8,
        23, 22, 21, 20, 19, 18, 17, 16,
        31, 30, 29, 28, 27, 26, 25, 24 );

    return( _mm256_shuffle_epi8( v, perm ) );
}

INLINE __m256i _mm256_select( const __m256i& a, const __m256i& b, const __m256i& mask )
{          
    return _mm256_blendv_epi8( a, b, mask ); // mask? b : a
}

template<>
INLINE simd4_avx2 MaskAllClear< simd4_avx2 >() 
{ 
    return( _mm256_setzero_si256() );
} 

template<>
INLINE simd4_avx2 MaskAllSet< simd4_avx2 >() 
{ 
    return( _mm256_set1_epi8( ~0 ) );
} 

template<> 
INLINE simd4_avx2 ByteSwap< simd4_avx2 >( simd4_avx2 val ) 
{
    return( _mm256_bswap_epi64_avx2( val.vec ) );
}

template<>
INLINE simd4_avx2 MaskOut< simd4_avx2 >( simd4_avx2 val, simd4_avx2 bitsToClear ) 
{ 
    return( _mm256_andnot_si256( bitsToClear.vec, val.vec ) );
}

template<>
INLINE simd4_avx2 CmpEqual< simd4_avx2 >( simd4_avx2 a, simd4_avx2 b ) 
{
    return( _mm256_cmpeq_epi64( a.vec, b.vec ) );
}

template<>
INLINE simd4_avx2 SelectIfZero< simd4_avx2 >( simd4_avx2 val, simd4_avx2 a ) 
{ 
    return( _mm256_and_si256( a.vec, _mm256_cmpeq_epi64( val.vec, _mm256_setzero_si256() ) ) );
}

template<>
INLINE simd4_avx2 SelectIfZero< simd4_avx2 >( simd4_avx2 val, simd4_avx2 a, simd4_avx2 b ) 
{
    return( _mm256_select( b.vec, a.vec, _mm256_cmpeq_epi64( val.vec, _mm256_setzero_si256() ) ) );
}

template<>
INLINE simd4_avx2 SelectIfNotZero< simd4_avx2 >( simd4_avx2 val, simd4_avx2 a ) 
{ 
    return( _mm256_andnot_si256( _mm256_cmpeq_epi64( val.vec, _mm256_setzero_si256() ), a.vec ) );
}

template<> 
INLINE simd4_avx2 SelectIfNotZero< simd4_avx2 >( simd4_avx2 val, simd4_avx2 a, simd4_avx2 b ) 
{
    return( _mm256_select( a.vec, b.vec, _mm256_cmpeq_epi64( val.vec, _mm256_setzero_si256() ) ) );
}

template<> 
INLINE simd4_avx2 SelectWithMask< simd4_avx2 >( simd4_avx2 mask, simd4_avx2 a, simd4_avx2 b ) 
{
    return( _mm256_select( b.vec, a.vec, mask.vec ) );
}

template<> 
INLINE void Transpose< simd4_avx2 >( const simd4_avx2* src, int srcStep, simd4_avx2* dest, int destStep )
{
    simd4_avx2 abcd = src[srcStep * 0];
    simd4_avx2 efgh = src[srcStep * 1];
    simd4_avx2 ijkl = src[srcStep * 2];
    simd4_avx2 nopq = src[srcStep * 3];

    simd4_avx2 aecg = _mm256_unpacklo_epi64( abcd, efgh );
    simd4_avx2 bfdh = _mm256_unpackhi_epi64( abcd, efgh );
    simd4_avx2 inkp = _mm256_unpacklo_epi64( ijkl, nopq );
    simd4_avx2 jolq = _mm256_unpackhi_epi64( ijkl, nopq );

    simd4_avx2 aein = _mm256_permute2f128_si256( aecg, inkp, 0x20 );
    simd4_avx2 bfjo = _mm256_permute2f128_si256( bfdh, jolq, 0x20 );
    simd4_avx2 cgkp = _mm256_permute2f128_si256( aecg, inkp, 0x31 );
    simd4_avx2 dhlq = _mm256_permute2f128_si256( bfdh, jolq, 0x31 ); 

    dest[destStep * 0] = aein;
    dest[destStep * 1] = bfjo;
    dest[destStep * 2] = cgkp;
    dest[destStep * 3] = dhlq;
}

