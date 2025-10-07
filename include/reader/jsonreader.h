#ifndef JSONREADER_H
#define JSONREADER_H

#include "tracereader.h"
#include "rapidjson/document.h"

class JsonReader : public TraceReader{
public:
    JsonReader() = default;

    ~JsonReader() { close();}

    bool initialize(AllData& alldata) override;
    void close() override;

    bool readDefinitions(AllData& alldata) override;
    bool readEvents(AllData& alldata) override;
    bool readStatistics(AllData& alldata) override;
    bool readSystemTree(AllData& alldata);

private:
    rapidjson::Document document;
};

#endif
