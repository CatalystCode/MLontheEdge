////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     IRCompiledMap.cpp (model)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// model
#include "IRCompiledMap.h"
#include "CompilableNodeUtilities.h"
#include "EmitterException.h"
#include "IRMapCompiler.h"
#include "ModelTransformer.h"
#include "OutputNode.h"
#include "Port.h"

// emitters
#include "IROptimizer.h"

// utilities
#include "Exception.h"
#include "Files.h"

// llvm
#include <llvm/Transforms/Utils/Cloning.h>

// stl
#include <sstream>

namespace ell
{
namespace model
{
    IRCompiledMap::IRCompiledMap(IRCompiledMap&& other)
        : CompiledMap(std::move(other), other._functionName, other._compilerOptions), _moduleName(std::move(other._moduleName)), _module(std::move(other._module)), _executionEngine(std::move(other._executionEngine)), _verifyJittedModule(other._verifyJittedModule), _computeFunctionDefined(false)
    {
    }

    // private constructor:
    IRCompiledMap::IRCompiledMap(Map map, const std::string& functionName, const MapCompilerOptions& options, std::unique_ptr<emitters::IRModuleEmitter> module, bool verifyJittedModule)
        : CompiledMap(std::move(map), functionName, options), _module(std::move(module)), _verifyJittedModule(verifyJittedModule), _computeFunctionDefined(false)
    {
        _moduleName = _module->GetModuleName();
    }

    bool IRCompiledMap::IsValid() const
    {
        return _module != nullptr && _module->IsValid();
    }

    emitters::IRExecutionEngine& IRCompiledMap::GetJitter()
    {
        EnsureExecutionEngine();
        return *_executionEngine;
    }

    void IRCompiledMap::EnsureExecutionEngine() const
    {
        if (!_executionEngine)
        {
            auto moduleClone = std::unique_ptr<llvm::Module>(llvm::CloneModule(_module->GetLLVMModule()));
            _executionEngine = std::make_unique<emitters::IRExecutionEngine>(std::move(moduleClone), _verifyJittedModule);
        }
    }

    void IRCompiledMap::FinishJitting() const
    {
        EnsureExecutionEngine();
        SetComputeFunction();
    }

    void IRCompiledMap::SetComputeFunction() const
    {
        switch (GetInput(0)->GetOutputPort().GetType())
        {
        case model::Port::PortType::boolean:
            SetComputeFunctionForInputType<bool>();
            break;

        case model::Port::PortType::integer:
            SetComputeFunctionForInputType<int>();
            break;

        case model::Port::PortType::bigInt:
            SetComputeFunctionForInputType<int64_t>();
            break;

        case model::Port::PortType::smallReal:
            SetComputeFunctionForInputType<float>();
            break;

        case model::Port::PortType::real:
            SetComputeFunctionForInputType<double>();
            break;

        default:
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }
    }

    void IRCompiledMap::SetNodeInput(model::InputNode<bool>* node, const std::vector<bool>& inputValues) const
    {
        FinishJitting();

        if (GetInput(0)->GetOutputPort().GetType() != node->GetOutputPort().GetType())
        {
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }
        utilities::ConformingVector<bool> temp(inputValues.size());
        for (size_t index = 0; index < temp.size(); ++index)
        {
            temp[index] = static_cast<bool>(inputValues[index]);
        }

        std::get<ComputeFunction<bool>>(_computeInputFunction)(GetContext(), (bool*)temp.data());
    }

    void IRCompiledMap::SetNodeInput(model::InputNode<int>* node, const std::vector<int>& inputValues) const
    {
        FinishJitting();

        if (GetInput(0)->GetOutputPort().GetType() != node->GetOutputPort().GetType())
        {
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }

        std::get<ComputeFunction<int>>(_computeInputFunction)(GetContext(), inputValues.data());
    }

    void IRCompiledMap::SetNodeInput(model::InputNode<int64_t>* node, const std::vector<int64_t>& inputValues) const
    {
        FinishJitting();

        if (GetInput(0)->GetOutputPort().GetType() != node->GetOutputPort().GetType())
        {
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }

        std::get<ComputeFunction<int64_t>>(_computeInputFunction)(GetContext(), inputValues.data());
    }

