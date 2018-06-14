////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     NodesTests.cpp (nodes_test)
//  Authors:  Chuck Jacobs, Byron Changuion, Kern Handa
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "NodesTests.h"
#include "NodesTestData.h"

// nodes
#include "AccumulatorNode.h"
#include "BatchNormalizationLayerNode.h"
#include "BiasLayerNode.h"
#include "BinaryOperationNode.h"
#include "BufferNode.h"
#include "ClockNode.h"
#include "ConcatenationNode.h"
#include "ConstantNode.h"
#include "DemultiplexerNode.h"
#include "FilterBankNode.h"
#include "ForestPredictorNode.h"
#include "IIRFilterNode.h"
#include "L2NormSquaredNode.h"
#include "LinearPredictorNode.h"
#include "MatrixVectorProductNode.h"
#include "MovingAverageNode.h"
#include "MovingVarianceNode.h"
#include "NeuralNetworkLayerNode.h"
#include "NeuralNetworkPredictorNode.h"
#include "ProtoNNPredictorNode.h"
#include "RegionDetectionLayerNode.h"
#include "SinkNode.h"
#include "SourceNode.h"
#include "SquaredEuclideanDistanceNode.h"
#include "TypeCastNode.h"
#include "UnaryOperationNode.h"

// math
#include "MathConstants.h"
#include "Tensor.h"
#include "TensorOperations.h"

// model
#include "InputNode.h"
#include "Model.h"
#include "Node.h"

// predictors
#include "LinearPredictor.h"
#include "NeuralNetworkPredictor.h"

// predictors/neural
#include "BatchNormalizationLayer.h"
#include "BiasLayer.h"
#include "InputLayer.h"
#include "ProtoNNPredictor.h"

// testing
#include "testing.h"

// utilities
#include "RandomEngines.h"

// stl
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

using namespace ell;
using namespace nodes;

//
// Helpers
//
namespace
{
double VectorMagnitudeSquared(const std::vector<double>& vec)
{
    double sumSq = 0.0;
    for (const auto& x : vec)
        sumSq += (x * x);

    return sumSq;
}

double VectorMean(const std::vector<double>& vec)
{
    double sum = 0.0;
    for (const auto& x : vec)
        sum += x;

    return sum / vec.size();
}

double VectorVariance(const std::vector<double>& vec, double mean)
{
    if (vec.size() == 0)
        return 0.0;
    double var = 0.0;
    for (auto x : vec)
    {
        double diff = x - mean;
        var += diff * diff;
    }
    return var / vec.size();
}

template <typename ValueType>
std::ostream& operator<<(std::ostream& os, const std::vector<ValueType>& vec)
{
    os << "[";
    for (auto x : vec)
    {
        os << x << " ";
    }
    os << "]";
    return os;
}

template <typename ValueType>
class Uniform
{
public:
    Uniform(ValueType minVal, ValueType maxVal, std::string seed = "123")
        : _rng(utilities::GetRandomEngine(seed)), _range(static_cast<double>(_rng.max() - _rng.min())), _minOutput(minVal), _outputRange(maxVal - minVal) {}

    ValueType operator()()
    {
        double uniform = static_cast<double>(_rng()) / _range;
        return static_cast<ValueType>((uniform * _outputRange) + _minOutput);
    }

private:
    std::default_random_engine _rng;
    double _range;
    ValueType _minOutput;
    ValueType _outputRange;
};

template <typename ElementType>
void FillRandomVector(std::vector<ElementType>& vector, ElementType min = -1, ElementType max = 1)
{
    Uniform<ElementType> rand(min, max);
    std::generate(vector.begin(), vector.end(), rand);
}
}

//
// Test compute functions
//

static void TestL2NormSquaredNodeCompute()
{
    std::vector<std::vector<double>> data = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 }, { 9 }, { 10 } };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(data[0].size());
    auto outputNode = model.AddNode<nodes::L2NormSquaredNode<double>>(inputNode->output);

    for (size_t index = 0; index < data.size(); ++index)
    {
        auto inputValue = data[index];
        double expectedOutput = VectorMagnitudeSquared(inputValue);

        inputNode->SetInput(inputValue);
        std::vector<double> outputVec = model.ComputeOutput(outputNode->output);

        testing::ProcessTest("Testing L2NormSquaredNode output size", outputVec.size() == 1);
        testing::ProcessTest("Testing L2NormSquaredNode compute", testing::IsEqual(outputVec[0], expectedOutput));
    }
}

