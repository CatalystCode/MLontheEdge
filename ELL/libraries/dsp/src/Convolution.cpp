////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     Convolution.cpp (dsp)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Convolution.h"
#include "SimpleConvolution.h"
#include "UnrolledConvolution.h"
#include "WinogradConvolution.h"

namespace ell
{
namespace dsp
{
    //
    // Functions
    //

    template<typename ValueType>
    math::RowVector<ValueType> Convolve1D(const math::RowVector<ValueType>& signal, const math::RowVector<ValueType>& filter, ConvolutionMethodOption alg)
    {
        switch (alg)
        {
        case ConvolutionMethodOption::automatic:
        // fallthrough
        case ConvolutionMethodOption::simple:
            return Convolve1DSimple(signal, filter);
        case ConvolutionMethodOption::unrolled:
            return Convolve1DUnrolled(signal, filter);
        case ConvolutionMethodOption::winograd:
            return Convolve1DWinograd(signal, filter);
        default:
            throw utilities::LogicException(utilities::LogicExceptionErrors::notImplemented);
        }
    }

    template<typename ValueType>
    math::ChannelColumnRowTensor<ValueType> Convolve2D(const math::ChannelColumnRowTensor<ValueType>& signal, const math::ChannelColumnRowTensor<ValueType>& filters, int numFilters, ConvolutionMethodOption alg)
    {
        return Convolve2D(signal, filters, numFilters, 1, alg);
    }

    template<typename ValueType>
    math::ChannelColumnRowTensor<ValueType> Convolve2D(const math::ChannelColumnRowTensor<ValueType>& signal, const math::ChannelColumnRowTensor<ValueType>& filters, int numFilters, int stride, ConvolutionMethodOption alg)
    {
        switch (alg)
        {
        case ConvolutionMethodOption::automatic:
        // fallthrough
        case ConvolutionMethodOption::simple:
            return Convolve2DSimple(signal, filters, numFilters, stride);
        case ConvolutionMethodOption::unrolled:
            return Convolve2DUnrolled(signal, filters, numFilters, stride);
        case ConvolutionMethodOption::winograd:
            if (stride == 1)
            {
                const int tileSize = 2;
                return Convolve2DWinograd(signal, filters, numFilters, tileSize);
            }
        default:
            break;
        }
        throw utilities::LogicException(utilities::LogicExceptionErrors::notImplemented);
    }

    //
    // Explicit instantiation definitions:
    //
    template math::RowVector<float> Convolve1D(const math::RowVector<float>& signal, const math::RowVector<float>& filter, ConvolutionMethodOption alg);
    template math::RowVector<double> Convolve1D(const math::RowVector<double>& signal, const math::RowVector<double>& filter, ConvolutionMethodOption alg);

    template math::ChannelColumnRowTensor<float> Convolve2D(const math::ChannelColumnRowTensor<float>& signal, const math::ChannelColumnRowTensor<float>& filters, int numFilters, ConvolutionMethodOption alg);
    template math::ChannelColumnRowTensor<double> Convolve2D(const math::ChannelColumnRowTensor<double>& signal, const math::ChannelColumnRowTensor<double>& filters, int numFilters, ConvolutionMethodOption alg);

    template math::ChannelColumnRowTensor<float> Convolve2D(const math::ChannelColumnRowTensor<float>& signal, const math::ChannelColumnRowTensor<float>& filters, int numFilters, int stride, ConvolutionMethodOption alg);
    template math::ChannelColumnRowTensor<double> Convolve2D(const math::ChannelColumnRowTensor<double>& signal, const math::ChannelColumnRowTensor<double>& filters, int numFilters, int stride, ConvolutionMethodOption alg);
}
}