    void IRCompiledMap::SetNodeInput(model::InputNode<float>* node, const std::vector<float>& inputValues) const
    {
        FinishJitting();

        if (GetInput(0)->GetOutputPort().GetType() != node->GetOutputPort().GetType())
        {
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }

        std::get<ComputeFunction<float>>(_computeInputFunction)(GetContext(), inputValues.data());
    }

    void IRCompiledMap::SetNodeInput(model::InputNode<double>* node, const std::vector<double>& inputValues) const
    {
        FinishJitting();
        if (GetInput(0)->GetOutputPort().GetType() != node->GetOutputPort().GetType())
        {
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }

        std::get<ComputeFunction<double>>(_computeInputFunction)(GetContext(), inputValues.data());
    }

    std::vector<bool> IRCompiledMap::ComputeBoolOutput(const model::PortElementsBase& outputs) const
    {
        FinishJitting();

        if (GetOutput(0).GetPortType() != model::Port::PortType::boolean)
        {
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }

        // Terrible hack to create a std::vector<bool>
        return std::vector<bool>((bool*)(std::get<utilities::ConformingVector<bool>>(_cachedOutput).data()), (bool*)(std::get<utilities::ConformingVector<bool>>(_cachedOutput).data() + std::get<utilities::ConformingVector<bool>>(_cachedOutput).size()));
    }

    std::vector<int> IRCompiledMap::ComputeIntOutput(const model::PortElementsBase& outputs) const
    {
        EnsureExecutionEngine();
        if (GetOutput(0).GetPortType() != model::Port::PortType::integer)
        {
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }

        return std::get<utilities::ConformingVector<int>>(_cachedOutput);
    }

    std::vector<int64_t> IRCompiledMap::ComputeInt64Output(const model::PortElementsBase& outputs) const
    {
        FinishJitting();
        if (GetOutput(0).GetPortType() != model::Port::PortType::bigInt)
        {
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }

        return std::get<utilities::ConformingVector<int64_t>>(_cachedOutput);
    }

    std::vector<float> IRCompiledMap::ComputeFloatOutput(const model::PortElementsBase& outputs) const
    {
        FinishJitting();
        if (GetOutput(0).GetPortType() != model::Port::PortType::smallReal)
        {
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }

        return std::get<utilities::ConformingVector<float>>(_cachedOutput);
    }

    std::vector<double> IRCompiledMap::ComputeDoubleOutput(const model::PortElementsBase& outputs) const
    {
        FinishJitting();
        if (GetOutput(0).GetPortType() != model::Port::PortType::real)
        {
            throw utilities::InputException(utilities::InputExceptionErrors::typeMismatch);
        }

        return std::get<utilities::ConformingVector<double>>(_cachedOutput);
    }

    void IRCompiledMap::WriteCode(const std::string& filePath) const
    {
        _module->WriteToFile(filePath);
    }

    void IRCompiledMap::WriteCode(const std::string& filePath, emitters::ModuleOutputFormat format) const
    {
        _module->WriteToFile(filePath, format);
    }

    void IRCompiledMap::WriteCode(const std::string& filePath, emitters::ModuleOutputFormat format, emitters::MachineCodeOutputOptions options) const
    {
        _module->WriteToFile(filePath, format, options);
    }

    void IRCompiledMap::WriteCodeHeader(const std::string& filePath) const
    {
        auto stream = utilities::OpenOfstream(filePath);
        WriteCodeHeader(stream);
    }

    void IRCompiledMap::WriteCode(std::ostream& stream, emitters::ModuleOutputFormat format) const
    {
        _module->WriteToStream(stream, format);
    }

    void IRCompiledMap::WriteCode(std::ostream& stream, emitters::ModuleOutputFormat format, emitters::MachineCodeOutputOptions options) const
    {
        _module->WriteToStream(stream, format, options);
    }

    void IRCompiledMap::WriteCodeHeader(std::ostream& stream) const
    {
        _module->WriteToStream(stream, emitters::ModuleOutputFormat::cHeader);
    }

