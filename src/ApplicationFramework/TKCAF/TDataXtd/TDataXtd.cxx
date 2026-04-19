// Created on: 2008-05-29
// Created by: Sergey ZARITCHNY
// Copyright (c) 2008-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#include <TDataXtd.hxx>
#include <TDataXtd_Axis.hxx>
#include <TDataXtd_Constraint.hxx>
#include <TDataXtd_Geometry.hxx>
#include <TDataXtd_PatternStd.hxx>
#include <TDataXtd_Placement.hxx>
#include <TDataXtd_Plane.hxx>
#include <TDataXtd_Point.hxx>
#include <TDataXtd_Position.hxx>
#include <TDataXtd_Shape.hxx>
#include <Standard_GUID.hxx>
#include <NCollection_List.hxx>

//=================================================================================================

void TDataXtd::IDList(NCollection_List<Standard_GUID>& anIDList)
{
  anIDList.Append(TDataXtd_Axis::GetID());
  anIDList.Append(TDataXtd_Constraint::GetID());
  anIDList.Append(TDataXtd_Geometry::GetID());
  anIDList.Append(TDataXtd_PatternStd::GetID());
  anIDList.Append(TDataXtd_Placement::GetID());
  anIDList.Append(TDataXtd_Point::GetID());
  anIDList.Append(TDataXtd_Plane::GetID());
  anIDList.Append(TDataXtd_Position::GetID());
  anIDList.Append(TDataXtd_Shape::GetID());
}

//=================================================================================================

Standard_OStream& TDataXtd::Print(const TDataXtd_ConstraintEnum C, Standard_OStream& s)
{
  switch (C)
  {

      // 2d planar constraints

    case TDataXtd_ConstraintEnum::TDataXtd_RADIUS: {
      s << "RADIUS";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_DIAMETER: {
      s << "DIAMETER";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_MINOR_RADIUS: {
      s << "MINOR_RADIUS";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_MAJOR_RADIUS: {
      s << "MAJOR_RADIUS";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_TANGENT: {
      s << "TANGENT";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_PARALLEL: {
      s << "PARALLEL";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_PERPENDICULAR: {
      s << "PERPENDICULAR";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_CONCENTRIC: {
      s << "CONCENTRIC";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_COINCIDENT: {
      s << "COINCIDENT";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_DISTANCE: {
      s << "DISTANCE";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_ANGLE: {
      s << "ANGLE";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_EQUAL_RADIUS: {
      s << "EQUAL_RADIUS";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_SYMMETRY: {
      s << "SYMMETRY";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_MIDPOINT: {
      s << "MIDPOINT";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_EQUAL_DISTANCE: {
      s << "EQUAL_DISTANCE";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_FIX: {
      s << "FIX";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_RIGID: {
      s << "RIGID";
      break;
    }

      // placement constraint

    case TDataXtd_ConstraintEnum::TDataXtd_FROM: {
      s << "FROM";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_AXIS: {
      s << "AXIS";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_MATE: {
      s << "MATE";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_ALIGN_FACES: {
      s << "ALIGN_FACES";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_ALIGN_AXES: {
      s << "ALIGN_AXES";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_AXES_ANGLE: {
      s << "AXES_ANGLE";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_FACES_ANGLE: {
      s << "FACES_ANGLE";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_ROUND: {
      s << "ROUND";
      break;
    }
    case TDataXtd_ConstraintEnum::TDataXtd_OFFSET: {
      s << "OFFSET";
      break;
    }

    default: {
      s << "UNKNOWN";
      break;
    }
  }
  return s;
}

//=================================================================================================

Standard_OStream& TDataXtd::Print(const TDataXtd_GeometryEnum G, Standard_OStream& s)
{
  switch (G)
  {
    case TDataXtd_GeometryEnum::TDataXtd_ANY_GEOM: {
      s << "ANY_GEOM";
      break;
    }
    case TDataXtd_GeometryEnum::TDataXtd_POINT: {
      s << "POINT";
      break;
    }
    case TDataXtd_GeometryEnum::TDataXtd_LINE: {
      s << "LINE";
      break;
    }
    case TDataXtd_GeometryEnum::TDataXtd_CIRCLE: {
      s << "CIRCLE";
      break;
    }
    case TDataXtd_GeometryEnum::TDataXtd_ELLIPSE: {
      s << "ELLIPSE";
      break;
    }
    case TDataXtd_GeometryEnum::TDataXtd_SPLINE: {
      s << "SPLINE";
      break;
    }
    case TDataXtd_GeometryEnum::TDataXtd_PLANE: {
      s << "PLANE";
      break;
    }
    case TDataXtd_GeometryEnum::TDataXtd_CYLINDER: {
      s << "CYLINDER";
      break;
    }
    default: {
      s << "UNKNOWN";
      break;
    }
  }
  return s;
}