static void TestAccumulatorNodeCompute()
{
    std::vector<std::vector<double>> data = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 }, { 9 }, { 10 } };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(data[0].size());
    auto outputNode = model.AddNode<nodes::AccumulatorNode<double>>(inputNode->output);

    std::vector<double> accumOutput(data[0].size());

    for (size_t index = 0; index < data.size(); ++index)
    {
        auto inputValue = data[index];
        for (size_t d = 0; d < inputValue.size(); d++)
        {
            accumOutput[d] += inputValue[d];
        }
        inputNode->SetInput(inputValue);
        std::vector<double> outputVec = model.ComputeOutput(outputNode->output);

        testing::ProcessTest("Testing AccumulatorNode compute", testing::IsEqual(outputVec, accumOutput));
    }
}

static void TestMovingAverageNodeCompute()
{
    const int windowSize = 4;

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(1);
    auto outputNode = model.AddNode<nodes::MovingAverageNode<double>>(inputNode->output, windowSize);

    std::vector<std::vector<double>> data = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 }, { 9 }, { 10 } };
    double expectedOutput = VectorMean({ 7.0, 8.0, 9.0, 10.0 });

    std::vector<double> outputVec;

    for (const auto& inputValue : data)
    {
        inputNode->SetInput(inputValue);
        outputVec = model.ComputeOutput(outputNode->output);
    }
    testing::ProcessTest("Testing MovingAverageNode compute", testing::IsEqual(outputVec[0], expectedOutput));
}

static void TestMovingVarianceNodeCompute()
{
    const int windowSize = 4;

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(1);
    auto outputNode = model.AddNode<nodes::MovingVarianceNode<double>>(inputNode->output, windowSize);

    std::vector<std::vector<double>> data = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 }, { 9 }, { 10 } };
    double mean = VectorMean({ 7.0, 8.0, 9.0, 10.0 });
    double expectedOutput = VectorVariance({ 7.0, 8.0, 9.0, 10.0 }, mean);

    std::vector<double> outputVec;

    for (const auto& inputValue : data)
    {
        inputNode->SetInput(inputValue);
        outputVec = model.ComputeOutput(outputNode->output);
    }
    testing::ProcessTest("Testing MovingVarianceNode compute", testing::IsEqual(outputVec[0], expectedOutput));
}

static void TestUnaryOperationNodeCompute(emitters::UnaryOperationType op, double (*expectedTransform)(double))
{
    std::vector<std::vector<double>> data = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 }, { 9 }, { 10 } };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(data[0].size());
    auto outputNode = model.AddNode<nodes::UnaryOperationNode<double>>(inputNode->output, op);

    for (size_t index = 0; index < data.size(); ++index)
    {
        auto inputValue = data[index];

        inputNode->SetInput(inputValue);
        std::vector<double> outputVec = model.ComputeOutput(outputNode->output);

        for (size_t d = 0; d < inputValue.size(); ++d)
        {
            auto expectedOutput = expectedTransform(inputValue[d]);
            testing::ProcessTest("Testing UnaryOperationNode compute for " + UnaryOperations::to_string(op),
                                 testing::IsEqual(outputVec[d], expectedOutput));
        }
    }
}

static void TestUnaryOperationNodeCompute(emitters::UnaryOperationType op, bool (*expectedTransform)(bool))
{
    std::vector<std::vector<bool>> data = { { true }, { false } };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<bool>>(data[0].size());
    auto outputNode = model.AddNode<nodes::UnaryOperationNode<bool>>(inputNode->output, op);

    for (size_t index = 0; index < data.size(); ++index)
    {
        auto inputValue = data[index];

        inputNode->SetInput(inputValue);
        std::vector<bool> outputVec = model.ComputeOutput(outputNode->output);

        for (size_t d = 0; d < inputValue.size(); ++d)
        {
            auto expectedOutput = expectedTransform(inputValue[d]);
            testing::ProcessTest("Testing UnaryOperationNode compute for " + UnaryOperations::to_string(op),
                                 testing::IsEqual(outputVec[d], expectedOutput));
        }
    }
}

static void TestUnaryOperationNodeCompute()
{
    TestUnaryOperationNodeCompute(emitters::UnaryOperationType::exp, std::exp);
    TestUnaryOperationNodeCompute(emitters::UnaryOperationType::log, std::log);
    TestUnaryOperationNodeCompute(emitters::UnaryOperationType::sqrt, std::sqrt);
    TestUnaryOperationNodeCompute(emitters::UnaryOperationType::logicalNot, [](bool b) { return !b; });
    TestUnaryOperationNodeCompute(emitters::UnaryOperationType::square, [](double d) { return d * d; });
    TestUnaryOperationNodeCompute(emitters::UnaryOperationType::tanh, std::tanh);
}

