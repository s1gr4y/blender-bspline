/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

/** \file
 * \ingroup bke
 *
 * Common field utilities and field definitions for geometry components.
 */

#include "BKE_attribute.h"
#include "BKE_geometry_set.hh"

#include "FN_field.hh"

struct Mesh;
struct PointCloud;

namespace blender::bke {

class CurvesGeometry;
class GeometryFieldInput;

class MeshFieldContext : public fn::FieldContext {
 private:
  const Mesh &mesh_;
  const eAttrDomain domain_;

 public:
  MeshFieldContext(const Mesh &mesh, const eAttrDomain domain);
  const Mesh &mesh() const
  {
    return mesh_;
  }

  eAttrDomain domain() const
  {
    return domain_;
  }
};

class CurvesFieldContext : public fn::FieldContext {
 private:
  const CurvesGeometry &curves_;
  const eAttrDomain domain_;

 public:
  CurvesFieldContext(const CurvesGeometry &curves, const eAttrDomain domain);

  const CurvesGeometry &curves() const
  {
    return curves_;
  }

  eAttrDomain domain() const
  {
    return domain_;
  }
};

class PointCloudFieldContext : public fn::FieldContext {
 private:
  const PointCloud &pointcloud_;

 public:
  PointCloudFieldContext(const PointCloud &pointcloud) : pointcloud_(pointcloud)
  {
  }

  const PointCloud &pointcloud() const
  {
    return pointcloud_;
  }
};

class InstancesFieldContext : public fn::FieldContext {
 private:
  const InstancesComponent &instances_;

 public:
  InstancesFieldContext(const InstancesComponent &instances) : instances_(instances)
  {
  }

  const InstancesComponent &instances() const
  {
    return instances_;
  }
};

/**
 * A field context that can represent meshes, curves, point clouds, or instances,
 * used for field inputs that can work for multiple geometry types.
 */
class GeometryFieldContext : public fn::FieldContext {
 private:
  /**
   * Store the geometry as a void pointer instead of a #GeometryComponent to allow referencing data
   * that doesn't correspond directly to a geometry component type, in this case #CurvesGeometry
   * instead of #Curves.
   */
  const void *geometry_;
  const GeometryComponentType type_;
  const eAttrDomain domain_;

  friend GeometryFieldInput;

 public:
  GeometryFieldContext(const GeometryComponent &component, eAttrDomain domain);
  GeometryFieldContext(const void *geometry, GeometryComponentType type, eAttrDomain domain);

  const void *geometry() const
  {
    return geometry_;
  }

  GeometryComponentType type() const
  {
    return type_;
  }

  eAttrDomain domain() const
  {
    return domain_;
  }

  std::optional<AttributeAccessor> attributes() const;
  const Mesh *mesh() const;
  const CurvesGeometry *curves() const;
  const PointCloud *pointcloud() const;
  const InstancesComponent *instances() const;

 private:
  GeometryFieldContext(const Mesh &mesh, eAttrDomain domain);
  GeometryFieldContext(const CurvesGeometry &curves, eAttrDomain domain);
  GeometryFieldContext(const PointCloud &points);
  GeometryFieldContext(const InstancesComponent &instances);
};

class GeometryFieldInput : public fn::FieldInput {
 public:
  using fn::FieldInput::FieldInput;
  GVArray get_varray_for_context(const fn::FieldContext &context,
                                 IndexMask mask,
                                 ResourceScope &scope) const override;
  virtual GVArray get_varray_for_context(const GeometryFieldContext &context,
                                         IndexMask mask) const = 0;
};

class MeshFieldInput : public fn::FieldInput {
 public:
  using fn::FieldInput::FieldInput;
  GVArray get_varray_for_context(const fn::FieldContext &context,
                                 IndexMask mask,
                                 ResourceScope &scope) const override;
  virtual GVArray get_varray_for_context(const Mesh &mesh,
                                         eAttrDomain domain,
                                         IndexMask mask) const = 0;
};

class CurvesFieldInput : public fn::FieldInput {
 public:
  using fn::FieldInput::FieldInput;
  GVArray get_varray_for_context(const fn::FieldContext &context,
                                 IndexMask mask,
                                 ResourceScope &scope) const override;
  virtual GVArray get_varray_for_context(const CurvesGeometry &curves,
                                         eAttrDomain domain,
                                         IndexMask mask) const = 0;
};

class PointCloudFieldInput : public fn::FieldInput {
 public:
  using fn::FieldInput::FieldInput;
  GVArray get_varray_for_context(const fn::FieldContext &context,
                                 IndexMask mask,
                                 ResourceScope &scope) const override;
  virtual GVArray get_varray_for_context(const PointCloud &pointcloud, IndexMask mask) const = 0;
};

class InstancesFieldInput : public fn::FieldInput {
 public:
  using fn::FieldInput::FieldInput;
  GVArray get_varray_for_context(const fn::FieldContext &context,
                                 IndexMask mask,
                                 ResourceScope &scope) const override;
  virtual GVArray get_varray_for_context(const InstancesComponent &instances,
                                         IndexMask mask) const = 0;
};

class AttributeFieldInput : public GeometryFieldInput {
 private:
  std::string name_;

