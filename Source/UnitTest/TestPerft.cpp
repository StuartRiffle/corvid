// JAGLAVAK CHESS ENGINE (c) 2019 Stuart Riffle

#include "Jaglavak.h"
#include "Util/FEN.h"
#include "Perft.h"


#include "catch.hpp"
#include "Generated/Perft.epd.h"

SCENARIO( "Embedded Perft tests" )
{
           
    GIVEN( "the embedded EPD file of Perft results" )
    {
        extern const char* Embedded_Perft;
        const char* file = Embedded_Perft;

        map< Position, map< int, u64 > > targetsByPosition;

        WHEN( "the file is split into lines" )
        {
            vector< vector< string > > epdLines = SplitLinesIntoFields( Embedded_Perft, "\r\n", ";", "#" );

            THEN( "it is not empty" )
                REQUIRE( !epdLines.empty() );

            WHEN( "those lines are split into fields" )
            {
                for( auto& fieldList : epdLines )
                {
                    for( string field : fieldList )
                        cout << "\"" << field << "\"" << endl;

                    Position pos;
                    string fen = fieldList[0];

                    THEN("the first field of each line is valid FEN")
                        REQUIRE(StringToPosition(fen.c_str(), pos) != NULL);

                    THEN("there are no repeated positions")
                        REQUIRE(targetsByPosition.find(pos) == targetsByPosition.end());

                    WHEN( "the fields are parsed into depth and target" )
                    {
                        map< int, u64 > depthToTarget;

                        THEN( "the fields contain ONLY depth and target" )
                        {
                            for( int i = 1; i < fieldList.size(); i++ )
                            {
                                string& field = fieldList[i];

                                REQUIRE( field[0] == 'D' );

                                size_t after = 0;
                                int depth = stoi( field.substr( 1 ), &after );

                                REQUIRE( depth > 0 );
                                REQUIRE( after > 0 );
                                REQUIRE( after != string::npos );
                                REQUIRE( field[after] == ' ' );
                                after++;
                                
                                u64 perftTarget = stoull( field.substr( after ) );
                                REQUIRE( perftTarget > 0 );

                                REQUIRE( depthToTarget.find( depth ) == depthToTarget.end() );
                                depthToTarget[depth] = perftTarget;
                            }
                        }

                        targetsByPosition[pos] = move( depthToTarget );
                    }

                    THEN( "there is at least one solution given for each position" )
                        REQUIRE( fieldList.size() > 1 );
                }
            }
        }

        WHEN( "the move generator calculates Perft" )
        {
            for( auto iter : targetsByPosition )
            {
                Position pos = iter.first;
                string fen = SerializePosition( pos );

                map< int, u64 >& depthToTarget = iter.second;
                for( auto& elem : depthToTarget )
                {
                    int depth = elem.first;
                    assert( depth > 0 );

                    u64 target = elem.second;
                    assert( target > 0 );

                    THEN( "Perft of (" << fen << ") at depth " << depth << " is " << target )
                    {
                        u64 calculated = CalcPerft( pos, depth );
                        REQUIRE( calculated == target );
                    }
                }
            }
        }
    }
}