static void TestBinaryOperationNodeCompute()
{
    std::vector<std::vector<double>> data = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 }, { 9 }, { 10 } };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(data[0].size());
    auto outputNode = model.AddNode<nodes::BinaryOperationNode<double>>(inputNode->output, inputNode->output, emitters::BinaryOperationType::add);

    for (size_t index = 0; index < data.size(); ++index)
    {
        auto inputValue = data[index];

        inputNode->SetInput(inputValue);
        std::vector<double> outputVec = model.ComputeOutput(outputNode->output);

        for (size_t d = 0; d < inputValue.size(); ++d)
        {
            auto expectedOutput = 2 * inputValue[d];
            testing::ProcessTest("Testing BinaryOperationNode compute", testing::IsEqual(outputVec[d], expectedOutput));
        }
    }
}

static void TestBinaryOperationNodeCompute2()
{
    model::Model model;
    int numRows = 2;
    int numColumns = 2;
    int numChannels = 2;
    int padding = 1;

    model::PortMemoryLayout input1Shape({ numRows, numColumns, numChannels }, { padding, padding, 0 });
    model::PortMemoryLayout input2Shape({ numRows, numColumns, numChannels });
    model::PortMemoryLayout outputShape({ numRows, numColumns, numChannels });

    auto input1Node = model.AddNode<model::InputNode<double>>(input1Shape.GetMemorySize());
    auto constantNode = model.AddNode<nodes::ConstantNode<double>>(std::vector<double>{ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 });
    auto outputNode = model.AddNode<nodes::BinaryOperationNode<double>>(input1Node->output, input1Shape, constantNode->output, input2Shape, outputShape, emitters::BinaryOperationType::add, 0);
    auto map = model::Map(model, { { "input", input1Node } }, { { "output", outputNode->output } });

    std::vector<double> expected { 2, 4, 6, 8, 10, 12, 14, 16 };
    std::vector<double> input { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                0.0, 0.0, 1.0, 2.0, 3.0, 4.0, 0.0, 0.0,
                                0.0, 0.0, 5.0, 6.0, 7.0, 8.0, 0.0, 0.0,
                                0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    auto result = map.Compute<double>(input);
    testing::ProcessTest("TestBinaryOperationNodeCompute2", testing::IsEqual(result, expected));
}

template <typename ElementType>
static void TestLinearPredictorNodeCompute()
{
    const int dim = 5;
    math::ColumnVector<ElementType> weights({ 1, 2, 3, 4, 5 });
    ElementType bias = 1.5f;

    predictors::LinearPredictor<ElementType> predictor(weights, bias);

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<ElementType>>(dim);
    auto predictorNode = model.AddNode<nodes::LinearPredictorNode<ElementType>>(inputNode->output, predictor);
    auto outputNode = model.AddNode<model::OutputNode<ElementType>>(predictorNode->output);

    auto map = model::Map(model, { { "input", inputNode } }, { { "output", outputNode->output } });

    std::vector<ElementType> input{ 1.0, 2.0, 1.0, -1.0, 0.5 };
    auto result = map.Compute<ElementType>(input);

    testing::ProcessTest("TestLinearPredictorNodeCompute", testing::IsEqual(result[0], static_cast<ElementType>(8.0)));
}

static void TestDemultiplexerNodeCompute()
{
    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(1);
    auto selectorNode = model.AddNode<model::InputNode<bool>>(1);
    auto muxNode = model.AddNode<nodes::DemultiplexerNode<double, bool>>(inputNode->output, selectorNode->output, 2);

    std::vector<double> inputValue{ 5.0 };
    inputNode->SetInput(inputValue);

    selectorNode->SetInput(false); // output[0] should get the input
    auto outputVec = model.ComputeOutput(muxNode->output);
    testing::ProcessTest("Testing DemultiplexerNode compute", testing::IsEqual(outputVec, { 5.0, 0 }));

    selectorNode->SetInput(true); // output[1] should get the input
    outputVec = model.ComputeOutput(muxNode->output);
    testing::ProcessTest("Testing DemultiplexerNode compute", testing::IsEqual(outputVec, { 0.0, 5.0 }));
}

static void TestSourceNodeCompute()
{
    // Callbacks
    struct SourceNodeTester
    {
        void Initialize(const std::vector<std::vector<double>>& inputSeries)
        {
            it = inputSeries.begin();
            end = inputSeries.end();
            assert(it != end);
        }

        bool InputCallback(std::vector<double>& input)
        {
            if (it == end)
            {
                return false;
            }
            input = *it;
            it++;
            return true;
        }

    private:
        std::vector<std::vector<double>>::const_iterator it;
        std::vector<std::vector<double>>::const_iterator end;
    } sourceNodeTester;

    const std::vector<std::vector<double>> data = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 }, { 9 }, { 10 } };
    sourceNodeTester.Initialize(data);

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<nodes::TimeTickType>>(2);
    auto sourceNode = model.AddNode<nodes::SourceNode<double>>(inputNode->output, data[0].size(), "SourceFunction", [&sourceNodeTester](std::vector<double>& input) -> bool {
        return sourceNodeTester.InputCallback(input);
    });

    for (const auto& inputValue : data)
    {
        using namespace std::chrono_literals;

        const nodes::TimeTickType now = std::chrono::steady_clock::now().time_since_epoch().count();
        std::vector<nodes::TimeTickType> timeInput{ now - 50, now };
        inputNode->SetInput(timeInput);

        auto output = model.ComputeOutput(sourceNode->output);
        testing::ProcessTest("Testing SourceNode output", testing::IsEqual(output, inputValue));

        std::this_thread::sleep_for(2ms);
    }
}

