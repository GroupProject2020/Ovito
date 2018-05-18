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

  int        materialID;

  uint8     *color;
  int        color_stride;
  int        color_offset;
  int        color_format;
 
  vec2f     *texcoord;
  bool       huge_mesh;
};

inline int32 Quadrics_getMaterialID(const uniform Quadrics *uniform self, int32 primID)
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
        uniform uint8 *varying quadricPtr = pagePtr + self->stride*localPrimID;
        materialID = *((uniform uint32 *varying)(quadricPtr+self->offset_materialID));
      }
    } else {
      uniform uint8 *varying quadricPtr = self->data + self->stride*primID;
      materialID = *((uniform uint32 *varying)(quadricPtr+self->offset_materialID));
    }
  } else {
    materialID = self->materialID;
  }
  return materialID;
}

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
      uniform uint8 *varying qaudricPtr = self->data + self->stride*ray.primID;
      colorID = *((uniform uint32 *varying)(qaudricPtr+self->offset_colorID));
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
    dg.materialID = Quadrics_getMaterialID(self, ray.primID);
  }
}

unmasked void Quadrics_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform Quadrics *uniform self = (uniform Quadrics *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;
  uniform uint8 *uniform quadricPtr = self->data + self->stride*((uniform int64)primID);
  uniform float radius = *((uniform float *uniform)(quadricPtr+self->offset_radius));
  uniform vec3f center = *((uniform vec3f *uniform)(quadricPtr+self->offset_center)); 
  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(center - radius, center + radius);
}

struct quadmatrix {
  float a; float b; float c;
  float d; float e; float f;
  float g; float h; float i; float j;
};

void Quadrics_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args, const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform Quadrics *uniform self = (uniform Quadrics *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer to a varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;

  uniform uint8 *uniform quadricPtr = self->data + self->stride*((uniform int64)primID);
  uniform float radius = *((uniform float *uniform)(quadricPtr+self->offset_radius));
  uniform vec3f center = *((uniform vec3f *uniform)(quadricPtr+self->offset_center));
  uniform quadmatrix *uniform mat = ((uniform quadmatrix *uniform)(quadricPtr+self->offset_coeff));

  uniform float epsilon = log(radius);
  if(epsilon < 0.f) epsilon = -1.f/epsilon;

  vec3f ro = ray->org - center;

  float Aq = (mat->a*(ray->dir.x * ray->dir.x)) +
        (2.0f * mat->b * ray->dir.x * ray->dir.y) +
        (2.0f * mat->c * ray->dir.x * ray->dir.z) +
        (mat->e * (ray->dir.y * ray->dir.y)) +
        (2.0f * mat->f * ray->dir.y * ray->dir.z) +
        (mat->h * (ray->dir.z * ray->dir.z));

  float Bq = 2.0f * (
        (mat->a * ro.x * ray->dir.x) +
        (mat->b * ((ro.x * ray->dir.y) + (ray->dir.x * ro.y))) +
        (mat->c * ((ro.x * ray->dir.z) + (ray->dir.x * ro.z))) +
        (mat->d * ray->dir.x) +
        (mat->e * ro.y * ray->dir.y) +
        (mat->f * ((ro.y * ray->dir.z) + (ray->dir.y * ro.z))) +
        (mat->g * ray->dir.y) +
        (mat->h * ro.z * ray->dir.z) +
        (mat->i * ray->dir.z));

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
    if(t1 > ray->t0 && t1 < ray->t) {
      hit = true;
      ray->t = t1;
    }
  }
  else {
    float disc = (Bq * Bq - 4.0f * Aq * Cq);
    if(disc > 0.0f) {
      disc = sqrt(disc);
      float t_out = (-Bq + disc) / (2.0 * Aq);
      float t_in = (-Bq - disc) / (2.0 * Aq);
      if(t_in > ray->t0 && t_in < ray->t) {
        hit = true;
        ray->t = t_in;
      } 
      else if (t_out > (ray->t0 + epsilon) && t_out < ray->t) {
        hit = true;
        ray->t = t_out;
      }
    }
  }  
  if(hit) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->primID = primID;
      ray->geomID = self->super.geomID;
      ray->instID = args->context->instID[0];
      vec3f pnt = ray->org + ray->t * ray->dir;
      
      ray->Ng.x = (mat->a*(pnt.x - center.x) + 
        mat->b*(pnt.y - center.y) + 
        mat->c*(pnt.z - center.z) + mat->d);

      ray->Ng.y = (mat->b*(pnt.x - center.x) + 
        mat->e*(pnt.y - center.y) + 
        mat->f*(pnt.z - center.z) + mat->g);

      ray->Ng.z = (mat->c*(pnt.x - center.x) + 
        mat->f*(pnt.y - center.y) + 
        mat->h*(pnt.z - center.z) + mat->i);

      ray->Ng = normalize(ray->Ng);
    }
  }
}

unmasked void Quadrics_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Quadrics_intersect_kernel(args,false);
}

unmasked void Quadrics_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Quadrics_intersect_kernel(args,true);
}

int32 Quadrics_getMaterialID(const Geometry *uniform const _self, const int32 primID)
{
  const Quadrics *const uniform self = (const Quadrics *uniform)_self;
  return Quadrics_getMaterialID(self, primID);
}

export void *uniform Quadrics_create(void *uniform cppEquivalent)
{
  uniform Quadrics *uniform self = uniform new uniform Quadrics;
  Geometry_Constructor(&self->super,cppEquivalent,
                       Quadrics_postIntersect,
                       NULL,NULL,0,NULL);
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
    , uniform int color_format
    , uniform int numQuadrics
    , uniform int bytesPerQuadric
    , uniform int materialID
    , uniform int offset_center
    , uniform int offset_coeff
    , uniform int offset_radius
    , uniform int offset_materialID
    , uniform int offset_colorID
    , uniform bool huge_mesh
    )
{
  uniform Quadrics *uniform self = (uniform Quadrics *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);
  
  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numQuadrics;
  self->super.materialList = (Material **)materialList;
  self->texcoord = texcoord;
  self->color = (uint8 *uniform)color;
  self->color_stride = color_stride;
  self->color_offset = color_offset;
  self->color_format = color_format;
  self->data = (uint8 *uniform)data;
  self->materialID = materialID;
  self->stride = bytesPerQuadric;

  self->offset_center     = offset_center;
  self->offset_coeff      = offset_coeff;
  self->offset_radius     = offset_radius;
  self->offset_materialID = offset_materialID;
  self->offset_colorID    = offset_colorID;

  self->huge_mesh = huge_mesh;

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom, numQuadrics);
  rtcSetGeometryBoundsFunction(geom,(uniform RTCBoundsFunction)&Quadrics_bounds, self);
  rtcSetGeometryIntersectFunction(geom,(uniform RTCIntersectFunctionN)&Quadrics_intersect);
  rtcSetGeometryOccludedFunction(geom,(uniform RTCOccludedFunctionN)&Quadrics_occluded);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);
}