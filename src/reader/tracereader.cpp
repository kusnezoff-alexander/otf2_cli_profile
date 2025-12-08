/*
 This is part of the OTF-Profiler. Copyright by ZIH, TU Dresden 2016-2018.
 Authors: Maximillian Neumann, Denis Hünich, Jens Doleschal
*/

#include <iostream>
#include <string>

#include "otf-profiler-config.h"
#include "tracereader.h"

#if defined HAVE_OPEN_TRACE_FORMAT
#include "OTFReader.h"
#endif

#if defined HAVE_OTF2
#include "OTF2Reader.h"
#endif

using namespace std;

unique_ptr<TraceReader> getTraceReader(AllData& alldata) {
    string filename                  = alldata.params.input_file_name;
    auto   n                         = filename.rfind(".");
    alldata.params.input_file_prefix = filename.substr(0, n);

    string filetype = filename.substr(n + 1);

    if (filetype == "otf") {
        cerr << "ERROR: Can't process OTF files! Only OTF2 is supported" << endl;
        return nullptr;
    } else if (filetype == "otf2") {
        return unique_ptr<OTF2Reader>(new OTF2Reader);
    } else if (filetype == "json"){
        cerr << "ERROR: Can't process json files! Support was dropped." << endl;
        return nullptr;
    } else
        cerr << "ERROR: Unknown file type!" << endl;

    return nullptr;
}
