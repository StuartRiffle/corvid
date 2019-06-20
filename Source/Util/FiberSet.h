#pragma once

#include "boost/fiber/all.hpp"

#define YIELD_FIBER() boost::this_fiber::yield()

class FiberSet
{
    typedef boost::fibers::fiber Fiber;

    list< Fiber > _Fibers;

public:

    template< typename T >
    void Spawn( T& func )
    {
        _Fibers.emplace_back( [=]() { func(); } );
    }

    int GetCount()
    {
        return (int) _Fibers.size();
    }

    void Update()
    {
        // -----------------------------------------------------------------------------------
        YIELD_FIBER();
        // -----------------------------------------------------------------------------------

        JoinCompletedFibers();
    }

private:

    void JoinCompletedFibers()
    {
        auto joinable = std::remove_if( _Fibers.begin(), _Fibers.end(),
            []( Fiber& f ) -> bool { return f.joinable()? (f.join(), true) : false; } );

        _Fibers.erase( joinable, _Fibers.end() );
    }
};

