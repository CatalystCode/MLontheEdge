////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     UniqueId.h (utilities)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "IArchivable.h"

// stl
#include <functional>
#include <ostream>
#include <string>

namespace ell
{
namespace utilities
{
    /// <summary> UniqueId: A placeholder for a real GUID-type class </summary>
    class UniqueId : public ArchivedAsPrimitive
    {
    public:
        /// <summary> Default constructor </summary>
        UniqueId();

        UniqueId(const UniqueId& other) = default;

        /// <summary> Constructor from string representation </summary>
        ///
        /// <param name="idString"> A string representation of a `UniqueId` </param>
        explicit UniqueId(const std::string& idString);

        UniqueId& operator=(const UniqueId& other) = default;

        /// <summary> Equality comparison </summary>
        bool operator==(const UniqueId& other) const;

        /// <summary> Inequality comparison </summary>
        bool operator!=(const UniqueId& other) const;

        /// <summary> Less-than comparison </summary>
        bool operator<(const UniqueId& other) const;

        /// <summary> Greater-than comparison </summary>
        bool operator>(const UniqueId& other) const;

        /// <summary> Gets the name of this type (for serialization). </summary>
        ///
        /// <returns> The name of this type. </returns>
        static std::string GetTypeName() { return "UniqueId"; }

        /// <summary> Gets the name of this type (for serialization). </summary>
        ///
        /// <returns> The name of this type. </returns>
        std::string GetRuntimeTypeName() const override { return GetTypeName(); }

        /// <summary> Stream output </summary>
        friend std::ostream& operator<<(std::ostream& stream, const UniqueId& id);

        /// <summary> String conversion </summary>
        friend std::string to_string(const UniqueId& id);

        /// <summary> String conversion </summary>
        std::string ToString() const { return _id; }

    protected:
        void WriteToArchive(Archiver& archiver) const override;
        void ReadFromArchive(Unarchiver& archiver) override;

    private:
        friend std::hash<UniqueId>;
        std::string _id = "0";
        static size_t _nextId;
    };

    std::string to_string(const UniqueId& id);
}
}

// custom specialization of std::hash so we can keep UniqueIds in containers that require hashable types
namespace std
{
/// <summary> Implements a hash function for the UniqueId class, so that it can be used with associative containers (maps, sets, and the like). </summary>
template <>
struct hash<ell::utilities::UniqueId>
{
    using argument_type = ell::utilities::UniqueId;
    using result_type = std::size_t;

    /// <summary> Computes a hash of the input value. </summary>
    ///
    /// <returns> A hash value for the given input. </returns>
    result_type operator()(argument_type const& id) const;
};
}
