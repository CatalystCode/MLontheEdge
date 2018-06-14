////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     PortMemoryLayout.h (model)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// utilities
#include "IArchivable.h"

// llvm
#include <llvm/IR/Value.h>

// stl
#include <string>
#include <vector>

namespace ell
{
namespace emitters
{
    class IRFunctionEmitter;
}

namespace model
{
    /// <summary> A vector of numbers representing shape (extent) information of a multidimensional array. </summary>
    using Shape = std::vector<int>;

    /// <summary> Checks if two shapes are equal. </summary>
    ///
    /// <param name="shape1"> The first shape. </param>
    /// <param name="shape2"> The other shape. </param>
    ///
    /// <return> true if the shapes are equal </return>
    bool ShapesEqual(const Shape& shape1, const Shape& shape2);

    /// <summary> Returns the number of elements in a given shape. </summary>
    ///
    /// <param name="shape"> The shape. </param>
    ///
    /// <return> Number of elements in the shape </return>
    size_t NumElements(const Shape& shape);

    /// <summary> A struct representing the memory layout of port data. </summary>
    class PortMemoryLayout : public utilities::IArchivable
    {
    public:
        PortMemoryLayout() = default;

        /// <summary> Constructor from size only (no padding). </summary>
        ///
        /// <param name="size"> The extent of the active are of the memory region. </param>
        PortMemoryLayout(const Shape& size);

        /// <summary> Constructor from size and padding. </summary>
        ///
        /// <param name="size"> The extent of the active are of the memory region. </param>
        /// <param name="padding"> The amount of padding to apply to the beginning and end of each dimension </param>
        /// <param name="offset"> The offset into memory to the active area of the memory region. </param>
        PortMemoryLayout(const Shape& size, const Shape& padding);

        /// <summary> General constructor. </summary>
        ///
        /// <param name="size"> The extent of the active are of the memory region. </param>
        /// <param name="stride"> The extent of the allocated memory of the memory region. </param>
        /// <param name="offset"> The offset into memory to the active area of the memory region. </param>
        PortMemoryLayout(const Shape& size, const Shape& stride, const Shape& offset);

        /// <summary> Returns the number of dimensions in this memory layout </summary>
        ///
        /// <returns> The number of dimensions </summary>
        size_t NumDimensions() const { return _size.size(); }

        /// <summary>
        /// Returns the size of the "active" memory area (not counting any padding).
        /// </summary>
        ///
        /// <returns> A `Shape` object containing the size of the memory area. </returns>
        const Shape& GetActiveSize() const { return _size; }

        /// <summary>
        /// Returns the allocated size of the memory (including padding).
        /// </summary>
        ///
        /// <returns> A `Shape` object containing the allocated size in each dimension </returns>
        const Shape& GetStride() const { return _stride; }

        /// <summary>
        /// Returns the offsets to the "active" area of memory.
        /// </summary>
        ///
        /// <returns> A `Shape` object containing the offset to the active part of memory for that dimension. </returns>
        const Shape& GetOffset() const { return _offset; }

        /// <summary>
        /// Returns the cumulative increments for each dimension. This is the distance in memory
        /// between two entries that are adjacent in that dimension.
        /// </summary>
        ///
        /// <returns> A `Shape` object containing the cumulative increment in each dimension </returns>
        const Shape& GetCumulativeIncrement() const { return _increment; }

        /// <summary>
        /// Returns the size of the "active" memory area for the given dimension (not counting any padding).
        /// </summary>
        ///
        /// <param name="index"> The dimension. </param>
        ///
        /// <returns> The size of the memory area for the given dimension. </returns>
        int GetActiveSize(size_t index) const { return _size[index]; }

        /// <summary>
        /// Returns the allocated size of the memory for the given dimension (including padding).
        /// </summary>
        ///
        /// <param name="index"> The dimension. </param>
        ///
        /// <returns> The allocated size in each dimension </returns>
        int GetStride(size_t index) const { return _stride[index]; }

