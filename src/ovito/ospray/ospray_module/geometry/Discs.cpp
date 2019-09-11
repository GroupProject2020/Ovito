///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include "Discs.h"
// 'export'ed functions from the ispc file:
#include "Discs_ispc.h"
// ospray core:
#include <common/Data.h>

/*! _everything_ in the ospray core universe should _always_ be in the
  'ospray' namespace. */
namespace ospray {
  namespace ovito {

    /*! constructor - will create the 'ispc equivalent' */
    Discs::Discs()
    {
      /*! create the 'ispc equivalent': ie, the ispc-side class that
        implements all the ispc-side code for intersection,
        postintersect, etc. See BilinearPatches.ispc */
      this->ispcEquivalent = ispc::Discs_create(this);

      // note we do _not_ yet do anything else here - the actual input
      // data isn't available to use until 'commit()' gets called
    }

    /*! 'finalize' is what ospray calls when everything is set and
        done, and a actual user geometry has to be built */
    void Discs::finalize(Model *model)
    {
      radius            = getParam1f("radius",0.01f);
      materialID        = getParam1i("materialID",0);
      bytesPerDisc      = getParam1i("bytes_per_disc",6*sizeof(float));
      offset_center     = getParam1i("offset_center",0);
      offset_normal     = getParam1i("offset_normal",3*sizeof(float));
      offset_radius     = getParam1i("offset_radius",-1);
      offset_materialID = getParam1i("offset_materialID",-1);
      offset_colorID    = getParam1i("offset_colorID",-1);
      discData          = getParamData("discs");
      colorData         = getParamData("color");
      colorOffset       = getParam1i("color_offset",0);
      texcoordData      = getParamData("texcoord");
      
      if (colorData) {
        if (hasParam("color_format")) {
          colorFormat = static_cast<OSPDataType>(getParam1i("color_format", OSP_UNKNOWN));
        } else {
          colorFormat = colorData->type;
        }
        if (colorFormat != OSP_FLOAT4 && colorFormat != OSP_FLOAT3
            && colorFormat != OSP_FLOAT3A && colorFormat != OSP_UCHAR4)
        {
          throw std::runtime_error("#ospray:geometry/discs: invalid "
                                  "colorFormat specified! Must be one of: "
                                  "OSP_FLOAT4, OSP_FLOAT3, OSP_FLOAT3A or "
                                  "OSP_UCHAR4.");
        }
      } else {
        colorFormat = OSP_UNKNOWN;
      }
      colorStride = getParam1i("color_stride",
                              colorFormat == OSP_UNKNOWN ?
                              0 : sizeOf(colorFormat));

      if(discData.ptr == nullptr) {
        throw std::runtime_error("#ospray:geometry/discs: no 'discs' data specified");
      }

      // look at the data we were provided with ....
      numDiscs = discData->numBytes / bytesPerDisc;

      if (numDiscs >= (1ULL << 30)) {
        throw std::runtime_error("#ospray::Discs: too many discs in this "
                                "cones geometry. Consider splitting this "
                                "geometry in multiple geometries with fewer "
                                "discs (you can still put all those "
                                "geometries into a single model, but you can't "
                                "put that many discs into a single geometry "
                                "without causing address overflows)");
      }

      // check whether we need 64-bit addressing
      bool huge_mesh = false;
      if (colorData && colorData->numBytes > INT32_MAX)
        huge_mesh = true;
      if (texcoordData && texcoordData->numBytes > INT32_MAX)
        huge_mesh = true;
        
      
      ispc::DiscsGeometry_set(getIE(), model->getIE(), discData->data, 
        materialList ? ispcMaterialPtrs.data() : nullptr,
        texcoordData ? (ispc::vec2f *)texcoordData->data : nullptr,
        colorData ? colorData->data : nullptr,
        colorOffset, colorStride, colorFormat,
        numDiscs, bytesPerDisc,
        radius, materialID,
        offset_center, offset_normal, offset_radius,
        offset_materialID, offset_colorID,
        huge_mesh);
    }


    /*! This macro 'registers' the Discs class under the ospray
        geometry type name of 'discs'.

        It is _this_ name that one can now (assuming the module has
        been loaded with ospLoadModule(), of course) create geometries
        with; i.e.,

        OSPGeometry geom = ospNewGeometry("discs") ;
    */
    OSP_REGISTER_GEOMETRY(Discs,discs);

  } // ::ospray::ovito
} // ::ospray
