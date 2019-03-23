// TreeSearcher.h - CORVID CHESS ENGINE (c) 2019 Stuart Riffle

struct TreeSearcher
{
    TreeNode*               mNodePool;
    size_t                  mNodePoolEntries;
    TreeLink                mMruListHead;
    TreeNode*               mSearchRoot;
    BranchInfo              mRootInfo;
    UciSearchConfig         mUciConfig;
    GlobalOptions*          mOptions;
    std::thread*            mSearchThread;
    Semaphore               mSearchThreadActive;
    Semaphore               mSearchThreadIdle;
    volatile bool           mShuttingDown;
    volatile bool           mSearchRunning;
    RandomGen               mRandom;
    PlayoutJobQueue         mJobQueue;
    PlayoutResultQueue      mResultQueue;

    std::vector< std::shared_ptr< IAsyncWorker > > mAsyncWorkers;

    TreeSearcher( GlobalOptions* options, u64 randomSeed = 1 ) : mOptions( options )
    {
        mShuttingDown  = false;
        mSearchRunning = false;
        mRandom.SetSeed( randomSeed );
        mSearchThread  = new std::thread( [&] { this->SearchThread(); } );

        mNodePoolEntries = mOptions->mMaxMegaTreeNodes * 1024 * 1024;
        mNodePool = new TreeNode[mNodePoolEntries];

        for( int i = 0; i < mNodePoolEntries; i++ )
        {
            mNodePool[i].mPrev = &mNodePool[i - 1];
            mNodePool[i].mNext = &mNodePool[i + 1];
        }

        mNodePool[0].mPrev = (TreeNode*) &mMruListHead;
        mMruListHead.mNext = &mNodePool[0];

        mNodePool[mNodePoolEntries - 1].mNext = (TreeNode*) &mMruListHead;
        mMruListHead.mPrev = &mNodePool[mNodePoolEntries - 1];

        this->Reset();
    }

    ~TreeSearcher()
    {
        this->StopSearching();

        mShuttingDown = true;
        mSearchThreadActive.Post();

        mSearchThread->join();
        delete mSearchThread;

        delete mNodePool;
    }

    void MoveToFront( TreeNode* node )
    {
        TreeNode* oldFront = mMruListHead.mNext;

        assert( node->mNext->mPrev == node );
        assert( node->mPrev->mNext == node );
        assert( oldFront->mPrev == (TreeNode*) &mMruListHead );

        node->mNext->mPrev = node->mPrev;
        node->mPrev->mNext = node->mNext;



        node->mNext = mMruListHead.mNext;
        node->mNext->mPrev = node;

        node->mPrev = (TreeNode*) &mMruListHead;
        node->mPrev->mNext = node;

        assert( mMruListHead.mNext == node );

        //this->DebugVerifyMruList();
    }

    TreeNode* AllocNode()
    {
        TreeNode* last = mMruListHead.mPrev;
        last->Clear();

        MoveToFront( last );

        TreeNode* first = mMruListHead.mNext;
        return first;
    }

    void SetPosition( const Position& startPos, const MoveList* moveList = NULL )
    {
        // TODO: recognize position and don't terf the whole tree

        Position pos = startPos;

        if( moveList )
            for( int i = 0; i < moveList->mCount; i++ )
                pos.Step( moveList->mMove[i] );

        mSearchRoot = AllocNode();
        mSearchRoot->Init( pos );

        mSearchRoot->mInfo = &mRootInfo;
        mRootInfo.mNode = mSearchRoot;
    }

    void DebugVerifyMruList()
    {
        TreeNode* node = mMruListHead.mNext;
        int count = 0;

        while( node != (TreeNode*) &mMruListHead )
        {
            count++;
            node = node->mNext;
        }

        assert( count == mNodePoolEntries );
    }

    void Reset()
    {
        this->StopSearching();

        Position startPos;
        startPos.Reset();

        this->SetPosition( startPos );
    }

    double CalculateUct( TreeNode* node, int childIndex )
    {
        BranchInfo* nodeInfo    = node->mInfo;
        BranchInfo& childInfo   = node->mBranch[childIndex];

        u64 childWins       = childInfo.mScores.mWins;
        u64 childPlays      = childInfo.mScores.mPlays;
        u64 nodePlays       = nodeInfo->mScores.mPlays;
        float exploringness = mOptions->mExplorationFactor * 1.0f / 100;

        //assert( nodePlays > 0 );
        //assert( childPlays > 0 );

        double childWinRatio = childWins * 1.0 / childPlays;
        double uct = childWinRatio + exploringness * sqrt( log( nodePlays * 1.0 ) / childPlays );

        return uct;
    }

