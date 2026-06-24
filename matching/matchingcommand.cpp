



#include "matchingcommand.h"

MatchingCommand::MatchingCommand(const int argc, char **argv) : CommandParser(argc, argv) {
    options_key[OptionKeyword::QueryGraphFile] = "-q";
    options_key[OptionKeyword::DataGraphFile] = "-d";
    options_key[OptionKeyword::Filter] = "-filter";
    options_key[OptionKeyword::Order] = "-order";
    options_key[OptionKeyword::Engine] = "-engine";
    options_key[OptionKeyword::MaxOutputEmbeddingNum] = "-num";
    options_key[OptionKeyword::SpectrumAnalysisTimeLimit] = "-time_limit";
    options_key[OptionKeyword::EnableEquivalence] = "-equivalent";
    processOptions();
};

void MatchingCommand::processOptions() {
    
    options_value[OptionKeyword::QueryGraphFile] = getCommandOption(options_key[OptionKeyword::QueryGraphFile]);;

    options_value[OptionKeyword::DataGraphFile] = getCommandOption(options_key[OptionKeyword::DataGraphFile]);

    options_value[OptionKeyword::Algorithm] = getCommandOption(options_key[OptionKeyword::Algorithm]);
    
    options_value[OptionKeyword::Filter] = getCommandOption(options_key[OptionKeyword::Filter]);
    
    options_value[OptionKeyword::Order] = getCommandOption(options_key[OptionKeyword::Order]);
    
    options_value[OptionKeyword::Engine] = getCommandOption(options_key[OptionKeyword::Engine]);
    
    options_value[OptionKeyword::MaxOutputEmbeddingNum] = getCommandOption(options_key[OptionKeyword::MaxOutputEmbeddingNum]);

    options_value[OptionKeyword::EnableEquivalence] = getCommandOption(options_key[OptionKeyword::EnableEquivalence]);
}