/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_UTIL_OBJECT_H_
#define EL_UTIL_OBJECT_H_

namespace el {
namespace util {

typedef void* tb_type_id_t;

// Implements custom RTTI so we can get type safe casts, and the class name at
// runtime.
// Each subclass is expected to define TBOBJECT_SUBCLASS to get the necessary
// implementations, instead of implementing those manually.
class TypedObject {
 public:
  virtual ~TypedObject() = default;

  // A static template method that returns a unique id for each type.
  template <class T>
  static tb_type_id_t GetTypeId() {
    static char type_id;
    return &type_id;
  }

  // Returns true if the class or the base class matches the type id.
  virtual bool IsOfTypeId(const tb_type_id_t type_id) const {
    return type_id == GetTypeId<TypedObject>();
  }

  // Returns this object as the given type or nullptr if it's not that type.
  template <class T>
  T* SafeCastTo() const {
    return (T*)(IsOfTypeId(GetTypeId<T>()) ? this : nullptr);
  }

  // Return true if this object can safely be casted to the given type.
  template <class T>
  bool IsOfType() const {
    return SafeCastTo<T>() ? true : false;
  }

  // Gets the classname of the object.
  virtual const char* GetTypeName() const { return "TypedObject"; }
};

// Returns the given object as the given type, or nullptr if it's not that type
// or if the object is nullptr.
template <class T>
T* SafeCast(TypedObject* obj) {
  return obj ? obj->SafeCastTo<T>() : nullptr;
}

// Returns the given object as the given type, or nullptr if it's not that type
// or if the object is nullptr.
template <class T>
const T* SafeCast(const TypedObject* obj) {
  return obj ? obj->SafeCastTo<T>() : nullptr;
}

// Implements the methods for safe typecasting without requiring RTTI.
#define TBOBJECT_SUBCLASS(clazz, baseclazz)                              \
  const char* GetTypeName() const override { return #clazz; }           \
  bool IsOfTypeId(const el::util::tb_type_id_t type_id) const override { \
    return el::util::TypedObject::GetTypeId<clazz>() == type_id          \
               ? true                                                    \
               : baseclazz::IsOfTypeId(type_id);                         \
  }

}  // namespace util
}  // namespace el

#endif  // EL_UTIL_OBJECT_H_
