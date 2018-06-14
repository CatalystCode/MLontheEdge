////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     CompiledProfile_main.cpp (profile)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// compiled model
#include "compiled_model.h"
#include "ProfileReport.h"

// stl
#include <algorithm>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

struct ProfileArguments
{
    std::string outputFilename;
    std::string outputComment;
    ProfileOutputFormat outputFormat;
    int numIterations;
    int numWarmUpIterations;
};

//
// Test-data-related
//
template<typename T>
std::vector<T> GetInputData(std::string filename, const TensorShape& inputShape, float scale)
{
    std::default_random_engine engine(123);
    auto inputSize = inputShape.rows * inputShape.columns * inputShape.channels;
    std::vector<T> result(inputSize);
    std::uniform_real_distribution<double> dist;
    for (auto index = 0; index < inputSize; ++index)
    {
        result[index] = static_cast<T>(dist(engine));
    }
    return result;
}

//
// Output-related
//

void WriteModelStatistics(ProfileOutputFormat format, std::ostream& out)
{
    // get overall stats
    auto modelStats = ELL_GetModelPerformanceCounters();
    WriteModelStatistics(modelStats, format, out);
}

void WriteNodeStatistics(ProfileOutputFormat format, std::ostream& out)
{
    // Gather node statistics
    std::vector<std::pair<ELL_NodeInfo, ELL_PerformanceCounters>> nodeInfo;
    auto numNodes = ELL_GetNumNodes();
    for (int index = 0; index < numNodes; ++index)
    {
        auto info = ELL_GetNodeInfo(index);
        auto stats = ELL_GetNodePerformanceCounters(index);
        nodeInfo.emplace_back(*info, *stats);
    }

    std::vector<std::pair<ELL_NodeInfo, ELL_PerformanceCounters>> nodeTypeInfo;
    auto numNodeTypes = ELL_GetNumNodeTypes();
    for (int index = 0; index < numNodeTypes; ++index)
    {
        auto info = ELL_GetNodeTypeInfo(index);
        auto stats = ELL_GetNodeTypePerformanceCounters(index);
        nodeTypeInfo.emplace_back(*info, *stats);
    }
    std::sort(nodeTypeInfo.begin(), nodeTypeInfo.end(), [](auto a, auto b) { return a.second.totalTime < b.second.totalTime; });

    WriteNodeStatistics(nodeInfo, nodeTypeInfo, format, out);
}

void WriteRegionStatistics(ProfileOutputFormat format, std::ostream& out)
{
    // Gather region statistics
    std::vector<ELL_ProfileRegionInfo> regions;
    auto numRegions = ELL_GetNumProfileRegions();
    size_t maxNameLength = 0;
    for (int index = 0; index < numRegions; ++index)
    {
        auto info = ELL_GetRegionProfilingInfo(index);
        maxNameLength = std::max(maxNameLength, std::strlen((const char*)(info->name)));
        regions.emplace_back(*info);
    }

    WriteRegionStatistics(regions, format, out);
}

//
// Profiling functions
//
void ResetProfilingInfo()
{
    ELL_ResetModelProfilingInfo();
    ELL_ResetNodeProfilingInfo();
    ELL_ResetNodeTypeProfilingInfo();
    ELL_ResetRegionProfilingInfo();
}

template<typename InputType, typename OutputType>
void ProfileModel(const ProfileArguments& profileArguments)
{
    auto& profileOutputStream = std::cout;
    const auto comment = profileArguments.outputComment;

    TensorShape inputShape;
    TensorShape outputShape;
    ELL_GetInputShape(0, &inputShape);
    ELL_GetOutputShape(0, &outputShape);

    auto inputSize = ELL_GetInputSize();
    auto outputSize = ELL_GetOutputSize();

    std::vector<InputType> input(inputSize);
    std::vector<OutputType> output(outputSize);

    // Warm up the system by evaluating the model some number of times
    for (int iter = 0; iter < profileArguments.numWarmUpIterations; ++iter)
    {
        ELL_Predict(nullptr, input.data(), output.data());
    }
    ResetProfilingInfo();

    // Now evaluate the model and record the profiling info
    for (int iter = 0; iter < profileArguments.numIterations; ++iter)
    {
        // Exercise the model
        ELL_Predict(nullptr, input.data(), output.data());
    }

    auto format = profileArguments.outputFormat;

    // print profile info
    if (format == ProfileOutputFormat::text)
    {
        if (!comment.empty())
        {
            WriteUserComment(comment, format, profileOutputStream);
        }
        WriteNodeStatistics(format, profileOutputStream);
        WriteRegionStatistics(format, profileOutputStream);
        WriteModelStatistics(format, profileOutputStream);
    }
    else
    {
        profileOutputStream << "{\n";
        if (!comment.empty())
        {
            WriteUserComment(comment, format, profileOutputStream);
            profileOutputStream << ",\n";
        }
        WriteNodeStatistics(format, profileOutputStream);
        profileOutputStream << ",\n";
        WriteRegionStatistics(format, profileOutputStream);
        profileOutputStream << ",\n";
        WriteModelStatistics(format, profileOutputStream);
        profileOutputStream << "}\n";
    }
}

int main(int argc, char* argv[])
{
    using InputType = float;
    using OutputType = float;

    int numIterations = 20;
    int numWarmUpIterations = 10;
    if(argc > 1)
    {
        numIterations = atoi(argv[1]);
    }
    if(argc > 2)
    {
        numWarmUpIterations = atoi(argv[2]);
    }

    std::cout << "Profiling model with " << numWarmUpIterations << " warm-up iterations and " << numIterations << " timed iterations" << std::endl;

    // add arguments to the command line parser
    ProfileArguments profileArguments;
    profileArguments.numWarmUpIterations = numWarmUpIterations;
    profileArguments.numIterations = numIterations;
    profileArguments.outputFormat = ProfileOutputFormat::text;
    ProfileModel<InputType, OutputType>(profileArguments);

    return 0;
}