void TestSinkNodeCompute(bool triggerValue)
{
    const std::vector<std::vector<double>> data = { { 12 }, { 10 }, { 8 }, { 6 }, { 4 }, { 2 } };

    std::vector<std::vector<double>> results;
    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(1);
    auto conditionNode = model.AddNode<nodes::ConstantNode<bool>>(triggerValue);
    auto sinkNode = model.AddNode<nodes::SinkNode<double>>(inputNode->output,
                                                           conditionNode->output,
                                                           "SinkFunction",
                                                           [&results](const std::vector<double>& values) {
                                                               results.push_back(values);
                                                           });

    for (const auto& inputValue : data)
    {
        inputNode->SetInput(inputValue);
        model.ComputeOutput(sinkNode->output);
    }

    if (triggerValue)
    {
        testing::ProcessTest("Testing SinkNode output (trigger = true)", testing::IsEqual(data, results));
    }
    else
    {
        testing::ProcessTest("Testing SinkNode output (trigger = true)", testing::IsTrue(results.empty()));
    }
}

static void TestSinkNodeCompute()
{
    TestSinkNodeCompute(true);
    TestSinkNodeCompute(false);
}

static void TestSquaredEuclideanDistanceNodeCompute()
{
    math::RowMatrix<double> m{
        { 1.0, 0.2, 0.3 },
        { 0.3, 0.7, 0.5 }
    };

    std::vector<double> input = { 1, 2, 3 };
    std::vector<double> output = { 10.53, 8.43 };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(input.size());
    auto sqEuclideanDistanceNode = model.AddNode<nodes::SquaredEuclideanDistanceNode<double, math::MatrixLayout::rowMajor>>(inputNode->output, m);

    inputNode->SetInput(input);
    auto computeOutput = model.ComputeOutput(sqEuclideanDistanceNode->output);

    testing::ProcessTest("Testing squared Euclidean distance node compute", testing::IsEqual(output, computeOutput));
}

//
// Node refinements
//

static void TestL2NormSquaredNodeRefine()
{
    std::vector<std::vector<double>> data = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 }, { 9 }, { 10 } };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(data[0].size());
    auto l2NormSquaredNode = model.AddNode<nodes::L2NormSquaredNode<double>>(inputNode->output);

    model::TransformContext context;
    model::ModelTransformer transformer;
    auto refinedModel = transformer.RefineModel(model, context);
    auto refinedInputNode = transformer.GetCorrespondingInputNode(inputNode);
    auto refinedOutputElements = transformer.GetCorrespondingOutputs(model::PortElements<double>{ l2NormSquaredNode->output });
    std::cout << "Original L2NormSquaredNode nodes: " << model.Size() << ", refined: " << refinedModel.Size() << std::endl;
    for (const auto& inputValue : data)
    {
        inputNode->SetInput(inputValue);
        auto outputVec1 = model.ComputeOutput(l2NormSquaredNode->output);
        refinedInputNode->SetInput(inputValue);
        auto outputVec2 = refinedModel.ComputeOutput(refinedOutputElements);

        testing::ProcessTest("Testing L2NormSquaredNode refine", testing::IsEqual(outputVec1, outputVec2));
    }
}

static void TestMovingAverageNodeRefine()
{
    const int windowSize = 4;

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(1);
    auto meanNode = model.AddNode<nodes::MovingAverageNode<double>>(inputNode->output, windowSize);

    std::vector<std::vector<double>> data = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 }, { 9 }, { 10 } };

    model::TransformContext context;
    model::ModelTransformer transformer;
    auto refinedModel = transformer.RefineModel(model, context);
    auto refinedInputNode = transformer.GetCorrespondingInputNode(inputNode);
    auto refinedOutputElements = transformer.GetCorrespondingOutputs(model::PortElements<double>{ meanNode->output });
    std::cout << "Original MovingAverageNode nodes: " << model.Size() << ", refined: " << refinedModel.Size() << std::endl;
    for (const auto& inputValue : data)
    {
        inputNode->SetInput(inputValue);
        auto outputVec1 = model.ComputeOutput(meanNode->output);
        refinedInputNode->SetInput(inputValue);
        auto outputVec2 = refinedModel.ComputeOutput(refinedOutputElements);

        testing::ProcessTest("Testing MovingAverageNode refine", testing::IsEqual(outputVec1, outputVec2));
    }
}

