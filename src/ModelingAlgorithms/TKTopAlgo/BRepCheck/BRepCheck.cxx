// Created on: 1995-12-08
// Created by: Jacques GOUSSARD
// Copyright (c) 1995-1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
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

#include <BRepCheck.hxx>

#include <BRep_Tool.hxx>
#include <BRepCheck_Status.hxx>
#include <NCollection_List.hxx>
#include <NCollection_Shared.hxx>
#include <BRepCheck_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <Adaptor3d_Curve.hxx>
#include <Adaptor3d_Surface.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <gp_Elips.hxx>

//=================================================================================================

void BRepCheck::Add(NCollection_List<BRepCheck_Status>& lst, const BRepCheck_Status stat)
{
  NCollection_List<BRepCheck_Status>::Iterator it(lst);
  while (it.More())
  {
    if (it.Value() == BRepCheck_Status::BRepCheck_NoError
        && stat != BRepCheck_Status::BRepCheck_NoError)
    {
      lst.Remove(it);
    }
    else
    {
      if (it.Value() == stat)
      {
        return;
      }
      it.Next();
    }
  }
  lst.Append(stat);
}

//=================================================================================================

bool BRepCheck::SelfIntersection(const TopoDS_Wire& W,
                                 const TopoDS_Face& myFace,
                                 TopoDS_Edge&       RetE1,
                                 TopoDS_Edge&       RetE2)
{
  occ::handle<BRepCheck_Wire> chkw = new BRepCheck_Wire(W);
  BRepCheck_Status            stat = chkw->SelfIntersect(myFace, RetE1, RetE2);
  return (stat == BRepCheck_Status::BRepCheck_SelfIntersectingWire);
}

//=================================================================================================

double BRepCheck::PrecCurve(const Adaptor3d_Curve& aAC3D)
{
  double aXEmax = RealEpsilon();
  //
  GeomAbs_CurveType aCT = aAC3D.GetType();
  if (aCT == GeomAbs_CurveType::GeomAbs_Ellipse)
  {
    double aX[5];
    //
    gp_Elips aEL3D = aAC3D.Ellipse();
    aEL3D.Location().Coord(aX[0], aX[1], aX[2]);
    aX[3]  = aEL3D.MajorRadius();
    aX[4]  = aEL3D.MinorRadius();
    aXEmax = -1.;
    for (int i = 0; i < 5; ++i)
    {
      if (aX[i] < 0.)
      {
        aX[i] = -aX[i];
      }
      double aXE = Epsilon(aX[i]);
      if (aXE > aXEmax)
      {
        aXEmax = aXE;
      }
    }
  } // if (aCT=GeomAbs_CurveType::GeomAbs_Ellipse) {
  //
  return aXEmax;
}

//=================================================================================================

double BRepCheck::PrecSurface(const occ::handle<Adaptor3d_Surface>& aAHSurf)
{
  double aXEmax = RealEpsilon();
  //
  GeomAbs_SurfaceType aST = aAHSurf->GetType();
  if (aST == GeomAbs_SurfaceType::GeomAbs_Cone)
  {
    gp_Cone aCone = aAHSurf->Cone();
    double  aX[4];
    //
    aCone.Location().Coord(aX[0], aX[1], aX[2]);
    aX[3]  = aCone.RefRadius();
    aXEmax = -1.;
    for (int i = 0; i < 4; ++i)
    {
      if (aX[i] < 0.)
      {
        aX[i] = -aX[i];
      }
      double aXE = Epsilon(aX[i]);
      if (aXE > aXEmax)
      {
        aXEmax = aXE;
      }
    }
  } // if (aST==GeomAbs_SurfaceType::GeomAbs_Cone) {
  return aXEmax;
}

//=================================================================================================

