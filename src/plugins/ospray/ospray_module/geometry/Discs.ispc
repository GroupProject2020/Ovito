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

// ospray
#include "math/vec.ih"
#include "math/box.ih"
#include "common/Ray.ih"
#include "common/Model.ih"
#include "geometry/Geometry.ih"
#include "math/sampling.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct Discs {
  /*! inherit from "Geometry" class: */
  Geometry   super; 

  /*! data array that contains the disc data (possibly but not
      necessarily including the color, which could be in color);
      most offset and stride values point into here.  */
  uint8     *data;
  int32      stride;
  int        offset_center;
  int        offset_normal;
  int        offset_radius;
  int        offset_materialID;
  int        offset_colorID; 
  
  float      radius;

  /*! list of multiple  materials, in case of per-disc materials */
  Material **materialList;

  int        materialID;

  uint8     *color;
  int        color_stride;
  int        color_offset;
  bool       has_alpha; // 4th color component is valid
 
  vec2f     *texcoord;
};

static void Discs_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  uniform Discs *uniform self = (uniform Discs *uniform)geometry;

  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR) && self->color) {
    uint32 colorID = 0;
    if (self->offset_colorID >= 0) {
      uniform uint8 *varying discPtr =
        self->data + self->stride*ray.primID;
      colorID = *((uniform uint32 *varying)(discPtr+self->offset_colorID));
    } else
      colorID = ray.primID;
    const uint64 colorAddr = self->color_offset+((uint64)colorID)*self->color_stride;
    dg.color = *((vec4f *)((uint64)self->color+colorAddr));
    if (!self->has_alpha)
      dg.color.w = 1.f;
  }

  if (flags & DG_TEXCOORD && self->texcoord)
    dg.st = self->texcoord[ray.primID];
  else
    dg.st = make_vec2f(0.0f);
  
  if (flags & DG_MATERIALID) {
    if (self->offset_materialID >= 0) {
      const uniform int32 primsPerPage = (1024*1024*64);
      if (any(ray.primID >= primsPerPage )) {
        const int primPageID  = ray.primID / primsPerPage;
        const int localPrimID = ray.primID % primsPerPage;
        foreach_unique(primPage in primPageID) {
          uniform uint8 *uniform pagePtr   = self->data
            + (((int64)primPage)
               * primsPerPage
               * self->stride);
          uniform uint8 *varying discPtr = pagePtr
            + self->stride*localPrimID;
          dg.materialID =
            *((uniform uint32 *varying)(discPtr+self->offset_materialID));
          if (self->materialList) {
            dg.material = self->materialList[dg.materialID];
          }
        }
      } else {
        uniform uint8 *varying discPtr = self->data
          + self->stride*ray.primID;
        dg.materialID =
          *((uniform uint32 *varying)(discPtr+self->offset_materialID));
        if (self->materialList) {
          dg.material = self->materialList[dg.materialID];
        }
      }
    } else {
      dg.materialID = self->materialID;
      if (self->materialList) {
        dg.material = self->materialList[dg.materialID];
      }
    }
  }
}

unmasked void Discs_bounds(uniform Discs *uniform self,
                             uniform size_t primID,
                             uniform box3fa &bbox)
{
  uniform uint8 *uniform discPtr = self->data
    + self->stride*((uniform int64)primID);
  uniform bool offr = self->offset_radius >= 0;
  uniform float radius =
    offr ? *((uniform float *uniform)(discPtr+self->offset_radius)) :
    self->radius;
  uniform vec3f center =
    *((uniform vec3f*uniform)(discPtr+self->offset_center));
  bbox = make_box3fa(center - radius, center + radius);
}

void Discs_intersect(uniform Discs *uniform self,
                       varying Ray &ray,
                       uniform size_t primID)
{
  uniform uint8 *uniform discPtr =
    self->data + self->stride*((uniform int64)primID);
  uniform float radius = self->radius;
  if (self->offset_radius >= 0) {
    radius = *((uniform float *uniform)(discPtr+self->offset_radius));
  }
  uniform vec3f center = *((uniform vec3f*uniform)(discPtr+self->offset_center));
  uniform vec3f normal = *((uniform vec3f*uniform)(discPtr+self->offset_normal));

  const float d = -dot(center, normal);
  float t = -(d + dot(normal, ray.org));
  const float td = dot(normal, ray.dir);
  if(td == 0.f) return;

  t = t / td;
  if(t > ray.t0 && t < ray.t) {
    const vec3f hitpnt = ray.org + t * ray.dir - center;
    if(dot(hitpnt,hitpnt) < radius*radius) {
      ray.t = t;
      ray.primID = primID;
      ray.geomID = self->super.geomID;
      ray.Ng = normal;
    }
  }
}

export void *uniform Discs_create(void *uniform cppEquivalent)
{
  uniform Discs *uniform self = uniform new uniform Discs;
  Geometry_Constructor(&self->super,cppEquivalent,
                       Discs_postIntersect,
                       NULL,0,NULL);
  return self;
}

export void DiscsGeometry_set(void  *uniform _self
    , void *uniform _model
    , void *uniform data
    , void *uniform materialList
    , vec2f *uniform texcoord
    , void *uniform color
    , uniform int color_offset
    , uniform int color_stride
    , uniform bool has_alpha
    , uniform int numDiscs
    , uniform int bytesPerDisc
    , uniform float radius
    , uniform int materialID
    , uniform int offset_center
    , uniform int offset_normal
    , uniform int offset_radius
    , uniform int offset_materialID
    , uniform int offset_colorID
    )
{
  uniform Discs *uniform self = (uniform Discs *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numDiscs);
  
  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numDiscs;
  self->materialList = (Material **)materialList;
  self->texcoord = texcoord;
  self->color = (uint8 *uniform)color;
  self->color_stride = color_stride;
  self->color_offset = color_offset;
  self->has_alpha = has_alpha;
  self->radius = radius;
  self->data = (uint8 *uniform)data;
  self->materialID = materialID;
  self->stride = bytesPerDisc;

  self->offset_center     = offset_center;
  self->offset_normal     = offset_normal;
  self->offset_radius     = offset_radius;
  self->offset_materialID = offset_materialID;
  self->offset_colorID    = offset_colorID;

  rtcSetUserData(model->embreeSceneHandle,geomID,self);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&Discs_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&Discs_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                         (uniform RTCOccludedFuncVarying)&Discs_intersect);
}