 public:
  AttributeFieldInput(std::string name, const CPPType &type)
      : GeometryFieldInput(type, name), name_(std::move(name))
  {
    category_ = Category::NamedAttribute;
  }

  template<typename T> static fn::Field<T> Create(std::string name)
  {
    const CPPType &type = CPPType::get<T>();
    auto field_input = std::make_shared<AttributeFieldInput>(std::move(name), type);
    return fn::Field<T>{field_input};
  }

  StringRefNull attribute_name() const
  {
    return name_;
  }

  GVArray get_varray_for_context(const GeometryFieldContext &context,
                                 IndexMask mask) const override;

  std::string socket_inspection_name() const override;

  uint64_t hash() const override;
  bool is_equal_to(const fn::FieldNode &other) const override;
};

class IDAttributeFieldInput : public GeometryFieldInput {
 public:
  IDAttributeFieldInput() : GeometryFieldInput(CPPType::get<int>())
  {
    category_ = Category::Generated;
  }

  GVArray get_varray_for_context(const GeometryFieldContext &context,
                                 IndexMask mask) const override;

  std::string socket_inspection_name() const override;

  uint64_t hash() const override;
  bool is_equal_to(const fn::FieldNode &other) const override;
};

VArray<float3> curve_normals_varray(const CurvesGeometry &curves, const eAttrDomain domain);

VArray<float3> mesh_normals_varray(const Mesh &mesh, const IndexMask mask, eAttrDomain domain);

class NormalFieldInput : public GeometryFieldInput {
 public:
  NormalFieldInput() : GeometryFieldInput(CPPType::get<float3>())
  {
    category_ = Category::Generated;
  }

  GVArray get_varray_for_context(const GeometryFieldContext &context,
                                 IndexMask mask) const override;

  std::string socket_inspection_name() const override;

  uint64_t hash() const override;
  bool is_equal_to(const fn::FieldNode &other) const override;
};

class AnonymousAttributeFieldInput : public GeometryFieldInput {
 private:
  /**
   * A strong reference is required to make sure that the referenced attribute is not removed
   * automatically.
   */
  StrongAnonymousAttributeID anonymous_id_;
  std::string producer_name_;

 public:
  AnonymousAttributeFieldInput(StrongAnonymousAttributeID anonymous_id,
                               const CPPType &type,
                               std::string producer_name)
      : GeometryFieldInput(type, anonymous_id.debug_name()),
        anonymous_id_(std::move(anonymous_id)),
        producer_name_(producer_name)
  {
    category_ = Category::AnonymousAttribute;
  }

  template<typename T>
  static fn::Field<T> Create(StrongAnonymousAttributeID anonymous_id, std::string producer_name)
  {
    const CPPType &type = CPPType::get<T>();
    auto field_input = std::make_shared<AnonymousAttributeFieldInput>(
        std::move(anonymous_id), type, std::move(producer_name));
    return fn::Field<T>{field_input};
  }

  GVArray get_varray_for_context(const GeometryFieldContext &context,
                                 IndexMask mask) const override;

  std::string socket_inspection_name() const override;

  uint64_t hash() const override;
  bool is_equal_to(const fn::FieldNode &other) const override;
};

class CurveLengthFieldInput final : public CurvesFieldInput {
 public:
  CurveLengthFieldInput();
  GVArray get_varray_for_context(const CurvesGeometry &curves,
                                 eAttrDomain domain,
                                 IndexMask mask) const final;
  uint64_t hash() const override;
  bool is_equal_to(const fn::FieldNode &other) const override;
};

}  // namespace blender::bke