    int SelectNextBranch( TreeNode* node )
    {
        assert( node->mBranch.size() > 0 );

        // Choose the move with highest UCT, breaking ties randomly

        double highestUct = node->mBranch[0].mUct;
        int highestIdx = 0;

        for( int i = 1; i < (int) node->mBranch.size(); i++ )
        {
            if( node->mBranch[i].mUct > highestUct )
            {
                highestUct = node->mBranch[i].mUct;
                highestIdx = i;
            }
        }

        int bestIdx[MAX_MOVE_LIST];
        int bestCount = 0;

        for( int i = 0; i < (int) node->mBranch.size(); i++ )
            if( (node->mBranch[i].mUct == highestUct) || (i == highestIdx) )
                bestIdx[bestCount++] = i;

        int chosen = (int) mRandom.GetRange( bestCount );
        return bestIdx[chosen];
    }

    ScoreCard ExpandAtLeaf( MoveList& pathFromRoot, TreeNode* node )
    {
        // Mark each node LRU as we walk up the tree

        MoveToFront( node );

        int nextBranchIdx = SelectNextBranch( node );
        BranchInfo& nextBranch = node->mBranch[nextBranchIdx];

        pathFromRoot.Append( nextBranch.mMove );

        /*
        DEBUG_LOG( "ExpandAtLeaf %s choosing %d/%d (%s)\n",
            pathFromRoot.mCount? SerializeMoveList( pathFromRoot ).c_str() : "(root)",
            nextBranchIdx, node->mBranch.size(), SerializeMoveSpec( nextBranch.mMove ).c_str() );
            */

        if( !nextBranch.mNode )
        {
            // This is a leaf, so create a new node 

            TreeNode* newNode = AllocNode();
            newNode->Init( node->mPos, &nextBranch ); 

            nextBranch.mNode = newNode;

            for( int i = 0; i < (int) node->mBranch.size(); i++ )
                this->CalculateUct( node, i );

            node->mNumChildren++;
            assert( node->mNumChildren <= (int) node->mBranch.size() );
           
            // Expand one of its moves at random

            int newBranchIdx = (int) mRandom.GetRange( newNode->mBranch.size() );

            BranchInfo& newBranch = newNode->mBranch[newBranchIdx];
            pathFromRoot.Append( newBranch.mMove );

            //DEBUG_LOG( "New node! %s expanding branch %d (%s)\n", SerializeMoveList( pathFromRoot ).c_str(), newBranchIdx, SerializeMoveSpec( newBranch.mMove ).c_str() );

            Position newPos = node->mPos;
            newPos.Step( newBranch.mMove );

            ScoreCard scores;
            PlayoutJob job;

            job.mOptions        = *mOptions;
            job.mRandomSeed     = mRandom.GetNext();
            job.mPosition       = newPos;
            job.mNumGames       = mOptions->mNumInitialPlays;
            job.mPathFromRoot   = pathFromRoot;
            
            if( mOptions->mNumInitialPlays > 0 )
            {
                // Do the initial playouts

                PlayoutResult jobResult = RunPlayoutJobCpu( job );
                scores += jobResult.mScores;

                jobResult.mScores.Print( "Initial playout" );
            }
            else
            {
                // (or just pretend we did)
                
                scores.mPlays = 1;
                scores.mDraws = 1;
            }

            if( mOptions->mNumAsyncPlays > 0 )
            {
                // Queue up any async playouts

                PlayoutJobRef asyncJob( new PlayoutJob() );

                *asyncJob = job;
                asyncJob->mNumGames = mOptions->mNumAsyncPlays;

                // This will BLOCK when the job queue fills up

                mJobQueue.Push( asyncJob );
            }

            newBranch.mScores += scores;

            scores.FlipColor();
            newNode->mInfo->mScores += scores;

            newBranch.mUct = CalculateUct( newNode, newBranchIdx );

            scores.FlipColor();
            return scores;
        }

        ScoreCard branchScores = ExpandAtLeaf( pathFromRoot, nextBranch.mNode );

        // Accumulate the scores on our way back down the tree

        nextBranch.mScores += branchScores;
        nextBranch.mUct = CalculateUct( node, nextBranchIdx );

        branchScores.FlipColor();
        return branchScores;
    }

