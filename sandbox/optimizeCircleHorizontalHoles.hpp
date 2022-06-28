#ifndef OPTIMIZE_HORIZONTAL_CIRCLE_HOLES_HPP
#define OPTIMIZE_HORIZONTAL_CIRCLE_HOLES_HPP

#include "selectPlane.hpp"
#include <set>

std::set<ksFaceDefinitionPtr> getHorizontalCircleHoles(ksPartPtr part, ksFaceDefinitionPtr printFace, PlaneEq plane);

#endif