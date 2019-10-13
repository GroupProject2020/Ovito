////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

// ospray
#include "math/vec.ih"
#include "math/box.ih"
#include "common/Ray.ih"
#include "common/Model.ih"
#include "geometry/Geometry.ih"
#include "math/sampling.ih"
#include "ospray/OSPDataType.h"
#include "safe_gather.ih"

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
  int        materialID;

  uint8     *color;
  int        color_stride;
  int        color_offset;
  int        color_format;

  vec2f     *texcoord;
  bool       huge_mesh;
};


inline int32 Discs_getMaterialID(const uniform Discs *uniform self, int32 primID)
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
        uniform uint8 *varying discPtr = pagePtr + self->stride*localPrimID;
        materialID = *((uniform uint32 *varying)(discPtr+self->offset_materialID));
      }
    } else {
      uniform uint8 *varying discPtr = self->data + self->stride*primID;
      materialID = *((uniform uint32 *varying)(discPtr+self->offset_materialID));
    }
  } else {
    materialID = self->materialID;
  }
  return materialID;
}

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
      uniform uint8 *varying discPtr = self->data + self->stride*ray.primID;
      colorID = *((uniform uint32 *varying)(discPtr+self->offset_colorID));
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
    dg.materialID = Discs_getMaterialID(self, ray.primID);
  }
}

unmasked void Discs_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform Discs *uniform self = (uniform Discs *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;
  uniform uint8 *uniform discPtr = self->data + self->stride*((uniform int64)primID);
  uniform bool offr = self->offset_radius >= 0;
  uniform float radius =
    offr ? *((uniform float *uniform)(discPtr+self->offset_radius)) :
    self->radius;
  uniform vec3f center = *((uniform vec3f*uniform)(discPtr+self->offset_center));
  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(center - radius, center + radius);
}

void Discs_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args, const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform Discs *uniform self = (uniform Discs *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer to a varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;

  uniform uint8 *uniform discPtr = self->data + self->stride*((uniform int64)primID);
  uniform float radius = self->radius;
  if(self->offset_radius >= 0) {
    radius = *((uniform float *uniform)(discPtr+self->offset_radius));
  }
  uniform vec3f center = *((uniform vec3f*uniform)(discPtr+self->offset_center));
  uniform vec3f normal = *((uniform vec3f*uniform)(discPtr+self->offset_normal));

  const float d = -dot(center, normal);
  float t = -(d + dot(normal, ray->org));
  const float td = dot(normal, ray->dir);
  if(td == 0.f) return;

  t = t / td;
  if(t > ray->t0 && t < ray->t) {
    const vec3f hitpnt = ray->org + t * ray->dir - center;
    if(dot(hitpnt,hitpnt) < radius*radius) {
      if (isOcclusionTest) {
        ray->t = neg_inf;
      } else {
        ray->t = t;
        ray->primID = primID;
        ray->geomID = self->super.geomID;
        ray->instID = args->context->instID[0];
        ray->Ng = normal;
      }
    }
  }
}

unmasked void Discs_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Discs_intersect_kernel(args,false);
}

unmasked void Discs_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Discs_intersect_kernel(args,true);
}

int32 Discs_getMaterialID(const Geometry *uniform const _self, const int32 primID)
{
  const Discs *const uniform self = (const Discs *uniform)_self;
  return Discs_getMaterialID(self, primID);
}

export void *uniform Discs_create(void *uniform cppEquivalent)
{
  uniform Discs *uniform self = uniform new uniform Discs;
  Geometry_Constructor(&self->super,cppEquivalent,
                       Discs_postIntersect,
                       NULL,NULL,0,NULL);
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
    , uniform int color_format
    , uniform int numDiscs
    , uniform int bytesPerDisc
    , uniform float radius
    , uniform int materialID
    , uniform int offset_center
    , uniform int offset_normal
    , uniform int offset_radius
    , uniform int offset_materialID
    , uniform int offset_colorID
    , uniform bool huge_mesh
    )
{
  uniform Discs *uniform self = (uniform Discs *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);

  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numDiscs;
  self->super.materialList = (Material **)materialList;
  self->texcoord = texcoord;
  self->color = (uint8 *uniform)color;
  self->color_stride = color_stride;
  self->color_offset = color_offset;
  self->color_format = color_format;
  self->radius = radius;
  self->data = (uint8 *uniform)data;
  self->materialID = materialID;
  self->stride = bytesPerDisc;

  self->offset_center     = offset_center;
  self->offset_normal     = offset_normal;
  self->offset_radius     = offset_radius;
  self->offset_materialID = offset_materialID;
  self->offset_colorID    = offset_colorID;

  self->huge_mesh = huge_mesh;

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom, numDiscs);
  rtcSetGeometryBoundsFunction(geom,(uniform RTCBoundsFunction)&Discs_bounds, self);
  rtcSetGeometryIntersectFunction(geom,(uniform RTCIntersectFunctionN)&Discs_intersect);
  rtcSetGeometryOccludedFunction(geom,(uniform RTCOccludedFunctionN)&Discs_occluded);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);
}