    void ProcessResult( TreeNode* node, const PlayoutResultRef& result, int depth = 0 )
    {
        if( node == NULL )
            return;

        MoveSpec move = result->mPathFromRoot.mMove[depth];

        int childIdx = node->FindMoveIndex( move );
        if( childIdx < 0 )
            return;

        TreeNode* child = node->mBranch[childIdx].mNode;
        if( child == NULL )
            return;

        ProcessResult( child, result, depth + 1 );

        ScoreCard scores = result->mScores;

        bool otherColor = (result->mPathFromRoot.mCount & 1) != 0;
        if( otherColor )
            scores.FlipColor();

        node->mBranch[childIdx].mScores += scores;
        node->mBranch[childIdx].mUct = CalculateUct( node, childIdx );
    }

    void DumpStats( TreeNode* node )
    {
        u64 bestDenom = 0;
        int bestDenomIdx = 0;

        float bestRatio = 0;
        int bestRatioIdx = 0;

        for( int i = 0; i < (int) node->mBranch.size(); i++ )
        {
            if( node->mBranch[i].mScores.mPlays > bestDenom )
            {
                bestDenom = node->mBranch[i].mScores.mPlays;
                bestDenomIdx = i;
            }

            if( node->mBranch[i].mScores.mPlays > 0 )
            {
                float ratio = node->mBranch[i].mScores.mWins * 1.0f / node->mBranch[i].mScores.mPlays;

                if( ratio > bestRatio )
                {
                    bestRatio = ratio;
                    bestRatioIdx = i;
                }
            }


        }

        for( int i = 0; i < (int) node->mBranch.size(); i++ )
        {
            std::string moveText = SerializeMoveSpec( node->mBranch[i].mMove );
            printf( "%s%s  %5s %.14f %12ld/%12ld\n", 
                (i == bestRatioIdx)? ">" : " ", 
                (i == bestDenomIdx)? "*" : " ", 
                moveText.c_str(), 
                node->mBranch[i].mUct, node->mBranch[i].mScores.mWins, node->mBranch[i].mScores.mPlays );
        }
    }

    void UpdateAsyncWorkers()
    {
        PROFILER_SCOPE( "TreeSearcher::UpdateAsyncWorkers" );

        for( auto& worker : mAsyncWorkers )
            worker->Update();
    }

    void ProcessAsyncResults()
    {
        PROFILER_SCOPE( "TreeSearcher::ProcessAsyncResults" );

        std::vector< PlayoutResultRef > results = mResultQueue.PopAll();

        for( const auto& result : results )
            this->ProcessResult( mSearchRoot, result );
    }

    void ExpandAtLeaf()
    {
        PROFILER_SCOPE( "TreeSearcher::ExpandAtLeaf" );

        MoveList pathFromRoot;
        ScoreCard rootScores = this->ExpandAtLeaf( pathFromRoot, mSearchRoot );

        mSearchRoot->mInfo->mScores += rootScores;

        int chosenBranch = mSearchRoot->FindMoveIndex( pathFromRoot.mMove[0] );
        assert( chosenBranch >= 0 );

        mSearchRoot->mInfo->mUct = CalculateUct( mSearchRoot, chosenBranch );
    }

    void SearchThread()
    {
        for( ;; )
        {
            // Wait until we're needed

            mSearchThreadIdle.Post();
            mSearchThreadActive.Wait();

            // Run until we're not

            if( mShuttingDown )
                break;

            int counter = 0;

            while( mSearchRunning )
            {
                PROFILER_SCOPE( "TreeSearcher::SearchThread" );

                //this->UpdateAsyncWorkers();
                //this->ProcessAsyncResults();

                this->ExpandAtLeaf();

                if( (counter % 10000) == 0 )
                {
                    printf( "\n%d:\n", counter );
                    this->DumpStats( mSearchRoot );
                }
                counter++;
            }
        }
    }

    void StartSearching( const UciSearchConfig& config )
    {
        this->StopSearching();

        mUciConfig = config;

        mSearchRunning  = true;
        mSearchThreadActive.Post();
    }

    void StopSearching()
    {
        if( mSearchRunning )
        {
            mSearchRunning = false;
            mSearchThreadIdle.Wait();
        }
    }
};