        /// <summary>
        /// Returns the offset to the "active" area of memory for the given dimension.
        /// </summary>
        ///
        /// <param name="index"> The dimension. </param>
        ///
        /// <returns> The offset to the active part of memory for the given dimension. </returns>
        int GetOffset(size_t index) const { return _offset[index]; }
        
        /// <summary>
        /// Returns the cumulative increment for the requested dimension. This is the distance in memory
        /// between two entries that are adjacent in that dimension.
        /// </summary>
        ///
        /// <param name="index"> The dimension. </param>
        ///
        /// <returns> The cumulative increment for the given dimension. </returns>
        size_t GetCumulativeIncrement(size_t index) const { return _increment[index]; }

        /// <summary> Returns the number of active elements in this memory layout </summary>
        ///
        /// <returns> The number of elements </summary>
        size_t NumEntries() const;

        /// <summary> Returns the number of total (active plus extra stride) elements in this memory layout </summary>
        ///
        /// <returns> The number of elements </summary>
        size_t GetMemorySize() const;

        /// <summary> Gets the offset into memory for an entry </summary>
        ///
        /// <param name="location"> The coordinates of the entry </param>
        /// <returns> The offset to the entry (from the beginning of memory) </returns>
        size_t GetEntryOffset(const Shape& location) const;

        /// <summary> Checks if a location is outside of the stored memory extent in any dimension </summary>
        ///
        /// <param name="location"> The coordinates of the entry </param>
        /// <returns> `true` if the location is out of bounds </returns>
        bool IsOutOfBounds(const Shape& location) const;

        /// <summary> Emits LLVM IR to get the offset into memory for an entry </summary>
        ///
        /// <param name="location"> The coordinates of the entry </param>
        /// <returns> The offset to the entry (from the beginning of memory) </returns>
        llvm::Value* EmitGetEntryOffset(emitters::IRFunctionEmitter& function, const std::vector<llvm::Value*>& location) const;

        /// <summary> Emits LLVM IR to check if a location is outside of the stored memory extent in any dimension </summary>
        ///
        /// <param name="location"> The coordinates of the entry </param>
        /// <returns> A value representing `true` if the location is out of bounds </returns>
        llvm::Value* EmitIsOutOfBounds(emitters::IRFunctionEmitter& function, const std::vector<llvm::Value*>& location) const;

        /// <summary> Gets the name of this type (for serialization). </summary>
        ///
        /// <returns> The name of this type. </returns>
        static std::string GetTypeName() { return "PortMemoryLayout"; }

        /// <summary> Gets the name of this type (for serialization). </summary>
        ///
        /// <returns> The name of this type. </returns>
        std::string GetRuntimeTypeName() const override { return GetTypeName(); }

    protected:
        size_t GetDataOffset() const; // offset for logical entry {0,0,0...}
        void WriteToArchive(utilities::Archiver& archiver) const override;
        void ReadFromArchive(utilities::Unarchiver& archiver) override;

    private:
        Shape ComputeCumulativeIncrement() const;

        Shape _size; // the "active" area of the memory
        Shape _stride; // the allocated size along each dimension
        Shape _offset; // the offset to the active area for each dimension
        Shape _increment; // the distance in memory between adjacent elements for each dimension
    };

    /// <summary> Checks if two shapes are equal. </summary>
    ///
    /// <param name="shape1"> The first shape. </param>
    /// <param name="shape2"> The other shape. </param>
    bool ShapesEqual(const Shape& shape1, const Shape& shape2);

    /// <summary> Checks if two memory layouts are equal. </summary>
    ///
    /// <param name="layout1"> The first memory layout. </param>
    /// <param name="layout2"> The other memory layout. </param>
    bool PortMemoryLayoutsEqual(const PortMemoryLayout& layout1, const PortMemoryLayout& layout2);
}
}