static void TestSimpleForestPredictorNodeRefine()
{
    // define some abbreviations
    using SplitAction = predictors::SimpleForestPredictor::SplitAction;
    using SplitRule = predictors::SingleElementThresholdPredictor;
    using EdgePredictorVector = std::vector<predictors::ConstantPredictor>;

    // build a forest
    predictors::SimpleForestPredictor forest;
    auto root = forest.Split(SplitAction{ forest.GetNewRootId(), SplitRule{ 0, 0.3 }, EdgePredictorVector{ -1.0, 1.0 } });
    forest.Split(SplitAction{ forest.GetChildId(root, 0), SplitRule{ 1, 0.6 }, EdgePredictorVector{ -2.0, 2.0 } });
    forest.Split(SplitAction{ forest.GetChildId(root, 1), SplitRule{ 2, 0.9 }, EdgePredictorVector{ -4.0, 4.0 } });
    forest.Split(SplitAction{ forest.GetNewRootId(), SplitRule{ 0, 0.2 }, EdgePredictorVector{ -3.0, 3.0 } });

    // build the model
    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(3);
    auto simpleForestPredictorNode = model.AddNode<nodes::SimpleForestPredictorNode>(inputNode->output, forest);

    // refine
    model::TransformContext context;
    model::ModelTransformer transformer;
    auto refinedModel = transformer.RefineModel(model, context);
    auto refinedInputNode = transformer.GetCorrespondingInputNode(inputNode);
    auto refinedOutputElements = transformer.GetCorrespondingOutputs(model::PortElements<double>{ simpleForestPredictorNode->output });
    auto refinedTreeOutputsElements = transformer.GetCorrespondingOutputs(model::PortElements<double>{ simpleForestPredictorNode->treeOutputs });
    auto refinedEdgeIndicatorVectorElements = transformer.GetCorrespondingOutputs(model::PortElements<bool>{ simpleForestPredictorNode->edgeIndicatorVector });

    // check equivalence
    inputNode->SetInput({ 0.18, 0.5, 0.0 });
    refinedInputNode->SetInput({ 0.18, 0.5, 0.0 });
    auto outputValue = model.ComputeOutput(simpleForestPredictorNode->output)[0];
    auto treeOutputsValue = model.ComputeOutput(simpleForestPredictorNode->treeOutputs);
    auto edgeIndicatorVectorValue = model.ComputeOutput(simpleForestPredictorNode->edgeIndicatorVector);

    auto refinedOutputValue = refinedModel.ComputeOutput(refinedOutputElements)[0];
    auto refinedTreeOutputsValue = refinedModel.ComputeOutput(refinedTreeOutputsElements);
    auto refinedEdgeIndicatorVectorValue = refinedModel.ComputeOutput(refinedEdgeIndicatorVectorElements);

    //  expected output is -3.0
    testing::ProcessTest("Testing SimpleForestPredictorNode refine (output)", testing::IsEqual(outputValue, refinedOutputValue));
    testing::ProcessTest("Testing SimpleForestPredictorNode refine (treeOutputs)", testing::IsEqual(treeOutputsValue, refinedTreeOutputsValue));
    testing::ProcessTest("Testing SimpleForestPredictorNode refine (edgeIndicatorVector)", testing::IsEqual(edgeIndicatorVectorValue, refinedEdgeIndicatorVectorValue));
}

static void TestSquaredEuclideanDistanceNodeRefine()
{
    math::RowMatrix<double> m{
        { 1.0, 0.2, 0.3 },
        { 0.3, 0.7, 0.5 }
    };

    std::vector<double> input = { 1, 2, 3 };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(input.size());
    auto sqEuclideanDistanceNode = model.AddNode<nodes::SquaredEuclideanDistanceNode<double, math::MatrixLayout::rowMajor>>(inputNode->output, m);
    model::TransformContext context;
    model::ModelTransformer transformer;
    auto refinedModel = transformer.RefineModel(model, context);
    auto refinedInputNode = transformer.GetCorrespondingInputNode(inputNode);
    auto refinedOutputElements = transformer.GetCorrespondingOutputs(model::PortElements<double>{ sqEuclideanDistanceNode->output });
    std::cout << "Original SquaredEuclideanDistanceNode nodes: " << model.Size() << ", refined: " << refinedModel.Size() << std::endl;

    inputNode->SetInput(input);
    auto outputVec1 = model.ComputeOutput(sqEuclideanDistanceNode->output);
    refinedInputNode->SetInput(input);
    auto outputVec2 = refinedModel.ComputeOutput(refinedOutputElements);

    testing::ProcessTest("Testing SquaredEuclideanDistanceNode refine", testing::IsEqual(outputVec1, outputVec2));
}

