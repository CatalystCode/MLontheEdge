////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     Map.h (model)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "InputNode.h"
#include "Node.h"
#include "OutputNode.h"
#include "PortElements.h"

// data
#include "DataVector.h"

// math
#include "Tensor.h"

// utilities
#include "Exception.h"
#include "IArchivable.h"
#include "PropertyBag.h"
#include "StlIndexValueIterator.h"
#include "TypeTraits.h"
#include "TypeName.h"

// stl
#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ell
{
namespace model
{
    class ModelOptimizer;
    class ModelOptimizerContext;
    class ModelTransformer;
    class OutputNodeBase;

    /// <summary> Class that wraps a model and its designated outputs </summary>
    class Map : public utilities::IArchivable
    {
    public:
        Map() = default;

        /// <summary> Copy constructor </summary>
        ///
        /// <param name="other"> The other map. </param>
        Map(const Map& other);

        Map(Map&& other) = default;

        /// <summary> Assignment operator. </summary>
        ///
        /// <param name="other"> The other map. </param>
        /// <returns> A reference to this map. </returns>
        Map& operator=(Map other);

        /// <summary> Constructor </summary>
        ///
        /// <param name="model"> The model to wrap </param>
        /// <param name="inputs"> A vector of name/value pairs for the inputs this map uses </param>
        /// <param name="outputs"> A vector of name/value pairs for the outputs this map generates </param>
        Map(const Model& model, const std::vector<std::pair<std::string, InputNodeBase*>>& inputs, const std::vector<std::pair<std::string, PortElementsBase>>& outputs);

        ~Map() override = default;

        /// <summary> Gets the model wrapped by this map </summary>
        ///
        /// <returns> The `Model` </returns>
        const Model& GetModel() const { return _model; }

        /// <summary> Gets the model wrapped by this map </summary>
        ///
        /// <returns> The `Model` </returns>
        Model& GetModel() { return _model; }

        /// <summary> Computes the map's output from input values </summary>
        ///
        /// <param name="inputValues"> The input to the map </param>
        /// <returns> A vector of output values </returns>
        template <typename OutputType, typename InputType, utilities::IsFundamental<OutputType> OutputConcept = 1, utilities::IsFundamental<InputType> InputConcept = 1>
        std::vector<OutputType> Compute(const std::vector<InputType>& inputValues) const;

        /// <summary> Computes the map's output from input values </summary>
        ///
        /// <param name="inputValues"> The input to the map </param>
        /// <returns> A vector of output values </returns>
        template <typename OutputVectorType, typename InputVectorType, data::IsDataVector<OutputVectorType> OutputConcept = true, data::IsDataVector<InputVectorType> InputConcept = true>
        OutputVectorType Compute(const InputVectorType& inputValues) const;

        /// <summary> Reset the state of the model </summary>
        void Reset();

        /// <summary> Returns the size of the map's input </summary>
        ///
        /// <returns> The dimensionality of the map's input port </returns>
        size_t GetInputSize() const;

        /// <summary> Returns the size of the map's output </summary>
        ///
        /// <returns> The dimensionality of the map's output port </returns>
        size_t GetOutputSize() const;

        /// <summary> Returns the shape of the map's input </summary>
        ///
        /// <returns> The dimensionality of the map's input </returns>
        math::TensorShape GetInputShape() const;

        /// <summary> Returns the shape of the map's output </summary>
        ///
        /// <returns> The dimensionality of the map's output port </returns>
        math::TensorShape GetOutputShape() const;

        /// <summary> Returns the type of the map's input </summary>
        ///
        /// <reutrns> The type of the map's input </summary>
        Port::PortType GetInputType() const;

        /// <summary> Returns the type of the map's output </summary>
        ///
        /// <reutrns> The type of the map's output </summary>
        Port::PortType GetOutputType() const;

        /// <summary> Returns the map's input node </summary>
        ///
        /// <returns> The input node </returns>
        InputNodeBase* GetInput() const { return GetInput(0); }

        /// <summary> Returns the map's output PortElementsBase </summary>
        ///
        /// <returns> The output </returns>
        PortElementsBase GetOutput() const { return GetOutput(0); }

        /// <summary> Refines the model wrapped by this map. </summary>
        ///
        /// <param name="maxIterations"> The maximum number of refinement iterations. </param>
        void Refine(int maxIterations = 10);

        /// <summary> Refines the model wrapped by this map. </summary>
        ///
        /// <param name="context"> The TransformContext to use during refinement. </param>
        /// <param name="maxIterations"> The maximum number of refinement iterations. </param>
        void Refine(const TransformContext& context, int maxIterations = 10);

        /// <summary> Optimizes the model wrapped by this map. </summary>
        ///
        /// <param name="optimizer"> The optimizer to use for optimizing the model. </param>
        void Optimize(const ModelOptimizer& optimizer);

        /// <summary> Transforms the model wrapped by this map by applying a transformation function to each node </summary>
        ///
        /// <param name="transformFunction"> The function to apply on each node </param>
        /// <param name="context"> The TransformContext to use during the transformation </param>
        void Transform(const std::function<void(const Node&, ModelTransformer&)>& transformFunction, const TransformContext& context);

        /// <summary> Renames the model callbacks in this map. </summary>
        ///
        /// <param name="sourceCallbackName"> The new source callback name. </param>
        /// <param name="sinkCallbackName"> The new sink callback name. </param>
        void RenameCallbacks(const std::string& sourceCallbackName, const std::string& sinkCallbackName);

        //
        // ELL-Internal routines for getting information about inputs / outputs of the map
        // and doing type-safe operations.
        //

        /// <returns> The number of input nodes </returns>
        size_t NumInputPorts() const { return _inputNodes.size(); }

        /// <summary> Returns an input node </summary>
        ///
        /// <param name="index"> The index of the input </param>
        /// <returns> The input node </returns>
        InputNodeBase* GetInput(size_t index) const;

        /// <summary> Returns an input node </summary>
        ///
        /// <param name="index"> The name of the input </param>
        /// <returns> The input node </returns>
        InputNodeBase* GetInput(const std::string& inputName) const;

        /// <summary> Returns the input nodes </summary>
        ///
        /// <returns> The input nodes </returns>
        const std::vector<InputNodeBase*>& GetInputs() { return _inputNodes; }

        /// <summary> Returns the input nodes </summary>
        ///
        /// <returns> The input nodes </returns>
        std::vector<const InputNodeBase*> GetInputNodes() const;

        /// <summary> Returns the output nodes </summary>
        ///
        /// <returns> The output nodes </returns>
        std::vector<const OutputNodeBase*> GetOutputNodes() const;

        /// <summary> Get the number of outputs </summary>
        ///
        /// <returns> The number of outputs </returns>
        size_t NumOutputPorts() const { return _outputElements.size(); }

        /// <summary> Returns an output </summary>
        ///
        /// <param name="index"> The index of the output </param>
        /// <returns> The output </returns>
        PortElementsBase GetOutput(size_t index) const;

        /// <summary> Returns an outputs </summary>
        ///
        /// <param name="outputName"> The name of the output </param>
        /// <returns> The output </returns>
        PortElementsBase GetOutput(const std::string& outputName) const;

        /// <summary> Returns the outputs </summary>
        ///
        /// <returns> The outputs </returns>
        const std::vector<PortElementsBase>& GetOutputs() const { return _outputElements; }

        /// <summary> Resets a specified output. </summary>
        ///
        /// <param name="index"> Output index. </param>
        /// <param name="outputElements"> The output elements. </param>
        void ResetOutput(size_t index, PortElementsBase outputElements);

        //
        // Routines for computing output (processing data)
        //

        /// <summary> Set a single InputNode's input </summary>
        ///
        /// <typeparam name="ValueType"> The datatype of the input node </typeparam>
        /// <param name="index"> index of the input node </param>
        /// <param name="inputValues"> The values to set on the input node </param>
        template <typename ValueType>
        void SetInputValue(int index, const std::vector<ValueType>& inputValues) const;

        /// <summary> Set a single InputNode's input </summary>
        ///
        /// <typeparam name="ValueType"> The datatype of the input node </typeparam>
        /// <param name="inputName"> The name assigned to the input node </param>
        /// <param name="inputValues"> The values to set on the input node </param>
        template <typename ValueType>
        void SetInputValue(const std::string& inputName, const std::vector<ValueType>& inputValues) const;

        /// <summary> Set a single InputNode's input </summary>
        ///
        /// <typeparam name="ValueType"> The datatype of the input node </typeparam>
        /// <param name="index"> The index of the input node </param>
        /// <param name="inputValues"> The values to set on the input node </param>
        template <typename DataVectorType, data::IsDataVector<DataVectorType> Concept = true>
        void SetInputValue(int index, const DataVectorType& inputValues) const;

        /// <summary> Set a single InputNode's input </summary>
        ///
        /// <typeparam name="ValueType"> The datatype of the input node </typeparam>
        /// <param name="index"> The index of the input node </param>
        /// <param name="inputValues"> The values to set on the input node </param>
        template <typename DataVectorType, data::IsDataVector<DataVectorType> Concept = true>
        void SetInputValue(const std::string& inputName, const DataVectorType& inputValues) const;

        /// <summary> Computes of one of the map's outputs from its current input values </summary>
        ///
        /// <param name="index"> The index of the output </param>
        /// <returns> A vector of output values </returns>
        template <typename ValueType, utilities::IsFundamental<ValueType> = 0>
        std::vector<ValueType> ComputeOutput(int index) const;

        /// <summary> Computes of one of the map's outputs from its current input values </summary>
        ///
        /// <param name="index"> The index of the output </param>
        /// <returns> A vector of output values </returns>
        template <typename DataVectorType, data::IsDataVector<DataVectorType> Concept = true>
        DataVectorType ComputeOutput(int index) const;

        /// <summary> Computes of one of the map's outputs from its current input values </summary>
        ///
        /// <param name="outputName"> The name of the output </param>
        /// <returns> A vector of output values </returns>
        template <typename ValueType, utilities::IsFundamental<ValueType> = 0>
        std::vector<ValueType> ComputeOutput(const std::string& outputName) const;

        /// <summary> Computes of one of the map's outputs from its current input values </summary>
        ///
        /// <param name="outputName"> The name of the output </param>
        /// <returns> A vector of output values </returns>
        template <typename DataVectorType, data::IsDataVector<DataVectorType> Concept = true>
        DataVectorType ComputeOutput(const std::string& outputName) const;

        /// <summary> Returns a `PortElements` object representing the indicated map output </summary>
        ///
        /// <param name="outputIndex"> The zero-based index of the map output </param>
        /// <returns> The `PortElements` object representing the indicated outputs </returns>
        template <typename ValueType>
        PortElements<ValueType> GetOutputElements(size_t outputIndex) const;

        /// <summary> Returns a `PortElements` object representing the indicated map output </summary>
        ///
        /// <param name="outputName"> The name of the map output </param>
        /// <returns> The `PortElements` object representing the indicated outputs </returns>
        template <typename ValueType>
        PortElements<ValueType> GetOutputElements(std::string outputName) const;

        /// <summary> Gets the name of this type (for serialization). </summary>
        ///
        /// <returns> The name of this type. </returns>
        static std::string GetTypeName() { return "Map"; }

        /// <summary> Gets the name of this type (for serialization). </summary>
        ///
        /// <returns> The name of this type. </returns>
        std::string GetRuntimeTypeName() const override { return GetTypeName(); }

        /// <summary> Get this object's metadata object. </summary>
        ///
        /// <returns> A reference to the PropertyBag containing the metadata for this object. </returns>
        utilities::PropertyBag& GetMetadata() { return _metadata; }

        /// <summary> Get this object's metadata object. </summary>
        ///
        /// <returns> A const reference to the PropertyBag containing the metadata for this object. </returns>
        const utilities::PropertyBag& GetMetadata() const { return _metadata; }

        /// <summary> Swaps the contents of two maps. </summary>
        ///
        /// <param name="a"> One of the maps to swap. </param>
        /// <param name="b"> The other map to swap. </param>
        friend void swap(Map& a, Map& b);

        /// <summary>Prune away unused parts of internal model. </summary>
        void Prune();

        /// <summary> Adds the given input node to the map. </summary>
        ///
        /// <param name="inputName"> Name of the input. </param>
        /// <param name="inputNode"> The input node. </param>
        void AddInput(const std::string& inputName, InputNodeBase* inputNode);

        /// <summary> Adds the given output node to the map. </summary>
        ///
        /// <param name="outputName"> Name of the output. </param>
        /// <param name="inputNode"> The output elements. </param>
        void AddOutput(const std::string& outputName, PortElementsBase outputElements);

    protected:
        template <typename DataVectorType, typename ElementsType, data::IsDataVector<DataVectorType> Concept = true>
        void SetInputValue(InputNodeBase* node, const DataVectorType& inputValues) const;

        template <typename DataVectorType, data::IsDataVector<DataVectorType> Concept = true>
        void SetInputValue(InputNodeBase* node, const DataVectorType& inputValues) const;

        template <typename DataVectorType, typename ElementsType, data::IsDataVector<DataVectorType> Concept = true>
        DataVectorType ComputeOutput(const PortElementsBase& elements) const;

        template <typename ValueType, utilities::IsFundamental<ValueType> = 1>
        std::vector<ValueType> ComputeOutput(const PortElementsBase& elements) const;

        template <typename DataVectorType, data::IsDataVector<DataVectorType> Concept = true>
        DataVectorType ComputeOutput(const PortElementsBase& elements) const;

        utilities::ArchiveVersion GetArchiveVersion() const override;
        bool CanReadArchiveVersion(const utilities::ArchiveVersion& version) const override;
        void WriteToArchive(utilities::Archiver& archiver) const override;
        void ReadFromArchive(utilities::Unarchiver& archiver) override;

        virtual void SetNodeInput(InputNode<bool>* node, const std::vector<bool>& inputValues) const;
        virtual void SetNodeInput(InputNode<int>* node, const std::vector<int>& inputValues) const;
        virtual void SetNodeInput(InputNode<int64_t>* node, const std::vector<int64_t>& inputValues) const;
        virtual void SetNodeInput(InputNode<float>* node, const std::vector<float>& inputValues) const;
        virtual void SetNodeInput(InputNode<double>* node, const std::vector<double>& inputValues) const;

        virtual std::vector<bool> ComputeBoolOutput(const PortElementsBase& outputs) const;
        virtual std::vector<int> ComputeIntOutput(const PortElementsBase& outputs) const;
        virtual std::vector<int64_t> ComputeInt64Output(const PortElementsBase& outputs) const;
        virtual std::vector<float> ComputeFloatOutput(const PortElementsBase& outputs) const;
        virtual std::vector<double> ComputeDoubleOutput(const PortElementsBase& outputs) const;

    private:
        Model _model;

        std::vector<InputNodeBase*> _inputNodes;
        std::vector<std::string> _inputNames;
        std::unordered_map<std::string, InputNodeBase*> _inputNodeMap;

        std::vector<PortElementsBase> _outputElements;
        std::vector<std::string> _outputNames;
        std::unordered_map<std::string, PortElementsBase> _outputElementsMap;
        utilities::PropertyBag _metadata;

        std::vector<const Node*> GetAllOutputNodes() const;
        std::vector<const Node*> GetDebugSinkNodes() const;
        void FixTransformedIO(ModelTransformer& transformer);
        void FixTransformedIO(ModelOptimizerContext& context);
    };

    /// <summary> A serialization context used during Map deserialization. Wraps an existing `ModelSerializationContext` </summary>
    class MapSerializationContext : public ModelSerializationContext
    {
    public:
        /// <summary> Constructor </summary>
        ///
        /// <param name="previousContext"> The `SerializationContext` to wrap </param>
        MapSerializationContext(utilities::SerializationContext& previousContext);
    };
}
}

#include "../tcc/Map.tcc"
