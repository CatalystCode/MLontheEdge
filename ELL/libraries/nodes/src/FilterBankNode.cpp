////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     FilterBankNode.cpp (nodes)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "FilterBankNode.h"

// emitters
#include "EmitterException.h"
#include "EmitterTypes.h"
#include "IRLocalValue.h"

namespace ell
{
namespace nodes
{
    //
    // FilterBankNode
    //
    template <typename ValueType>
    FilterBankNode<ValueType>::FilterBankNode(const dsp::TriangleFilterBank& filters)
        : CompilableNode({ &_input }, { &_output }), _input(this, {}, defaultInputPortName), _output(this, defaultOutputPortName, 0), _filters(filters)
    {
    }

    template <typename ValueType>
    FilterBankNode<ValueType>::FilterBankNode(const model::PortElements<ValueType>& input, const dsp::TriangleFilterBank& filters)
        : CompilableNode({ &_input }, { &_output }), _input(this, input, defaultInputPortName), _output(this, defaultOutputPortName, 0), _filters(filters)
    {
    }

    template <typename ValueType>
    void FilterBankNode<ValueType>::Compute() const
    {
        std::vector<ValueType> input = _input.GetValue();
        auto output = _filters.FilterFrequencyMagnitudes(input);
        _output.SetOutput(output);
    };

    template <typename ValueType>
    void FilterBankNode<ValueType>::Compile(model::IRMapCompiler& compiler, emitters::IRFunctionEmitter& function)
    {
        using namespace std::string_literals;

        auto& module = function.GetModule();
        auto numFilters = output.Size();

        // Write out global variables with bin info
        std::vector<int> startBins;
        std::vector<int> centerBins;
        std::vector<int> endBins;
        for (size_t filterIndex = _filters.GetBeginFilter(); filterIndex < _filters.GetEndFilter(); ++filterIndex)
        {
            auto f = _filters.GetFilter(filterIndex);
            startBins.push_back(f.GetStart());
            centerBins.push_back(f.GetCenter());
            endBins.push_back(f.GetEnd());
        }
        auto beginVar = module.ConstantArray("filterStart_"s + GetInternalStateIdentifier(), startBins);
        auto centerVar = module.ConstantArray("filterCenter_"s + GetInternalStateIdentifier(), centerBins);
        auto endVar = module.ConstantArray("filterEnd_"s + GetInternalStateIdentifier(), endBins);

        auto half = function.LocalScalar<ValueType>(0.5);
        // Get port variables
        llvm::Value* pInput = compiler.EnsurePortEmitted(input);
        llvm::Value* pOutput = compiler.EnsurePortEmitted(output);

        function.For(numFilters, [pInput, pOutput, half, beginVar, centerVar, endVar](emitters::IRFunctionEmitter& function, llvm::Value* filterIndex) {
            auto sum = function.Variable(emitters::GetVariableType<ValueType>());
            auto begin = function.LocalScalar(function.ValueAt(beginVar, filterIndex));
            auto center = function.LocalScalar(function.ValueAt(centerVar, filterIndex));
            auto end = function.LocalScalar(function.ValueAt(endVar, filterIndex));
            function.StoreZero(sum);

            // for index in [begin, center)
            function.For(begin, center, [pInput, half, sum, begin, center](emitters::IRFunctionEmitter& function, llvm::Value* indexValue) {
                // sum += signal[i] * ((i-begin+0.5) / (center-begin))
                auto index = function.LocalScalar(indexValue);
                auto inputVal = function.LocalScalar(function.ValueAt(pInput, index));
                auto numer = index - begin;
                auto denom = center - begin;
                auto val = inputVal * ((function.LocalScalar(function.CastValue<int, ValueType>(numer)) + half) / function.LocalScalar(function.CastValue<int, ValueType>(denom)));
                function.Store(sum, function.LocalScalar(function.Load(sum)) + val);
            });

            // for index in [center, end)
            function.For(center, end, [pInput, half, sum, center, end](emitters::IRFunctionEmitter& function, llvm::Value* indexValue) {
                // sum += signal[i] * ((end-i-0.5) / (end-center))
                auto index = function.LocalScalar(indexValue);
                auto inputVal = function.LocalScalar(function.ValueAt(pInput, index));
                auto numer = end - index;
                auto denom = end - center;
                auto val = inputVal * ((function.LocalScalar(function.CastValue<int, ValueType>(numer)) - half) / function.LocalScalar(function.CastValue<int, ValueType>(denom)));
                function.Store(sum, function.LocalScalar(function.Load(sum)) + val);
            });

            function.SetValueAt(pOutput, filterIndex, function.Load(sum));

        });
    }