template <typename ElementType>
static void TestLinearPredictorNodeRefine()
{
    // make a linear predictor
    size_t dim = 3;
    predictors::LinearPredictor<ElementType> predictor(dim);
    predictor.GetBias() = 2.0;
    predictor.GetWeights() = math::ColumnVector<ElementType>{ 3.0, 4.0, 5.0 };

    // make a model
    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<ElementType>>(3);
    auto linearPredictorNode = model.AddNode<nodes::LinearPredictorNode<ElementType>>(inputNode->output, predictor);

    // refine the model
    model::TransformContext context;
    model::ModelTransformer transformer;
    auto newModel = transformer.RefineModel(model, context);

    // check for equality
    auto newInputNode = transformer.GetCorrespondingInputNode(inputNode);
    auto newOutputElements = transformer.GetCorrespondingOutputs(model::PortElements<ElementType>{ linearPredictorNode->output });
    inputNode->SetInput({ 1.0, 1.0, 1.0 });
    newInputNode->SetInput({ 1.0, 1.0, 1.0 });
    auto modelOutputValue = model.ComputeOutput(linearPredictorNode->output)[0];
    auto newOutputValue = newModel.ComputeOutput(newOutputElements)[0];

    testing::ProcessTest("Testing LinearPredictorNode refine", testing::IsEqual(modelOutputValue, newOutputValue));
}

static void TestDemultiplexerNodeRefine()
{
    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(1);
    auto selectorNode = model.AddNode<model::InputNode<bool>>(1);
    auto muxNode = model.AddNode<nodes::DemultiplexerNode<double, bool>>(inputNode->output, selectorNode->output, 2);

    // refine the model
    model::TransformContext context;
    model::ModelTransformer transformer;
    auto refinedModel = transformer.RefineModel(model, context);
    std::cout << "Original DemultiplexerNode nodes: " << model.Size() << ", refined: " << refinedModel.Size() << std::endl;
    auto newInputNode = transformer.GetCorrespondingInputNode(inputNode);
    auto newSelectorNode = transformer.GetCorrespondingInputNode(selectorNode);
    auto newMuxNodeElements = transformer.GetCorrespondingOutputs(muxNode->output);

    std::vector<double> inputValue{ 5.0 };
    inputNode->SetInput(inputValue);
    newInputNode->SetInput(inputValue);

    selectorNode->SetInput(false);
    newSelectorNode->SetInput(false); // output[0] should get the input
    auto outputVec = model.ComputeOutput(muxNode->output);
    auto newOutputVec = refinedModel.ComputeOutput(newMuxNodeElements);
    testing::ProcessTest("Testing DemultiplexerNode refine", testing::IsEqual(outputVec, newOutputVec));

    selectorNode->SetInput(true);
    newSelectorNode->SetInput(true); // output[1] should get the input
    outputVec = model.ComputeOutput(muxNode->output);
    newOutputVec = refinedModel.ComputeOutput(newMuxNodeElements);
    testing::ProcessTest("Testing DemultiplexerNode refine", testing::IsEqual(outputVec, newOutputVec));
}

static void TestMatrixVectorProductRefine()
{
    math::ColumnMatrix<double> w(2, 3);
    w(0, 0) = 1.0;
    w(0, 1) = 0.2;
    w(0, 2) = 0.3;
    w(1, 0) = 0.3;
    w(1, 1) = 0.7;
    w(1, 2) = 0.5;

    std::vector<double> input = { 1, 2, 3 };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(input.size());

    inputNode->SetInput(input);

    auto matrixVectorProductNode = model.AddNode<nodes::MatrixVectorProductNode<double, math::MatrixLayout::columnMajor>>(inputNode->output, w);

    model::TransformContext context;
    model::ModelTransformer transformer;
    auto refinedModel = transformer.RefineModel(model, context);
    auto refinedInputNode = transformer.GetCorrespondingInputNode(inputNode);
    auto refinedOutputElements = transformer.GetCorrespondingOutputs(model::PortElements<double>{ matrixVectorProductNode->output });

    refinedInputNode->SetInput(input);

    auto refinedOutput = refinedModel.ComputeOutput(refinedOutputElements);
    auto computeOutput = model.ComputeOutput(matrixVectorProductNode->output);

    testing::ProcessTest("Testing matrix vector product node refine", testing::IsEqual(refinedOutput, computeOutput));
}

