////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     DTWDistanceNode.h (nodes)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "SumNode.h"

// model
#include "BinaryOperationNode.h"
#include "CompilableNode.h"
#include "IRMapCompiler.h"
#include "InputPort.h"
#include "MapCompiler.h"
#include "ModelTransformer.h"
#include "Node.h"
#include "OutputPort.h"
#include "PortElements.h"

// utilities
#include "Exception.h"
#include "TypeName.h"

// stl
#include <string>

namespace ell
{
namespace nodes
{
    /// <summary> A node that computes the dynamic time-warping distance between its inputs </summary>
    template <typename ValueType>
    class DTWDistanceNode : public model::CompilableNode
    {
    public:
        /// @name Input and Output Ports
        /// @{
        const model::InputPort<ValueType>& input = _input;
        const model::OutputPort<ValueType>& output = _output;
        /// @}

        /// <summary> Default Constructor </summary>
        DTWDistanceNode();

        /// <summary> Constructor </summary>
        ///
        /// <param name="input"> The signals to compare to the prototype </param>
        /// <param name="prototype"> The prototype </param>
        DTWDistanceNode(const model::PortElements<ValueType>& input, const std::vector<std::vector<ValueType>>& prototype);

        /// <summary> Gets the name of this type (for serialization). </summary>
        ///
        /// <returns> The name of this type. </returns>
        static std::string GetTypeName() { return utilities::GetCompositeTypeName<ValueType>("DTWDistanceNode"); }

        /// <summary> Gets the name of this type (for serialization). </summary>
        ///
        /// <returns> The name of this type. </returns>
        std::string GetRuntimeTypeName() const override { return GetTypeName(); }

        /// <summary> Makes a copy of this node in the model being constructed by the transformer </summary>
        ///
        /// <param name="transformer"> The `ModelTransformer` currently copying the model </param>
        void Copy(model::ModelTransformer& transformer) const override;

        /// <summary></summary>
        std::vector<std::vector<ValueType>> GetPrototype() const { return _prototype; }

        /// <summary> Reset the state of the node </summary>
        void Reset() override;

    protected:
        void Compute() const override;
        void Compile(model::IRMapCompiler& compiler, emitters::IRFunctionEmitter& function) override;
        bool HasState() const override { return true; }
        void WriteToArchive(utilities::Archiver& archiver) const override;
        void ReadFromArchive(utilities::Unarchiver& archiver) override;

    private:
        std::vector<ValueType> GetPrototypeData() const;

        model::InputPort<ValueType> _input;
        model::OutputPort<ValueType> _output;

        size_t _sampleDimension;
        size_t _prototypeLength;
        std::vector<std::vector<ValueType>> _prototype;
        // double _threshold;
        double _prototypeVariance;

        mutable std::vector<ValueType> _d;
        mutable std::vector<int> _s;
        mutable int _currentTime;
    };
}
}

#include "../tcc/DTWDistanceNode.tcc"