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

struct Cones {
  /*! inherit from "Geometry" class: */
  Geometry   super; 

  /*! data array that contains the cone data (possibly but not
      necessarily including the color, which could be in color);
      most offset and stride values point into here.  */
  uint8     *data;
  int32      stride;
  int        offset_center;
  int        offset_axis;
  int        offset_radius;
  int        offset_materialID;
  int        offset_colorID; 
  
  float      radius;
  float      epsilon;

  /*! list of multiple  materials, in case of per-cone materials */
  Material **materialList;

  int        materialID;

  uint8     *color;
  int        color_stride;
  int        color_offset;
  bool       has_alpha; // 4th color component is valid
 
  vec2f     *texcoord;
};

static void Cones_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  uniform Cones *uniform self = (uniform Cones *uniform)geometry;

  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR) && self->color) {
    uint32 colorID = 0;
    if (self->offset_colorID >= 0) {
      uniform uint8 *varying conePtr =
        self->data + self->stride*ray.primID;
      colorID = *((uniform uint32 *varying)(conePtr+self->offset_colorID));
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
          uniform uint8 *varying conePtr = pagePtr
            + self->stride*localPrimID;
          dg.materialID =
            *((uniform uint32 *varying)(conePtr+self->offset_materialID));
          if (self->materialList) {
            dg.material = self->materialList[dg.materialID];
          }
        }
      } else {
        uniform uint8 *varying conePtr = self->data
          + self->stride*ray.primID;
        dg.materialID =
          *((uniform uint32 *varying)(conePtr+self->offset_materialID));
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

unmasked void Cones_bounds(uniform Cones *uniform self,
                             uniform size_t primID,
                             uniform box3fa &bbox)
{
  uniform uint8 *uniform conePtr = self->data
    + self->stride*((uniform int64)primID);
  uniform bool offr = self->offset_radius >= 0;
  uniform float radius =
    offr ? *((uniform float *uniform)(conePtr+self->offset_radius)) :
    self->radius;
  uniform vec3f axis =
    *((uniform vec3f*uniform)(conePtr+self->offset_axis));
  radius = max(radius, length(axis));
  uniform vec3f center =
    *((uniform vec3f*uniform)(conePtr+self->offset_center));
  bbox = make_box3fa(center - radius, center + radius);
}

void Cones_intersect(uniform Cones *uniform self,
                       varying Ray &ray,
                       uniform size_t primID)
{
  uniform uint8 *uniform conePtr =
    self->data + self->stride*((uniform int64)primID);
  uniform float radius = self->radius;
  if (self->offset_radius >= 0) {
    radius = *((uniform float *uniform)(conePtr+self->offset_radius));
  }
  uniform vec3f center = *((uniform vec3f*uniform)(conePtr+self->offset_center));
  uniform vec3f axis = *((uniform vec3f*uniform)(conePtr+self->offset_axis));
  uniform float height = length(axis);
  uniform float angle = atan(radius / height);
  uniform float cos_angle = cos(angle);
  uniform float sin_angle = sin(angle);
  uniform float cosSqr = cos_angle * cos_angle;
  float AdD = dot(axis, ray.dir) / height;
  vec3f E = ray.org - center;
  float AdE = dot(axis, E) / height;
  float DdE = dot(ray.dir, E);
  float EdE = dot(E, E);
  float c2 = AdD*AdD - cosSqr;
  float c1 = AdD*AdE - cosSqr*DdE;
  float c0 = AdE*AdE - cosSqr*EdE;
  // Solve the quadratic. Keep only those X for which dot(A,X-V) >= 0.
  bool hit = false;
  if(abs(c2) >= 1e-9) {
    float discr = c1*c1 - c0*c2;
    if(discr < 0.f) {
      // Q(t) = 0 has no real-valued roots.  The line does not
      // intersect the double-sided cone.
    }
    else if(discr > 1e-9) {
      // Q(t) = 0 has two distinct real-valued roots.  However, one or
      // both of them might intersect the portion of the double-sided
      // cone "behind" the vertex.  We are interested only in those
      // intersections "in front" of the vertex.
      float root = sqrt(discr);
      float t = (-c1 - root) / c2;
      E = ray.org + t * ray.dir - center;
      float ddot = dot(E, axis);
      if(ddot > 0.f && ddot < height*height && t > ray.t0 && t < ray.t) {
        ray.t = t;
        hit = true;
      }
      t = (-c1 + root) / c2;
      vec3f E2 = ray.org + t * ray.dir - center;
      ddot = dot(E2, axis);
      if(ddot > 0.f && ddot < height*height && t > ray.t0 && t < ray.t) {
        ray.t = t;
        E = E2;
        hit = true;
      }
    }
    else {
      // One repeated real root (line is tangent to the cone).
      float t = -(c1/c2);
      if(t > ray.t0 && t < ray.t) {
        E = ray.org + t * ray.dir - center;
        if(dot(E, axis) > 0.f) {
          ray.t = t;
          hit = true;
        }
      }
    }
  }
  else if(abs(c1) >= self->epsilon) {
    // c2 = 0, c1 != 0 (D is a direction vector on the cone boundary)
    float t = - (0.5f*c0/c1);
    if(t > ray.t0 && t < ray.t) {
      E = ray.org + t * ray.dir - center;
      if(dot(E, axis) > 0.f) {
        ray.t = t;
        hit = true;
      }
    }
  }
  else if(abs(c0) >= self->epsilon) {
      // c2 = c1 = 0, c0 != 0
  }
  else if(DdE > ray.t0 && DdE < ray.t) {
    // c2 = c1 = c0 = 0, cone contains ray V+t*D where V is cone vertex
    // and D is the line direction.
    ray.t = DdE;
    hit = true;
  }
  if(hit) {
    ray.primID = primID;
    ray.geomID = self->super.geomID;
    vec3f axis_var = axis;
    ray.Ng = safe_normalize(cross(E, cross(E, axis_var)));
  }
}

export void *uniform Cones_create(void *uniform cppEquivalent)
{
  uniform Cones *uniform self = uniform new uniform Cones;
  Geometry_Constructor(&self->super,cppEquivalent,
                       Cones_postIntersect,
                       NULL,0,NULL);
  return self;
}

export void ConesGeometry_set(void  *uniform _self
    , void *uniform _model
    , void *uniform data
    , void *uniform materialList
    , vec2f *uniform texcoord
    , void *uniform color
    , uniform int color_offset
    , uniform int color_stride
    , uniform bool has_alpha
    , uniform int numCones
    , uniform int bytesPerCone
    , uniform float radius
    , uniform int materialID
    , uniform int offset_center
    , uniform int offset_axis
    , uniform int offset_radius
    , uniform int offset_materialID
    , uniform int offset_colorID
    )
{
  uniform Cones *uniform self = (uniform Cones *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numCones);
  
  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numCones;
  self->materialList = (Material **)materialList;
  self->texcoord = texcoord;
  self->color = (uint8 *uniform)color;
  self->color_stride = color_stride;
  self->color_offset = color_offset;
  self->has_alpha = has_alpha;
  self->radius = radius;
  self->data = (uint8 *uniform)data;
  self->materialID = materialID;
  self->stride = bytesPerCone;

  self->offset_center     = offset_center;
  self->offset_axis       = offset_axis;
  self->offset_radius     = offset_radius;
  self->offset_materialID = offset_materialID;
  self->offset_colorID    = offset_colorID;

  self->epsilon = log(self->radius);
  if (self->epsilon < 0.f)
    self->epsilon = -1.f/self->epsilon;

  rtcSetUserData(model->embreeSceneHandle,geomID,self);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&Cones_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&Cones_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                         (uniform RTCOccludedFuncVarying)&Cones_intersect);
}