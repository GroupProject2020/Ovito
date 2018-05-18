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
#include "ospray/OSPDataType.h"
#include "safe_gather.ih"

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

  int        materialID;

  uint8     *color;
  int        color_stride;
  int        color_offset;
  int        color_format; 
  
  vec2f     *texcoord;
  bool       huge_mesh;
};

inline int32 Cones_getMaterialID(const uniform Cones *uniform self, int32 primID)
{
  int32 materialID = -1;
  if (self->offset_materialID >= 0) {
    const uniform int32 primsPerPage = (1024*1024*64);
    if (any(primID >= primsPerPage )) {
      const int primPageID  = primID / primsPerPage;
      const int localPrimID = primID % primsPerPage;
      foreach_unique (primPage in primPageID) {
        uniform uint8 *uniform pagePtr = self->data
          + (((int64)primPage)
             * primsPerPage
             * self->stride);
        uniform uint8 *varying conePtr = pagePtr + self->stride*localPrimID;
        materialID = *((uniform uint32 *varying)(conePtr+self->offset_materialID));
      }
    } else {
      uniform uint8 *varying conePtr = self->data + self->stride*primID;
      materialID = *((uniform uint32 *varying)(conePtr+self->offset_materialID));
    }
  } else {
    materialID = self->materialID;
  }
  return materialID;
}

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
      uniform uint8 *varying conePtr = self->data + self->stride*ray.primID;
      colorID = *((uniform uint32 *varying)(conePtr+self->offset_colorID));
    } else
      colorID = ray.primID;
    if (self->color_format == OSP_FLOAT4) {
      dg.color = gather_byte_stride_vec4f(self->huge_mesh,
                                          self->color + self->color_offset,
                                          self->color_stride,
                                          colorID);
    } else if (self->color_format == OSP_FLOAT3
        || self->color_format == OSP_FLOAT3A)
    {
      vec3f c = gather_byte_stride_vec3f(self->huge_mesh,
                                         self->color + self->color_offset,
                                         self->color_stride,
                                         colorID);
      dg.color = make_vec4f(c.x, c.y, c.z, 1.f);
    } else if (self->color_format == OSP_UCHAR4) {
      const uint32 c = gather_stride_uint32(self->huge_mesh,
                                            self->color + self->color_offset,
                                            self->color_stride,
                                            colorID);
      dg.color.x = (c & 0x000000ff) / 255.0;
      dg.color.y = ((c & 0x0000ff00) >> 8) / 255.0;
      dg.color.z = ((c & 0x00ff0000) >> 16) / 255.0;
      dg.color.w = ((c & 0xff000000) >> 24) / 255.0;
    }
  }

  if (flags & DG_TEXCOORD && self->texcoord)
    dg.st = self->texcoord[ray.primID];
  else
    dg.st = make_vec2f(0.0f);

  if (flags & DG_MATERIALID) {
    dg.materialID = Cones_getMaterialID(self, ray.primID);
  }
}

unmasked void Cones_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform Cones *uniform self = (uniform Cones *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;
  uniform uint8 *uniform conePtr = self->data + self->stride*((uniform int64)primID);
  uniform bool offr = self->offset_radius >= 0;
  uniform float radius =
    offr ? *((uniform float *uniform)(conePtr+self->offset_radius)) :
    self->radius;
  uniform vec3f axis =
    *((uniform vec3f*uniform)(conePtr+self->offset_axis));
  radius = max(radius, length(axis));
  uniform vec3f center =
    *((uniform vec3f*uniform)(conePtr+self->offset_center));
  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(center - radius, center + radius);
}

void Cones_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args, const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform Cones *uniform self = (uniform Cones *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer to a varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;

  uniform uint8 *uniform conePtr = self->data + self->stride*((uniform int64)primID);
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
  float AdD = dot(axis, ray->dir) / height;
  vec3f E = ray->org - center;
  float AdE = dot(axis, E) / height;
  float DdE = dot(ray->dir, E);
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
      E = ray->org + t * ray->dir - center;
      float ddot = dot(E, axis);
      if(ddot > 0.f && ddot < height*height && t > ray->t0 && t < ray->t) {
        ray->t = t;
        hit = true;
      }
      t = (-c1 + root) / c2;
      vec3f E2 = ray->org + t * ray->dir - center;
      ddot = dot(E2, axis);
      if(ddot > 0.f && ddot < height*height && t > ray->t0 && t < ray->t) {
        ray->t = t;
        E = E2;
        hit = true;
      }
    }
    else {
      // One repeated real root (line is tangent to the cone).
      float t = -(c1/c2);
      if(t > ray->t0 && t < ray->t) {
        E = ray->org + t * ray->dir - center;
        if(dot(E, axis) > 0.f) {
          ray->t = t;
          hit = true;
        }
      }
    }
  }
  else if(abs(c1) >= self->epsilon) {
    // c2 = 0, c1 != 0 (D is a direction vector on the cone boundary)
    float t = - (0.5f*c0/c1);
    if(t > ray->t0 && t < ray->t) {
      E = ray->org + t * ray->dir - center;
      if(dot(E, axis) > 0.f) {
        ray->t = t;
        hit = true;
      }
    }
  }
  else if(abs(c0) >= self->epsilon) {
      // c2 = c1 = 0, c0 != 0
  }
  else if(DdE > ray->t0 && DdE < ray->t) {
    // c2 = c1 = c0 = 0, cone contains ray V+t*D where V is cone vertex
    // and D is the line direction.
    ray->t = DdE;
    hit = true;
  }
  if(hit) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->primID = primID;
      ray->geomID = self->super.geomID;
      ray->instID = args->context->instID[0];
      vec3f axis_var = axis;
      ray->Ng = safe_normalize(cross(E, cross(E, axis_var)));
    }
  }
}

unmasked void Cones_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Cones_intersect_kernel(args,false);
}

unmasked void Cones_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Cones_intersect_kernel(args,true);
}

int32 Cones_getMaterialID(const Geometry *uniform const _self, const int32 primID)
{
  const Cones *const uniform self = (const Cones *uniform)_self;
  return Cones_getMaterialID(self, primID);
}

export void *uniform Cones_create(void *uniform cppEquivalent)
{
  uniform Cones *uniform self = uniform new uniform Cones;
  Geometry_Constructor(&self->super,cppEquivalent,
                       Cones_postIntersect,
                       NULL,NULL,0,NULL);
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
    , uniform int color_format
    , uniform int numCones
    , uniform int bytesPerCone
    , uniform float radius
    , uniform int materialID
    , uniform int offset_center
    , uniform int offset_axis
    , uniform int offset_radius
    , uniform int offset_materialID
    , uniform int offset_colorID
    , uniform bool huge_mesh
    )
{
  uniform Cones *uniform self = (uniform Cones *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);
  
  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numCones;
  self->super.materialList = (Material **)materialList;
  self->texcoord = texcoord;
  self->color = (uint8 *uniform)color;
  self->color_stride = color_stride;
  self->color_offset = color_offset;
  self->color_format = color_format;
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

  self->huge_mesh = huge_mesh;

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom, numCones);
  rtcSetGeometryBoundsFunction(geom,(uniform RTCBoundsFunction)&Cones_bounds, self);
  rtcSetGeometryIntersectFunction(geom,(uniform RTCIntersectFunctionN)&Cones_intersect);
  rtcSetGeometryOccludedFunction(geom,(uniform RTCOccludedFunctionN)&Cones_occluded);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);
}