static void TestEuclideanDistanceNodeRefine()
{
    math::RowMatrix<double> v(2, 3);
    v(0, 0) = 1.0;
    v(0, 1) = 0.2;
    v(0, 2) = 0.3;
    v(1, 0) = 0.3;
    v(1, 1) = 0.7;
    v(1, 2) = 0.5;

    std::vector<double> input = { 1, 2, 3 };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(input.size());

    inputNode->SetInput(input);

    auto euclideanDistanceNode = model.AddNode<nodes::SquaredEuclideanDistanceNode<double, math::MatrixLayout::columnMajor>>(inputNode->output, v);

    model::TransformContext context;
    model::ModelTransformer transformer;
    auto refinedModel = transformer.RefineModel(model, context);
    auto refinedInputNode = transformer.GetCorrespondingInputNode(inputNode);
    auto refinedOutputElements = transformer.GetCorrespondingOutputs(model::PortElements<double>{ euclideanDistanceNode->output });

    refinedInputNode->SetInput(input);

    auto refinedOutput = refinedModel.ComputeOutput(refinedOutputElements);
    auto computeOutput = model.ComputeOutput(euclideanDistanceNode->output);

    testing::ProcessTest("Testing Euclidean distance node refine", testing::IsEqual(refinedOutput, computeOutput));
}

static void TestProtoNNPredictorNode()
{
    size_t dim = 5, projectedDim = 4, numPrototypes = 3, numLabels = 2;
    double gamma = 0.3;
    predictors::ProtoNNPredictor protonnPredictor(dim, projectedDim, numPrototypes, numLabels, gamma);

    // projectedDim * dim
    auto W = protonnPredictor.GetProjectionMatrix().GetReference();
    W(0, 0) = 0.4;
    W(0, 1) = 0.5;
    W(0, 2) = 0.1;
    W(0, 3) = 0.1;
    W(0, 4) = 0.1;
    W(1, 0) = 0.1;
    W(1, 1) = 0.4;
    W(1, 2) = 0.8;
    W(1, 3) = 0.2;
    W(1, 4) = 0.5;
    W(2, 0) = 0.2;
    W(2, 1) = 0.1;
    W(2, 2) = 0.7;
    W(2, 3) = 0.3;
    W(2, 4) = 0.4;
    W(3, 0) = 0.3;
    W(3, 1) = 0.3;
    W(3, 2) = 0.2;
    W(3, 3) = 0.5;
    W(3, 4) = 0.2;

    // projectedDim * numPrototypes
    auto B = protonnPredictor.GetPrototypes().GetReference();
    B(0, 0) = 0.1;
    B(0, 1) = 0.2;
    B(0, 2) = 0.3;
    B(1, 0) = 0.8;
    B(1, 1) = 0.7;
    B(1, 2) = 0.6;
    B(2, 0) = 0.4;
    B(2, 1) = 0.6;
    B(2, 2) = 0.2;
    B(3, 0) = 0.2;
    B(3, 1) = 0.1;
    B(3, 2) = 0.3;

    // numLabels * numPrototypes
    auto Z = protonnPredictor.GetLabelEmbeddings().GetReference();
    Z(0, 0) = 0.1;
    Z(0, 1) = 0.3;
    Z(0, 2) = 0.2;
    Z(1, 0) = 0.2;
    Z(1, 1) = 0.4;
    Z(1, 2) = 0.8;

    std::vector<double> input = { 0.2, 0.5, 0.6, 0.8, 0.1 };

    model::Model model;
    auto inputNode = model.AddNode<model::InputNode<double>>(input.size());

    inputNode->SetInput(input);

    auto protonnPredictorNode = model.AddNode<nodes::ProtoNNPredictorNode>(inputNode->output, protonnPredictor);

    model::TransformContext context;
    model::ModelTransformer transformer;
    auto refinedModel = transformer.RefineModel(model, context);
    auto refinedInputNode = transformer.GetCorrespondingInputNode(inputNode);
    auto refinedScoreOutputElements = transformer.GetCorrespondingOutputs(model::PortElements<double>{ protonnPredictorNode->output });

    refinedInputNode->SetInput(input);

    auto refinedScoresOutput = refinedModel.ComputeOutput(refinedScoreOutputElements);

    auto computeScoreOutput = model.ComputeOutput(protonnPredictorNode->output);

    testing::ProcessTest("Testing protonnPredictor node refine", testing::IsEqual(refinedScoresOutput, computeScoreOutput));
}

