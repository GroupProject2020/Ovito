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

    /*! destructor - supposed to clean up all alloced memory */
    Discs::~Discs()
    {
      if (_materialList) {
        free(_materialList);
        _materialList = nullptr;
      }
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
      materialList      = getParamData("materialList");
      colorData         = getParamData("color");
      colorOffset       = getParam1i("color_offset",0);
      auto colComps     = colorData && colorData->type == OSP_FLOAT3 ? 3 : 4;
      colorStride       = getParam1i("color_stride", colComps * sizeof(float));
      texcoordData      = getParamData("texcoord");
      
      if(discData.ptr == nullptr) {
        throw std::runtime_error("#ospray:geometry/discs: no 'discs' data specified");
      }

      // look at the data we were provided with ....
      numDiscs = discData->numBytes / bytesPerDisc;

      if (_materialList) {
        free(_materialList);
        _materialList = nullptr;
      }
  
      if (materialList) {
        void **ispcMaterials = (void**) malloc(sizeof(void*)*materialList->numItems);
        for (uint32_t i = 0; i < materialList->numItems; i++) {
          Material *m = ((Material**)materialList->data)[i];
          ispcMaterials[i] = m?m->getIE():nullptr;
        }
        _materialList = (void*)ispcMaterials;
      }

      const char* discPtr = (const char*)discData->data;
      bounds = empty;
      for(uint32_t i = 0; i < numDiscs; i++, discPtr += bytesPerDisc) {
        const float r = offset_radius < 0 ? radius : *(const float*)(discPtr + offset_radius);
        const vec3f center = *(const vec3f*)(discPtr + offset_center);
        bounds.extend(box3f(center - r, center + r));
      }
      
      ispc::DiscsGeometry_set(getIE(), model->getIE(), discData->data, _materialList,
        texcoordData ? (ispc::vec2f *)texcoordData->data : nullptr,
        colorData ? colorData->data : nullptr,
        colorOffset, colorStride,
        colorData && colorData->type == OSP_FLOAT4,
        numDiscs, bytesPerDisc,
        radius, materialID,
        offset_center, offset_normal, offset_radius,
        offset_materialID, offset_colorID);
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