void BRepCheck::Print(const BRepCheck_Status stat, Standard_OStream& OS)
{

  switch (stat)
  {
    case BRepCheck_Status::BRepCheck_NoError:
      OS << "BRepCheck_Status::BRepCheck_NoError\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidPointOnCurve:
      OS << "BRepCheck_Status::BRepCheck_InvalidPointOnCurve\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidPointOnCurveOnSurface:
      OS << "BRepCheck_Status::BRepCheck_InvalidPointOnCurveOnSurface\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidPointOnSurface:
      OS << "BRepCheck_Status::BRepCheck_InvalidPointOnSurface\n";
      break;
    case BRepCheck_Status::BRepCheck_No3DCurve:
      OS << "BRepCheck_Status::BRepCheck_No3DCurve\n";
      break;
    case BRepCheck_Status::BRepCheck_Multiple3DCurve:
      OS << "BRepCheck_Status::BRepCheck_Multiple3DCurve\n";
      break;
    case BRepCheck_Status::BRepCheck_Invalid3DCurve:
      OS << "BRepCheck_Status::BRepCheck_Invalid3DCurve\n";
      break;
    case BRepCheck_Status::BRepCheck_NoCurveOnSurface:
      OS << "BRepCheck_Status::BRepCheck_NoCurveOnSurface\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidCurveOnSurface:
      OS << "BRepCheck_Status::BRepCheck_InvalidCurveOnSurface\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidCurveOnClosedSurface:
      OS << "BRepCheck_Status::BRepCheck_InvalidCurveOnClosedSurface\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidSameRangeFlag:
      OS << "BRepCheck_Status::BRepCheck_InvalidSameRangeFlag\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidSameParameterFlag:
      OS << "BRepCheck_Status::BRepCheck_InvalidSameParameterFlag\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidDegeneratedFlag:
      OS << "BRepCheck_Status::BRepCheck_InvalidDegeneratedFlag\n";
      break;
    case BRepCheck_Status::BRepCheck_FreeEdge:
      OS << "BRepCheck_Status::BRepCheck_FreeEdge\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidMultiConnexity:
      OS << "BRepCheck_Status::BRepCheck_InvalidMultiConnexity\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidRange:
      OS << "BRepCheck_Status::BRepCheck_InvalidRange\n";
      break;
    case BRepCheck_Status::BRepCheck_EmptyWire:
      OS << "BRepCheck_Status::BRepCheck_EmptyWire\n";
      break;
    case BRepCheck_Status::BRepCheck_RedundantEdge:
      OS << "BRepCheck_Status::BRepCheck_RedundantEdge\n";
      break;
    case BRepCheck_Status::BRepCheck_SelfIntersectingWire:
      OS << "BRepCheck_Status::BRepCheck_SelfIntersectingWire\n";
      break;
    case BRepCheck_Status::BRepCheck_NoSurface:
      OS << "BRepCheck_Status::BRepCheck_NoSurface\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidWire:
      OS << "BRepCheck_Status::BRepCheck_InvalidWire\n";
      break;
    case BRepCheck_Status::BRepCheck_RedundantWire:
      OS << "BRepCheck_Status::BRepCheck_RedundantWire\n";
      break;
    case BRepCheck_Status::BRepCheck_IntersectingWires:
      OS << "BRepCheck_Status::BRepCheck_IntersectingWires\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidImbricationOfWires:
      OS << "BRepCheck_Status::BRepCheck_InvalidImbricationOfWires\n";
      break;
    case BRepCheck_Status::BRepCheck_EmptyShell:
      OS << "BRepCheck_Status::BRepCheck_EmptyShell\n";
      break;
    case BRepCheck_Status::BRepCheck_RedundantFace:
      OS << "BRepCheck_Status::BRepCheck_RedundantFace\n";
      break;
    case BRepCheck_Status::BRepCheck_UnorientableShape:
      OS << "BRepCheck_Status::BRepCheck_UnorientableShape\n";
      break;
    case BRepCheck_Status::BRepCheck_NotClosed:
      OS << "BRepCheck_Status::BRepCheck_NotClosed\n";
      break;
    case BRepCheck_Status::BRepCheck_NotConnected:
      OS << "BRepCheck_Status::BRepCheck_NotConnected\n";
      break;
    case BRepCheck_Status::BRepCheck_SubshapeNotInShape:
      OS << "BRepCheck_Status::BRepCheck_SubshapeNotInShape\n";
      break;
    case BRepCheck_Status::BRepCheck_BadOrientation:
      OS << "BRepCheck_Status::BRepCheck_BadOrientation\n";
      break;
    case BRepCheck_Status::BRepCheck_BadOrientationOfSubshape:
      OS << "BRepCheck_Status::BRepCheck_BadOrientationOfSubshape\n";
      break;
    case BRepCheck_Status::BRepCheck_CheckFail:
      OS << "BRepCheck_Status::BRepCheck_CheckFail\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidPolygonOnTriangulation:
      OS << "BRepCheck_Status::BRepCheck_InvalidPolygonOnTriangulation\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidToleranceValue:
      OS << "BRepCheck_Status::BRepCheck_InvalidToleranceValue\n";
      break;
    case BRepCheck_Status::BRepCheck_InvalidImbricationOfShells:
      OS << "BRepCheck_Status::BRepCheck_InvalidImbricationOfShells\n";
      break;
    case BRepCheck_Status::BRepCheck_EnclosedRegion:
      OS << "BRepCheck_Status::BRepCheck_EnclosedRegion\n";
      break;
    default:
      break;
  }
}
