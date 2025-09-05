/*
 This is part of the OTF-Profiler. Copyright by ZIH, TU Dresden 2016-2018.
 Authors: Maximillian Neumann, Denis Hünich, Jens Doleschal
*/

#ifndef ALLDATA_H
#define ALLDATA_H

#include "data_tree.h"
#include "definitions.h"
#include "utils.h"

/* *** management and statistics data structures, needed on all ranks ***
 *
 * @note I/O Statistics are collected within the callback for `IoOperationComplete`-events
 * */
struct AllData {
    /* data_tree */
    data_tree call_path_tree;
    /*meta meta*/
    meta_data metaData;

	/* Definitions include global definitions like: regions, paradigms, metrics, I/O paradigms, I/O handles, .. */
    definitions::Definitions definitions;
    uint64_t                 traceID;

    /* program parameters */
    Params params;

    /* runtime measurement */
    TimeMeasurement tm;

    /* I/O summary per paradigm */
    std::map<uint64_t, IoData> io_data_per_paradigm;
	/* I/O Statistics per file
	 * @note get corresponding @ref{IoHandle} by calling `auto h = alldata->definitions.iohandles.get(handle);` (eg to retrieve file name)
	 * */
    std::map<OTF2_IoHandleRef, IoData> io_data_per_file; // NEXT: TODO
	/* I/O Statistics per file
	 * @note get corresponding @ref{IoHandle} by calling `auto h = alldata->definitions.regions.get(handle);` (eg to retrieve function name)
	 * @TODO use to output per-function summary (eg of 50 most I/O-intensive functions - by time and size)
	 * */
	std::map<OTF2_LocationRef, IoData> io_data_per_location; // NEXT: TODO

    AllData(uint32_t my_rank = 0, uint32_t num_ranks = 1) {
        metaData.myRank   = my_rank;
        metaData.numRanks = num_ranks;
    }

    void verbosePrint(uint8_t vlevel, bool master_only, std::string msg) {
        if (params.verbose_level < vlevel)
            return;

        if (master_only) {
            if (metaData.myRank == 0)
                std::cout << msg << std::endl;
            return;
        }

        std::cout << "[" << metaData.myRank << "] " << msg << std::endl;
    }
};

#endif /* DATASTRUCTS_H */
