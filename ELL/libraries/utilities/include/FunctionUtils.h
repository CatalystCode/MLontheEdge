////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     FunctionUtils.h (utilities)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Unused.h"

// stl
#include <tuple>
#include <functional>

namespace ell
{
namespace utilities
{

    /// <summary>
    /// InOrderFunctionEvaluator() is a template function that evaluates a number of zero-argument functions in order.
    /// Usage:
    ///
    /// InOrderFunctionEvaluator(f1, f2, f3);
    ///
    /// The above is equivalent to:
    ///
    /// f1(); f2(); f3()
    ///
    /// </summary>

    /// <summary> Recursive base case with zero functions. Does nothing. </summary>
    inline void InOrderFunctionEvaluator() {}

    /// <summary> Invokes a series of zero-argument functions. </summary>
    ///
    /// <param name="function"> The first function to evaluate </param>
    /// <param name="functions"> The rest of the functions to evaluate </param>
    template <typename Function, typename... Functions>
    void InOrderFunctionEvaluator(Function&& function, Functions&&... functions);

    /// <summary>
    /// ApplyToEach() is a template function that applies a single-argument function to each
    /// of a number of arguments.
    /// Usage:
    ///
    /// ApplyToEach(f, arg1, arg2, arg3);
    ///
    /// The above is equivalent to:
    ///
    /// f(arg1); f(arg2); f(arg3);
    ///
    /// </summary>

    /// <summary> Recursive base case with zero arguments. Does nothing. </summary>
    template <typename FunctionType>
    inline void ApplyToEach(FunctionType&& function)
    {
        UNUSED(function);
    }

    /// <summary> Applies a single-argument function to each of a number of arguments. </summary>
    ///
    /// <param name="function"> The function to apply </param>
    /// <param name="arg"> The first argument to apply the function to </param>
    /// <param name="args"> The rest of the arguments to apply the function to </param>
    template <typename FunctionType, typename Arg, typename... Args>
    void ApplyToEach(FunctionType&& function, Arg&& arg, Args&&... args);
    
    //
    // FunctionTraits
    //

    /// <summary> FunctionTraits: A type-traits-like way to get the return type and argument types of a function </summary>
    ///
    template <typename T>
    struct FunctionTraits; // undefined base template

    // Function pointers
    template <typename ReturnT, typename... Args>
    struct FunctionTraits<ReturnT(Args...)>
    {
        using ReturnType = ReturnT;
        using ArgTypes = std::tuple<Args...>;
        static constexpr size_t NumArgs = typename std::tuple_size<ArgTypes>();
    };

    // std::function
    template <typename ReturnT, typename... Args>
    struct FunctionTraits<std::function<ReturnT(Args...)>>
    {
        using ReturnType = ReturnT;
        using ArgTypes = std::tuple<Args...>;
        static constexpr size_t NumArgs = typename std::tuple_size<ArgTypes>();
    };

    template <typename ReturnT, typename... Args>
    struct FunctionTraits<const std::function<ReturnT(Args...)>>
    {
        using ReturnType = ReturnT;
        using ArgTypes = std::tuple<Args...>;
        static constexpr size_t NumArgs = typename std::tuple_size<ArgTypes>();
    };

    // Handy type aliases
    template <typename FunctionType>
    using FunctionReturnType = typename FunctionTraits<FunctionType>::ReturnType;

    template <typename FunctionType>
    using FunctionArgTypes = typename FunctionTraits<FunctionType>::ArgTypes;

    /// <summary> Returns a default-constructed tuple of types the given function expects as arguments </summary>
    template <typename FunctionType>
    FunctionArgTypes<FunctionType> GetFunctionArgTuple(FunctionType& function);

    /// <summary> Calls the given function with the given arguments </summary>
    template <typename FunctionType, typename... Args>
    auto ApplyFunction(const FunctionType& function, Args... args) -> FunctionReturnType<FunctionType>;

    /// <summary> Calls the given function with the given arguments </summary>
    template <typename FunctionType, typename... Args>
    auto ApplyFunction(const FunctionType& function, std::tuple<Args...>&& args) -> FunctionReturnType<FunctionType>;
}
}

#include "../tcc/FunctionUtils.tcc"
