// JAGLAVAK CHESS ENGINE (c) 2019 Stuart Riffle
// GENERATED CODE - DO NOT EDIT THIS

const char* Embedded_DefaultSettings = R"EMBEDDED_FILE(
{
    "CPU.Enabled":              { "value": 1,       "desc": "Run playouts on the CPU" },   
    "CPU.Multicore":            { "value": 1,       "desc": "Use all available CPU cores" },   
    "CPU.AffinityMask":         { "value": 0,       "desc": "" },      
    "CPU.DispatchThreads":      { "value": 4,       "desc": "" },
    "CPU.SearchFibers":         { "value": 8192,      "desc": "" },       
    "CPU.SIMD.Enabled":         { "value": 0,       "desc": "Detect and use SIMD instructions on CPU" },
    "CPU.SIMD.ForceLevel":      { "value": 0,       "desc": "Override detected SIMD level [1, 2, 4, 8]" },

    "CUDA.Enabled":             { "value": 1,       "desc": "Run playouts on CUDA devices" },      
    "CUDA.HeapMegs":            { "value": 64,      "desc": "CUDA heap size" },     
    "CUDA.AffinityMask":        { "value": 1,       "desc": "" },      
    "CUDA.BatchSize":           { "value": 8192,    "desc": "" },       

    "Search.DrawsWorthHalf":    { "value": 0,       "desc": "" },      
    "Search.NumPlayoutMoves":   { "value": 200,     "desc": "" },        
    "Search.NumPlayouts":       { "value": 100,       "desc": "" },  
    "Search.ExplorationFactor": { "value": 1.41,    "desc": "" },
    "Search.BranchesToExpand":  { "value": 1,       "desc": "A value of zero means to expand them all." },
    "Search.NumTreeNodes":      { "value": 10000000,  "desc": "" },   
    "Search.ForceRandomSeed":   { "value": 0,       "desc": "" },      
    "Search.BatchSize":         { "value": 128,     "desc": "" },      
    "Search.FlushEveryBatch":   { "value": 0,       "desc": "" },     
    "Search.VirtualLoss":       { "value": 0,     "desc": "" },  


    "UCI.TimeSafetyMargin":     { "value": 100,     "desc": "" },        
    "UCI.UpdateTime":           { "value": 3000,     "desc": "" }
}


)EMBEDDED_FILE";
