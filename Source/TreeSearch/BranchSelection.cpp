// JAGLAVAK CHESS ENGINE (c) 2019 Stuart Riffle

#include "Jaglavak.h"
#include "TreeSearch.h"

double TreeSearch::CalculateUct( TreeNode* node, int childIndex )
{
    BranchInfo* nodeInfo    = node->_Info;
    BranchInfo& childInfo   = node->_Branch[childIndex];
    const ScoreCard& scores = childInfo._Scores;

    u64 nodePlays  = Max< u64 >( nodeInfo->_Scores._Plays, 1 );
    u64 childPlays = Max< u64 >( scores._Plays, 1 );
    u64 childWins  = scores._Wins[node->_Color];

    if( _DrawsWorthHalf )
    {
        u64 draws = scores._Plays - (scores._Wins[WHITE] + scores._Wins[BLACK]);
        childWins += draws / 2;
    }

    double invChildPlays = 1.0 / childPlays;
    double childWinRatio = childWins * invChildPlays;

    double uct = 
        childWinRatio + 
        sqrt( log( (double) nodePlays ) * 2 * invChildPlays ) * _ExplorationFactor +
        childInfo._VirtualLoss +
        childInfo._Prior;

    return uct;
}

int TreeSearch::GetRandomUnexploredBranch( TreeNode* node )
{
    int numBranches = (int) node->_Branch.size();
    int idx = (int) _RandomGen.GetRange( numBranches );

    for( int i = 0; i < numBranches; i++ )
    {
        if( !node->_Branch[idx]._Node )
            return idx;

        idx = (idx + 1) % numBranches;
    }

    return( -1 );
}

int TreeSearch::SelectNextBranch( TreeNode* node )
{
    int numBranches = (int) node->_Branch.size();
    assert( numBranches > 0 );

//    bool expandingAllBranches = (_Settings->Get( "Search.BranchesToExpand" ) == 0);
//    if( !expandingAllBranches )
    {
        int randomBranch = GetRandomUnexploredBranch( node );
        if( randomBranch >= 0 )
            return randomBranch;
    }

    // This node is fully expanded, so choose the move with highest UCT

    double branchUct[MAX_POSSIBLE_MOVES];
    double highestUct = -DBL_MAX;

    for( int i = 0; i < numBranches; i++ )
    {
        double uct = CalculateUct( node, i );
        branchUct[i] = uct;

        if( uct > highestUct )
            highestUct = uct;
    }

    // Early on, there could be a lot with the same UCT, so pick fairly among them

    int highestIndex[MAX_POSSIBLE_MOVES];
    int numHighest = 0;

    for( int i = 0; i < numBranches; i++ )
    {
        assert( branchUct[i] <= highestUct );
        if( branchUct[i] == highestUct )
            highestIndex[numHighest++] = i;
    }

    assert( numHighest > 0 );
    int idx = ( int) _RandomGen.GetRange( numHighest );

    return highestIndex[idx];
}


