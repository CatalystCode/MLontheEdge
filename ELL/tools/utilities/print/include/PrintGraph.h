////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     PrintGraph.h (print)
//  Authors:  Chris Lovett
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// model
#include "Model.h"

// stl
#include <ostream>

namespace ell
{
void PrintGraph(const model::Model& model, const std::string& outputFormat,
                std::ostream& out, bool includeNodeId);
}