    std::string IRCompiledMap::GetCodeHeaderString() const
    {
        std::stringstream s;
        WriteCodeHeader(s);
        return s.str();
    }

    //
    // Profiling support
    //

    void IRCompiledMap::PrintModelProfilingInfo()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<void (*)()>(jitter.GetFunctionAddress(_moduleName + "_PrintModelProfilingInfo"));
        fn();
    }

    PerformanceCounters* IRCompiledMap::GetModelPerformanceCounters()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<PerformanceCounters* (*)()>(jitter.GetFunctionAddress(_moduleName + "_GetModelPerformanceCounters"));
        return fn();
    }

    void IRCompiledMap::ResetModelProfilingInfo()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<void (*)()>(jitter.GetFunctionAddress(_moduleName + "_ResetModelProfilingInfo"));
        fn();
    }

    void IRCompiledMap::PrintNodeProfilingInfo()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<void (*)()>(jitter.GetFunctionAddress(_moduleName + "_PrintNodeProfilingInfo")); // TODO: hide this reinterpret_cast in a templated method of IRExecutionEngine
        fn();
    }

    void IRCompiledMap::ResetNodeProfilingInfo()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<void (*)()>(jitter.GetFunctionAddress(_moduleName + "_ResetNodeProfilingInfo"));
        fn();
    }

    int IRCompiledMap::GetNumProfiledNodes()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<int (*)()>(jitter.GetFunctionAddress(_moduleName + "_GetNumNodes"));
        return fn();
    }

    NodeInfo* IRCompiledMap::GetNodeInfo(int nodeIndex)
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<NodeInfo* (*)(int)>(jitter.GetFunctionAddress(_moduleName + "_GetNodeInfo"));
        return fn(nodeIndex);
    }

    PerformanceCounters* IRCompiledMap::GetNodePerformanceCounters(int nodeIndex)
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<PerformanceCounters* (*)(int)>(jitter.GetFunctionAddress(_moduleName + "_GetNodePerformanceCounters"));
        return fn(nodeIndex);
    }

    void IRCompiledMap::PrintNodeTypeProfilingInfo()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<void (*)()>(jitter.GetFunctionAddress(_moduleName + "_PrintNodeTypeProfilingInfo"));
        fn();
    }

    void IRCompiledMap::ResetNodeTypeProfilingInfo()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<void (*)()>(jitter.GetFunctionAddress(_moduleName + "_ResetNodeTypeProfilingInfo"));
        fn();
    }

    int IRCompiledMap::GetNumProfiledNodeTypes()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<int (*)()>(jitter.GetFunctionAddress(_moduleName + "_GetNumNodeTypes"));
        return fn();
    }

    NodeInfo* IRCompiledMap::GetNodeTypeInfo(int nodeIndex)
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<NodeInfo* (*)(int)>(jitter.GetFunctionAddress(_moduleName + "_GetNodeTypeInfo"));
        return fn(nodeIndex);
    }

    PerformanceCounters* IRCompiledMap::GetNodeTypePerformanceCounters(int nodeIndex)
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<PerformanceCounters* (*)(int)>(jitter.GetFunctionAddress(_moduleName + "_GetNodeTypePerformanceCounters"));
        return fn(nodeIndex);
    }

    //
    // Low-level region profiling support
    //
    int IRCompiledMap::GetNumProfileRegions()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<int (*)()>(jitter.GetFunctionAddress(_moduleName + "_GetNumProfileRegions"));
        return fn();
    }

    emitters::ProfileRegionInfo* IRCompiledMap::GetRegionProfilingInfo(int regionIndex)
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<ProfileRegionInfo* (*)(int)>(jitter.GetFunctionAddress(_moduleName + "_GetRegionProfilingInfo"));
        return fn(regionIndex);
    }

    void IRCompiledMap::ResetRegionProfilingInfo()
    {
        auto& jitter = GetJitter();
        auto fn = reinterpret_cast<void (*)()>(jitter.GetFunctionAddress(_moduleName + "_ResetRegionProfilingInfo"));
        fn();
    }
}
}
