////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     ProfileArguments.h (profile)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ProfileReport.h"

// utilities
#include "CommandLineParser.h"

// stl
#include <string>

namespace ell
{
/// <summary> Arguments for profile tool. </summary>
struct ProfileArguments
{
    std::string inputConverter;
    std::string outputFilename;
    std::string timingOutputFilename;
    ProfileOutputFormat outputFormat = ProfileOutputFormat::text;
    std::string outputComment;

    int numIterations = 1;
    int numBurnInIterations = 0;
    bool filterTrivialNodes = true;
    bool summaryOnly = false;

    // TODO: something about regions

};

/// <summary> Arguments for parsed print. </summary>
struct ParsedProfileArguments : public ProfileArguments, public utilities::ParsedArgSet
{
    /// <summary> Adds the arguments. </summary>
    ///
    /// <param name="parser"> [in,out] The parser. </param>
    void AddArgs(utilities::CommandLineParser& parser) override;
};
}