static void TestClockNodeCompute()
{
    constexpr nodes::TimeTickType lagThreshold = 165;
    constexpr nodes::TimeTickType interval = 50;
    constexpr nodes::TimeTickType start = 1511889201834.5767; // timestamp from python: time.time() * 1000

    model::Model model;

    int lagNotificationCallbackCount = 0;
    auto inputNode = model.AddNode<model::InputNode<nodes::TimeTickType>>(1);
    auto clockNode = model.AddNode<nodes::ClockNode>(inputNode->output, interval, lagThreshold, "LagNotificationCallback", [&lagNotificationCallbackCount](nodes::TimeTickType timeLag) {
        std::cout << "LagNotificationCallback: " << timeLag << "\n";
        lagNotificationCallbackCount++;
    });

    std::vector<std::vector<nodes::TimeTickType>> signal =
        {
          { start },
          { start + interval * 1 + lagThreshold / 2 }, // within threshold
          { start + interval * 2 }, // on time
          { start + interval * 3 + lagThreshold }, // late (expect notification)
          { start + interval * 4 + lagThreshold * 20 }, // really late (expect notification)
          { start + interval * 5 } // on time
        };

    std::vector<std::vector<nodes::TimeTickType>> expectedResults =
        {
          // lastIntervalTime, currentTime
          { start, start },
          { start + interval * 1, start + interval * 1 + lagThreshold / 2 },
          { start + interval * 2, start + interval * 2 },
          { start + interval * 3, start + interval * 3 + lagThreshold },
          { start + interval * 4, start + interval * 4 + lagThreshold * 20 },
          { start + interval * 5, start + interval * 5 }
        };

    std::vector<nodes::TimeTickType> expectedGetTicksResults =
        {
          interval,
          interval - lagThreshold / 2,
          interval,
          interval - lagThreshold,
          interval - lagThreshold * 20,
          interval
        };

    std::vector<std::vector<nodes::TimeTickType>> results;
    std::vector<nodes::TimeTickType> getTicksResults;
    for (const auto& input : signal)
    {
        inputNode->SetInput(input);
        results.push_back(model.ComputeOutput(clockNode->output));
        getTicksResults.push_back(clockNode->GetTicksUntilNextInterval(input[0]));
    }

    testing::ProcessTest("Testing ClockNode compute", testing::IsEqual(results, expectedResults));
    testing::ProcessTest("Testing ClockNode GetTicksUntilNextInterval", testing::IsEqual(getTicksResults, expectedGetTicksResults));
    testing::ProcessTest("Testing lag notification count", testing::IsEqual(lagNotificationCallbackCount, 2));
}

static void TestConcatenationNodeCompute()
{
    model::Model model;

    std::vector<double> data = { 1, 2, 3, 4, 5 };
    auto inputNode = model.AddNode<model::InputNode<double>>(data.size());
    auto constantNode = model.AddNode<nodes::ConstantNode<double>>(std::vector<double>{ 6, 7, 8 });
    auto concatenationInputs = model::PortElements<double>({ inputNode->output, constantNode->output });
    auto outputNode = model.AddNode<nodes::ConcatenationNode<double>>(concatenationInputs, math::TensorShape( 1, 1, 8 ));

    auto map = model::Map(model, { { "input", inputNode } }, { { "output", outputNode->output } });

    std::vector<double> expected{ 1, 2, 3, 4, 5, 6, 7, 8 };
    auto result = map.Compute<double>(data);

    testing::ProcessTest("TestConcatenationNodeCompute", testing::IsEqual(result, expected));
}

//
// Main driver function to call all the tests
//
void TestNodes()
{
    //
    // Compute tests
    //
    TestAccumulatorNodeCompute();
    TestBinaryOperationNodeCompute();
    TestBinaryOperationNodeCompute2();
    TestClockNodeCompute();
    TestConcatenationNodeCompute();
    TestDemultiplexerNodeCompute();
    TestL2NormSquaredNodeCompute();
    TestLinearPredictorNodeCompute<double>();
    TestLinearPredictorNodeCompute<float>();
    TestMovingAverageNodeCompute();
    TestMovingVarianceNodeCompute();
    TestSinkNodeCompute();
    TestSourceNodeCompute();
    TestSquaredEuclideanDistanceNodeCompute();
    TestUnaryOperationNodeCompute();

    //
    // Refine tests
    //
    TestL2NormSquaredNodeRefine();
    TestLinearPredictorNodeRefine<double>();
    TestLinearPredictorNodeRefine<float>();
    TestMovingAverageNodeRefine();
    TestSimpleForestPredictorNodeRefine();
    TestDemultiplexerNodeRefine();
    TestMatrixVectorProductRefine();
    TestEuclideanDistanceNodeRefine();
    TestProtoNNPredictorNode();
    TestSquaredEuclideanDistanceNodeRefine();
}