    template <typename ValueType>
    void FilterBankNode<ValueType>::WriteToArchive(utilities::Archiver& archiver) const
    {
        model::CompilableNode::WriteToArchive(archiver);
        archiver[defaultInputPortName] << _input;
    }

    template <typename ValueType>
    void FilterBankNode<ValueType>::ReadFromArchive(utilities::Unarchiver& archiver)
    {
        model::CompilableNode::ReadFromArchive(archiver);
        archiver[defaultInputPortName] >> _input;
    }

    //
    // LinearFilterBankNode
    //
    template <typename ValueType>
    LinearFilterBankNode<ValueType>::LinearFilterBankNode()
        : FilterBankNode<ValueType>(_linearFilters)
    {
    }

    template <typename ValueType>
    LinearFilterBankNode<ValueType>::LinearFilterBankNode(const model::PortElements<ValueType>& input, const dsp::LinearFilterBank& filters)
        : FilterBankNode<ValueType>(input, _linearFilters), _linearFilters(filters)
    {
        _output.SetSize(_linearFilters.NumActiveFilters());
    }

    template <typename ValueType>
    void LinearFilterBankNode<ValueType>::Copy(model::ModelTransformer& transformer) const
    {
        auto newPortElements = transformer.TransformPortElements(this->_input.GetPortElements());
        auto newNode = transformer.AddNode<LinearFilterBankNode<ValueType>>(newPortElements, _linearFilters);
        transformer.MapNodeOutput(output, newNode->output);
    }

    template <typename ValueType>
    void LinearFilterBankNode<ValueType>::WriteToArchive(utilities::Archiver& archiver) const
    {
        nodes::FilterBankNode<ValueType>::WriteToArchive(archiver);
        archiver["filters"] << _linearFilters;
    }

    template <typename ValueType>
    void LinearFilterBankNode<ValueType>::ReadFromArchive(utilities::Unarchiver& archiver)
    {
        nodes::FilterBankNode<ValueType>::ReadFromArchive(archiver);
        archiver["filters"] >> _linearFilters;
        _output.SetSize(_linearFilters.NumActiveFilters());
    }

    //
    // MelFilterBankNode
    //
    template <typename ValueType>
    MelFilterBankNode<ValueType>::MelFilterBankNode()
        : FilterBankNode<ValueType>(_melFilters)
    {
    }

    template <typename ValueType>
    MelFilterBankNode<ValueType>::MelFilterBankNode(const model::PortElements<ValueType>& input, const dsp::MelFilterBank& filters)
        : FilterBankNode<ValueType>(input, _melFilters), _melFilters(filters)
    {
        _output.SetSize(_melFilters.NumActiveFilters());
    }

    template <typename ValueType>
    void MelFilterBankNode<ValueType>::Copy(model::ModelTransformer& transformer) const
    {
        auto newPortElements = transformer.TransformPortElements(_input.GetPortElements());
        auto newNode = transformer.AddNode<MelFilterBankNode<ValueType>>(newPortElements, _melFilters);
        transformer.MapNodeOutput(output, newNode->output);
    }

    template <typename ValueType>
    void MelFilterBankNode<ValueType>::WriteToArchive(utilities::Archiver& archiver) const
    {
        nodes::FilterBankNode<ValueType>::WriteToArchive(archiver);
        archiver["filters"] << _melFilters;
    }

    template <typename ValueType>
    void MelFilterBankNode<ValueType>::ReadFromArchive(utilities::Unarchiver& archiver)
    {
        nodes::FilterBankNode<ValueType>::ReadFromArchive(archiver);
        archiver["filters"] >> _melFilters;
        _output.SetSize(_melFilters.NumActiveFilters());
    }

    // Explicit instantiations
    template class FilterBankNode<float>;
    template class FilterBankNode<double>;
    template class LinearFilterBankNode<float>;
    template class LinearFilterBankNode<double>;
    template class MelFilterBankNode<float>;
    template class MelFilterBankNode<double>;
} // nodes
} // ell
