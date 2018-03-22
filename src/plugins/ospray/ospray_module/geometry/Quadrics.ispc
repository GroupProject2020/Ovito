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

struct Quadrics {
  /*! inherit from "Geometry" class: */
  Geometry   super; 

  /*! data array that contains the quadric data (possibly but not
      necessarily including the color, which could be in color);
      most offset and stride values point into here.  */
  uint8     *data;
  int32      stride;
  int        offset_center;
  int        offset_coeff;
  int        offset_radius;
  int        offset_materialID;
  int        offset_colorID; 
  
  /*! list of multiple  materials, in case of per-disc materials */
  Material **materialList;

  int        materialID;

  uint8     *color;
  int        color_stride;
  int        color_offset;
  bool       has_alpha; // 4th color component is valid
 
  vec2f     *texcoord;
};

static void Quadrics_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  uniform Quadrics *uniform self = (uniform Quadrics *uniform)geometry;

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

unmasked void Quadrics_bounds(uniform Quadrics *uniform self,
                             uniform size_t primID,
                             uniform box3fa &bbox)
{
  uniform uint8 *uniform quadricPtr = self->data + self->stride*((uniform int64)primID);
  uniform float radius = *((uniform float *uniform)(quadricPtr+self->offset_radius));
  uniform vec3f center = *((uniform vec3f *uniform)(quadricPtr+self->offset_center)); 
  bbox = make_box3fa(center - radius, center + radius);
}

struct quadmatrix {
  float a; float b; float c;
  float d; float e; float f;
  float g; float h; float i; float j;
};

void Quadrics_intersect(uniform Quadrics *uniform self,
                       varying Ray &ray,
                       uniform size_t primID)
{
  uniform uint8 *uniform quadricPtr = self->data + self->stride*((uniform int64)primID);
  uniform float radius = *((uniform float *uniform)(quadricPtr+self->offset_radius));
  uniform vec3f center = *((uniform vec3f *uniform)(quadricPtr+self->offset_center));
  uniform quadmatrix *uniform mat = ((uniform quadmatrix *uniform)(quadricPtr+self->offset_coeff));

  uniform float epsilon = log(radius);
  if(epsilon < 0.f) epsilon = -1.f/epsilon;

  vec3f ro = ray.org - center;

  float Aq = (mat->a*(ray.dir.x * ray.dir.x)) +
        (2.0f * mat->b * ray.dir.x * ray.dir.y) +
        (2.0f * mat->c * ray.dir.x * ray.dir.z) +
        (mat->e * (ray.dir.y * ray.dir.y)) +
        (2.0f * mat->f * ray.dir.y * ray.dir.z) +
        (mat->h * (ray.dir.z * ray.dir.z));

  float Bq = 2.0f * (
        (mat->a * ro.x * ray.dir.x) +
        (mat->b * ((ro.x * ray.dir.y) + (ray.dir.x * ro.y))) +
        (mat->c * ((ro.x * ray.dir.z) + (ray.dir.x * ro.z))) +
        (mat->d * ray.dir.x) +
        (mat->e * ro.y * ray.dir.y) +
        (mat->f * ((ro.y * ray.dir.z) + (ray.dir.y * ro.z))) +
        (mat->g * ray.dir.y) +
        (mat->h * ro.z * ray.dir.z) +
        (mat->i * ray.dir.z));

  float Cq = (mat->a * (ro.x * ro.x)) +
        (2.0f * mat->b * ro.x * ro.y) +
        (2.0f * mat->c * ro.x * ro.z) +
        (2.0f * mat->d * ro.x) +
        (mat->e * (ro.y * ro.y)) +
        (2.0f * mat->f * ro.y * ro.z) +
        (2.0f * mat->g * ro.y) +
        (mat->h * (ro.z * ro.z)) +
        (2.0f * mat->i * ro.z) +
        mat->j;

  bool hit = false;
  if(Aq == 0.0f) {
    float t1 = - Cq / Bq;
    if(t1 > ray.t0 && t1 < ray.t) {
      hit = true;
      ray.t = t1;
    }
  }
  else {
    float disc = (Bq * Bq - 4.0f * Aq * Cq);
    if(disc > 0.0f) {
      disc = sqrt(disc);
      float t_out = (-Bq + disc) / (2.0 * Aq);
      float t_in = (-Bq - disc) / (2.0 * Aq);
      if(t_in > ray.t0 && t_in < ray.t) {
        hit = true;
        ray.t = t_in;
      } 
      else if (t_out > (ray.t0 + epsilon) && t_out < ray.t) {
        hit = true;
        ray.t = t_out;
      }
    }
  }  
  if(hit) {
    ray.primID = primID;
    ray.geomID = self->super.geomID;
    vec3f pnt = ray.org + ray.t * ray.dir;
    
    ray.Ng.x = (mat->a*(pnt.x - center.x) + 
	    mat->b*(pnt.y - center.y) + 
	    mat->c*(pnt.z - center.z) + mat->d);

    ray.Ng.y = (mat->b*(pnt.x - center.x) + 
	    mat->e*(pnt.y - center.y) + 
	    mat->f*(pnt.z - center.z) + mat->g);

    ray.Ng.z = (mat->c*(pnt.x - center.x) + 
	    mat->f*(pnt.y - center.y) + 
	    mat->h*(pnt.z - center.z) + mat->i);

    ray.Ng = normalize(ray.Ng);
  }
}

export void *uniform Quadrics_create(void *uniform cppEquivalent)
{
  uniform Quadrics *uniform self = uniform new uniform Quadrics;
  Geometry_Constructor(&self->super,cppEquivalent,
                       Quadrics_postIntersect,
                       NULL,0,NULL);
  return self;
}

export void QuadricsGeometry_set(void  *uniform _self
    , void *uniform _model
    , void *uniform data
    , void *uniform materialList
    , vec2f *uniform texcoord
    , void *uniform color
    , uniform int color_offset
    , uniform int color_stride
    , uniform bool has_alpha
    , uniform int numQuadrics
    , uniform int bytesPerDisc
    , uniform int materialID
    , uniform int offset_center
    , uniform int offset_coeff
    , uniform int offset_radius
    , uniform int offset_materialID
    , uniform int offset_colorID
    )
{
  uniform Quadrics *uniform self = (uniform Quadrics *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numQuadrics);
  
  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numQuadrics;
  self->materialList = (Material **)materialList;
  self->texcoord = texcoord;
  self->color = (uint8 *uniform)color;
  self->color_stride = color_stride;
  self->color_offset = color_offset;
  self->has_alpha = has_alpha;
  self->data = (uint8 *uniform)data;
  self->materialID = materialID;
  self->stride = bytesPerDisc;

  self->offset_center     = offset_center;
  self->offset_coeff      = offset_coeff;
  self->offset_radius     = offset_radius;
  self->offset_materialID = offset_materialID;
  self->offset_colorID    = offset_colorID;

  rtcSetUserData(model->embreeSceneHandle,geomID,self);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&Quadrics_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&Quadrics_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                         (uniform RTCOccludedFuncVarying)&Quadrics_intersect);
}