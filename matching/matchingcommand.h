



#ifndef SUBGRAPHMATCHING_MATCHINGCOMMAND_H
#define SUBGRAPHMATCHING_MATCHINGCOMMAND_H

#include "utility/commandparser.h"
#include <map>
#include <iostream>
enum OptionKeyword {
    Algorithm = 0,          
    QueryGraphFile = 1,     
    DataGraphFile = 2,      
    ThreadCount = 3,        
    DepthThreshold = 4,     
    WidthThreshold = 5,     
    IndexType = 6,          
    Filter = 7,             
    Order = 8,              
    Engine = 9,             
    MaxOutputEmbeddingNum = 10, 
    SpectrumAnalysisTimeLimit = 11, 
    SpectrumAnalysisOrderNum = 12, 
    DistributionFilePath = 13,          
    CSRFilePath = 14,            
    EnableSymmetry = 15,
    EnableEquivalence = 16,
};

class MatchingCommand : public CommandParser{
private:
    std::map<OptionKeyword, std::string> options_key;
    std::map<OptionKeyword, std::string> options_value;

private:
    void processOptions();

public:
    MatchingCommand(int argc, char **argv);

    std::string getDataGraphFilePath() {
        return options_value[OptionKeyword::DataGraphFile];
    }

    std::string getQueryGraphFilePath() {
        return options_value[OptionKeyword::QueryGraphFile];
    }

    std::string getFilterType() {
        return options_value[OptionKeyword::Filter] == "" ? "CFL" : options_value[OptionKeyword::Filter];
    }

    std::string getOrderType() {
        return options_value[OptionKeyword::Order] == "" ? "GQL" : options_value[OptionKeyword::Order];
    }

    std::string getEngineType() {
        return options_value[OptionKeyword::Engine] == "" ? "LFTJ" : options_value[OptionKeyword::Engine];
    }

    std::string getMaximumEmbeddingNum() {
        return options_value[OptionKeyword::MaxOutputEmbeddingNum] == "" ? "MAX" : options_value[OptionKeyword::MaxOutputEmbeddingNum];
    }

    std::string getTimeLimit() {
        return options_value[OptionKeyword::SpectrumAnalysisTimeLimit] == "" ? "5" : options_value[OptionKeyword::SpectrumAnalysisTimeLimit];
    }

    std::string getOrderNum() {
        return options_value[OptionKeyword::SpectrumAnalysisOrderNum] == "" ? "100" : options_value[OptionKeyword::SpectrumAnalysisOrderNum];
    }

    std::string getEnableEquivalence() {
        return options_value[OptionKeyword::EnableEquivalence] == "" ? "" : options_value[OptionKeyword::EnableEquivalence];
    }
};


#endif 
