// JAGLAVAK CHESS ENGINE (c) 2019 Stuart Riffle
#pragma once

struct CpuInfo
{
    static void Cpuid( int leaf, unsigned int* dest )
    {
    #if TOOLCHAIN_GCC
        __get_cpuid( leaf, dest + 0, dest + 1, dest + 2, dest + 3 );
    #elif TOOLCHAIN_MSVC
        __cpuid( (int*) dest, leaf );
    #endif
    }

    static bool CheckCpuFlag( int leaf, int idxWord, int idxBit )
    {
        unsigned int info[4];
        Cpuid( leaf, info );

        return( (info[idxWord] & (1 << idxBit)) != 0 );
    }

    static int DetectSimdLevel()
    {
    #if TOOLCHAIN_GCC
        bool popcnt = __builtin_cpu_supports( "popcnt" );
    #elif TOOLCHAIN_MSVC    
        bool popcnt = CheckCpuFlag( 1, 2, 23 );
    #endif

        bool avx512 = 
            CheckCpuFlag( 7, 1, 16 ) && // avx512f
            CheckCpuFlag( 7, 1, 17 ) && // avx512dq
            CheckCpuFlag( 7, 1, 30 );   // avx512bw

        if(avx512)
        {
            assert( popcnt );
            return( 8 );
        }

        bool avx2 = CheckCpuFlag( 7, 1, 5 );   

        if( avx2 )
        {
            assert( popcnt );
            return( 4 );
        }

        bool sse4 = CheckCpuFlag( 1, 2, 19 );

        if( sse4 && popcnt )
            return( 2 );

        return( 1 );
    }

    static int DetectCpuCores()
    {
    #if TOOLCHAIN_GCC
        return( sysconf( _SC_NPROCESSORS_ONLN ) );
    #elif TOOLCHAIN_MSVC
        SYSTEM_INFO si = { 0 };
        GetSystemInfo( &si );
        return( si.dwNumberOfProcessors );
    #endif
    }

    static string GetCpuName()
    {
        string result;

        union
        {
            unsigned int info[12];
            char desc[48];
        };

        Cpuid( 0x8000'0000, info );
        if( info[0] >= 0x8000'0004 )
        {
            Cpuid( 0x8000'0002, info );
            Cpuid( 0x8000'0003, info + 4 );
            Cpuid( 0x8000'0004, info + 8 );

            size_t len = 0;
            for( int i = 0; i < 47; i++ )
                if( (desc[i] != ' ') || (desc[i + 1] != ' ') )
                    desc[len++] = desc[i];

            result = string( desc, desc + len );
        }

        return result;
    }

    static INLINE u64 GetClockTick()
    { 
    #if TOOLCHAIN_GCC
        timespec ts;
        clock_gettime( CLOCK_REALTIME, &ts );
        return( (ts.tv_sec * GetClockFrequency()) + ts.tv_nsec );    
    #elif TOOLCHAIN_MSVC
        LARGE_INTEGER tick; 
        QueryPerformanceCounter( &tick ); 
        return( tick.QuadPart ); 
    #endif
    }

    static INLINE u64 GetClockFrequency()
    {
    #if TOOLCHAIN_GCC
        return( 1'000'000'000 );
    #elif TOOLCHAIN_MSVC
        static LARGE_INTEGER freq = { 0 };
        if( !freq.QuadPart )
            QueryPerformanceFrequency( &freq );
        return( freq.QuadPart );
    #endif
    }
};
