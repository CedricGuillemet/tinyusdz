// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

//
#include "value-types.hh"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "nonstd/expected.hpp"
#include "nonstd/optional.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "primvar.hh"
#include "tiny-variant.hh"

namespace tinyusdz {

enum class SpecType {
  Attribute,
  Connection,
  Expression,
  Mapper,
  MapperArg,
  Prim,
  PseudoRoot,
  Relationship,
  RelationshipTarget,
  Variant,
  VariantSet,
  Invalid,
};

enum class Orientation {
  RightHanded,  // 0
  LeftHanded,
  Invalid
};

enum class Visibility {
  Inherited,  // "inherited" (default)
  Invisible,  // "invisible"
  Invalid
};

enum class Purpose {
  Default,  // 0
  Render,
  Proxy,
  Guide,
  Invalid
};

enum class Kind { Model, Group, Assembly, Component, Subcomponent, Invalid };

// Attribute interpolation
enum class Interpolation {
  Constant,     // "constant"
  Uniform,      // "uniform"
  Varying,      // "varying"
  Vertex,       // "vertex"
  FaceVarying,  // "faceVarying"
  Invalid
};

enum class ListEditQual {
  ResetToExplicit,  // "unqualified"(no qualifier)
  Append,           // "append"
  Add,              // "add"
  Delete,           // "delete"
  Prepend,          // "prepend"
  Invalid
};

enum class Axis { X, Y, Z, Invalid };

// For PrimSpec
enum class Specifier {
  Def,  // 0
  Over,
  Class,
  Invalid
};

enum class Permission {
  Public,  // 0
  Private,
  Invalid
};

enum class Variability {
  Varying,  // 0
  Uniform,
  Config,
  Invalid
};

// single or triple-quoted('"""') string
struct StringData {
  std::string value;
  bool is_triple_quoted{false};

  // optional(for USDA)
  int line_row{0};
  int line_col{0};
};

// Variable class for Prim and Attribute Metadataum.
// TODO: use `value::Value` to store variable.
class MetaVariable {
 public:
  std::string type;  // Explicit name of type
  std::string name;
  bool custom{false};

  // using Array = std::vector<Variable>;
  using Object = std::map<std::string, MetaVariable>;

  value::Value value;
  // Array arr_value;
  Object obj_value;
  value::TimeSamples timeSamples;

  MetaVariable &operator=(const MetaVariable &rhs) {
    type = rhs.type;
    name = rhs.name;
    custom = rhs.custom;
    value = rhs.value;
    // arr_value = rhs.arr_value;
    obj_value = rhs.obj_value;

    return *this;
  }

  MetaVariable(const MetaVariable &rhs) {
    type = rhs.type;
    name = rhs.name;
    custom = rhs.custom;
    value = rhs.value;
    obj_value = rhs.obj_value;
  }

  static std::string type_name(const MetaVariable &v) {
    if (!v.type.empty()) {
      return v.type;
    }

    // infer type from value content
    if (v.IsObject()) {
      return "dictionary";
    } else if (v.IsTimeSamples()) {
      std::string ts_type = "TODO: TimeSample typee";
      // FIXME
#if 0
      auto ts_struct = v.as_timesamples();

      for (const TimeSampleType &item : ts_struct->values) {
        auto tname = value::type_name(item);
        if (tname != "none") {
          return tname;
        }
      }
#endif

      // ??? TimeSamples data contains all `None` values
      return ts_type;

    } else if (v.IsEmpty()) {
      return "none";
    } else {
      return v.value.type_name();
    }
  }

  // template <typename T>
  // bool is() const {
  //   return value.index() == ValueType::index_of<T>();
  // }

  // TODO
  bool IsEmpty() const { return false; }
  bool IsValue() const { return false; }
  // bool IsArray() const {
  // }

  bool IsObject() const { return obj_value.size(); }

  // TODO
  bool IsTimeSamples() const { return false; }

  // For Value
#if 0
  template <typename T>
  const nonstd::optional<T> cast() const {
    printf("cast\n");
    if (IsValue()) {
      std::cout << "type_name = " << Variable::type_name(*this) << "\n";
      const T *p = nonstd::get_if<T>(&value);
      printf("p = %p\n", static_cast<const void *>(p));
      if (p) {
        return *p;
      } else {
        return nonstd::nullopt;
      }
    }
    return nonstd::nullopt;
  }
#endif

  bool valid() const { return !IsEmpty(); }

  MetaVariable() = default;

};

using CustomDataType = std::map<std::string, MetaVariable>;

struct APISchemas
{
  // TinyUSDZ does not allow user-supplied API schema for now
  enum class APIName {
    MaterialBindingAPI, // "MaterialBindingAPI"
    SkelBindingAPI, // "SkelBindingAPI"
  };

  ListEditQual qual{ListEditQual::ResetToExplicit}; // must be 'prepend'

  // std::get<1>: instance name. For Multi-apply API Schema e.g. `material:MainMaterial` for `CollectionAPI:material:MainMaterial`
  std::vector<std::pair<APIName, std::string>> names;
};

// Metadata for Prim
struct PrimMeta {
  nonstd::optional<bool> active; // 'active'
  nonstd::optional<Kind> kind;                  // 'kind'
  nonstd::optional<CustomDataType> customData;  // `customData`

  std::map<std::string, MetaVariable> meta;  // other meta values

  nonstd::optional<APISchemas> apiSchemas; // 'apiSchemas'

  // String only metadataum.
  // TODO: Represent as `MetaVariable`?
  std::vector<StringData> stringData;

  bool authored() const { return (kind || customData || meta.size() || apiSchemas || stringData.size()); }
};

// Metadata for Attribute
struct AttrMeta {
  // frequently used items
  // nullopt = not specified in USD data
  nonstd::optional<Interpolation> interpolation;  // 'interpolation'
  nonstd::optional<uint32_t> elementSize;         // usdSkel 'elementSize'
  nonstd::optional<CustomDataType> customData;    // `customData`

  std::map<std::string, MetaVariable> meta;  // other meta values

  // String only metadataum.
  // TODO: Represent as `MetaVariable`?
  std::vector<StringData> stringData;

  bool authored() const {
    return (interpolation || elementSize || customData || meta.size() || stringData.size());
  }
};

template <typename T>
class AttribWithFallback;

///
/// Attribute with fallback(default) value
///
/// - `authored() = true` : Attribute value is authored(attribute is
/// described in USDA/USDC)
/// - `authored() = false` : Attribute value is not authored(not described
/// in USD). If you call `get()`, fallback value is returned.
///
template <typename T>
class AttribWithFallback {
 public:
  AttribWithFallback() = delete;

  ///
  /// Init with fallback value;
  ///
  AttribWithFallback(const T &fallback_) : fallback(fallback_) {}

  AttribWithFallback &operator=(const T &value) {
    attrib = value;

    // fallback Value should be already set with `AttribWithFallback(const T&
    // fallback)` constructor.

    return (*this);
  }

  //
  // FIXME: Defininig copy constructor, move constructor and  move assignment
  // operator Gives compilation error :-(. so do not define it.
  //

  // AttribWithFallback(const AttribWithFallback &rhs) {
  //   attrib = rhs.attrib;
  //   fallback = rhs.fallback;
  // }

  // AttribWithFallback &operator=(T&& value) noexcept {
  //   if (this != &value) {
  //       attrib = std::move(value.attrib);
  //       fallback = std::move(value.fallback);
  //   }
  //   return (*this);
  // }

  // AttribWithFallback(AttribWithFallback &&rhs) noexcept {
  //   if (this != &rhs) {
  //       attrib = std::move(rhs.attrib);
  //       fallback = std::move(rhs.fallback);
  //   }
  // }

  void set(const T &v) { attrib = v; }

  const T &get() const {
    if (attrib) {
      return attrib.value();
    }
    return fallback;
  }

  // value set?
  bool authored() const {
    if (attrib) {
      return true;
    }
    return false;
  }

  AttrMeta meta;

 private:
  nonstd::optional<T> attrib;
  T fallback;
};

class PrimNode;

#if 0  // TODO
class PrimRange
{
 public:
  class iterator;

  iterator begin() const {
  }
  iterator end() const {
  }

 private:
  const PrimNode *begin_;
  const PrimNode *end_;
  size_t depth_{0};
};
#endif

template <typename T>
class ListOp {
 public:
  ListOp() : is_explicit(false) {}

  void ClearAndMakeExplicit() {
    explicit_items.clear();
    added_items.clear();
    prepended_items.clear();
    appended_items.clear();
    deleted_items.clear();
    ordered_items.clear();

    is_explicit = true;
  }

  bool IsExplicit() const { return is_explicit; }
  bool HasExplicitItems() const { return explicit_items.size(); }

  bool HasAddedItems() const { return added_items.size(); }

  bool HasPrependedItems() const { return prepended_items.size(); }

  bool HasAppendedItems() const { return appended_items.size(); }

  bool HasDeletedItems() const { return deleted_items.size(); }

  bool HasOrderedItems() const { return ordered_items.size(); }

  const std::vector<T> &GetExplicitItems() const { return explicit_items; }

  const std::vector<T> &GetAddedItems() const { return added_items; }

  const std::vector<T> &GetPrependedItems() const { return prepended_items; }

  const std::vector<T> &GetAppendedItems() const { return appended_items; }

  const std::vector<T> &GetDeletedItems() const { return deleted_items; }

  const std::vector<T> &GetOrderedItems() const { return ordered_items; }

  void SetExplicitItems(const std::vector<T> &v) { explicit_items = v; }

  void SetAddedItems(const std::vector<T> &v) { added_items = v; }

  void SetPrependedItems(const std::vector<T> &v) { prepended_items = v; }

  void SetAppendedItems(const std::vector<T> &v) { appended_items = v; }

  void SetDeletedItems(const std::vector<T> &v) { deleted_items = v; }

  void SetOrderedItems(const std::vector<T> &v) { ordered_items = v; }

 private:
  bool is_explicit{false};
  std::vector<T> explicit_items;
  std::vector<T> added_items;
  std::vector<T> prepended_items;
  std::vector<T> appended_items;
  std::vector<T> deleted_items;
  std::vector<T> ordered_items;
};

struct ListOpHeader {
  enum Bits {
    IsExplicitBit = 1 << 0,
    HasExplicitItemsBit = 1 << 1,
    HasAddedItemsBit = 1 << 2,
    HasDeletedItemsBit = 1 << 3,
    HasOrderedItemsBit = 1 << 4,
    HasPrependedItemsBit = 1 << 5,
    HasAppendedItemsBit = 1 << 6
  };

  ListOpHeader() : bits(0) {}

  explicit ListOpHeader(uint8_t b) : bits(b) {}

  explicit ListOpHeader(ListOpHeader const &op) : bits(0) {
    bits |= op.IsExplicit() ? IsExplicitBit : 0;
    bits |= op.HasExplicitItems() ? HasExplicitItemsBit : 0;
    bits |= op.HasAddedItems() ? HasAddedItemsBit : 0;
    bits |= op.HasPrependedItems() ? HasPrependedItemsBit : 0;
    bits |= op.HasAppendedItems() ? HasAppendedItemsBit : 0;
    bits |= op.HasDeletedItems() ? HasDeletedItemsBit : 0;
    bits |= op.HasOrderedItems() ? HasOrderedItemsBit : 0;
  }

  bool IsExplicit() const { return bits & IsExplicitBit; }

  bool HasExplicitItems() const { return bits & HasExplicitItemsBit; }
  bool HasAddedItems() const { return bits & HasAddedItemsBit; }
  bool HasPrependedItems() const { return bits & HasPrependedItemsBit; }
  bool HasAppendedItems() const { return bits & HasAppendedItemsBit; }
  bool HasDeletedItems() const { return bits & HasDeletedItemsBit; }
  bool HasOrderedItems() const { return bits & HasOrderedItemsBit; }

  uint8_t bits;
};


///
/// Simlar to SdfPath.
///
/// We don't need the performance for USDZ, so use naiive implementation
/// to represent Path.
/// Path is something like Unix path, delimited by `/`, ':' and '.'
/// Square brackets('<', '>' is not included)
///
/// Example:
///
/// - `/muda/bora.dora` : prim_part is `/muda/bora`, prop_part is `.dora`.
/// - `bora` : Relative path
///
/// ':' is a namespce delimiter(example `input:muda`).
///
/// Limitations:
///
/// - Relational attribute path(`[` `]`. e.g. `/muda/bora[/ari].dora`) is not
/// supported.
/// - Variant chars('{' '}') is not supported.
/// - '../' is TODO
///
/// and have more limitatons.
///
class Path {
 public:
  Path() : valid(false) {}

  // `p` is split into prim_part and prop_part
  // Path(const std::string &p);
  Path(const std::string &prim, const std::string &prop);

  // : prim_part(prim), valid(true) {}
  // Path(const std::string &prim, const std::string &prop)
  //    : prim_part(prim), prop_part(prop) {}

  Path(const Path &rhs) = default;

  Path &operator=(const Path &rhs) {
    this->valid = rhs.valid;

    this->prim_part = rhs.prim_part;
    this->prop_part = rhs.prop_part;

    return (*this);
  }

  std::string full_path_name() const {
    std::string s;
    if (!valid) {
      s += "#INVALID#";
    }

    s += prim_part;
    if (prop_part.empty()) {
      return s;
    }

    s += "." + prop_part;

    return s;
  }

  std::string GetPrimPart() const { return prim_part; }

  std::string GetPropPart() const { return prop_part; }

  bool IsValid() const { return valid; }

  bool IsEmpty() { return (prim_part.empty() && prop_part.empty()); }

  static Path RootPath() { return Path("/", ""); }
  // static Path RelativePath() { return Path("."); }

  Path AppendProperty(const std::string &elem);

  Path AppendElement(const std::string &elem);

  ///
  /// Split a path to the root(common ancestor) and its siblings
  ///
  /// example:
  ///
  /// - / -> [/, Empty]
  /// - /bora -> [/bora, Empty]
  /// - /bora/dora -> [/bora, /dora]
  /// - /bora/dora/muda -> [/bora, /dora/muda]
  /// - bora -> [Empty, bora]
  /// - .muda -> [Empty, .muda]
  ///
  std::pair<Path, Path> SplitAtRoot() const;

  Path GetParentPrim() const;

  ///
  /// @returns true if a path is '/' only
  ///
  bool IsRootPath() const {
    if (!valid) {
      return false;
    }

    if ((prim_part.size() == 1) && (prim_part[0] == '/')) {
      return true;
    }

    return false;
  }

  ///
  /// @returns true if a path is root prim: e.g. '/bora'
  ///
  bool IsRootPrim() const {
    if (!valid) {
      return false;
    }

    if (IsRootPath()) {
      return false;
    }

    if ((prim_part.size() > 1) && (prim_part[0] == '/')) {
      // no other '/' except for the fist one
      if (prim_part.find_last_of('/') == 0) {
        return true;
      }
    }

    return false;
  }

  bool IsAbsolutePath() const {
    if (prim_part.size()) {
      if ((prim_part.size() > 0) && (prim_part[0] == '/')) {
        return true;
      }
    }

    return false;
  }

  bool IsRelativePath() const {
    if (prim_part.size()) {
      return !IsAbsolutePath();
    }

    return true;  // prop part only
  }

  // Strip '/'
  Path &MakeRelative() {
    if (IsAbsolutePath() && (prim_part.size() > 1)) {
      // Remove first '/'
      prim_part.erase(0, 1);
    }
    return *this;
  }

  const Path MakeRelative(Path &&rhs) {
    (*this) = std::move(rhs);

    return MakeRelative();
  }

  static const Path MakeRelative(const Path &rhs) {
    Path p = rhs;  // copy
    return p.MakeRelative();
  }

 private:
  std::string prim_part;  // e.g. /Model/MyMesh, MySphere
  std::string prop_part;  // e.g. .visibility
  bool valid{false};
};

///
/// Split Path by the delimiter(e.g. "/") then create lists.
///
class TokenizedPath {
 public:
  TokenizedPath() {}

  TokenizedPath(const Path &path) {
    std::string s = path.GetPropPart();
    if (s.empty()) {
      // ???
      return;
    }

    if (s[0] != '/') {
      // Path must start with "/"
      return;
    }

    s.erase(0, 1);

    char delimiter = '/';
    size_t pos{0};
    while ((pos = s.find(delimiter)) != std::string::npos) {
      std::string token = s.substr(0, pos);
      _tokens.push_back(token);
      s.erase(0, pos + sizeof(char));
    }

    if (!s.empty()) {
      // leaf element
      _tokens.push_back(s);
    }
  }

 private:
  std::vector<std::string> _tokens;
};

bool operator==(const Path &lhs, const Path &rhs);

class TimeCode {
  TimeCode(double tm = 0.0) : _time(tm) {}

  size_t hash() const { return std::hash<double>{}(_time); }

  double value() const { return _time; }

 private:
  double _time;
};

struct LayerOffset {
  double _offset;
  double _scale;
};

struct Payload {
  std::string _asset_path;
  Path _prim_path;
  LayerOffset _layer_offset;
};

struct Reference {
  std::string asset_path;
  Path prim_path;
  LayerOffset layer_offset;
  value::dict custom_data;
};

//
// Colum-major order(e.g. employed in OpenGL).
// For example, 12th([3][0]), 13th([3][1]), 14th([3][2]) element corresponds to
// the translation.
//
// template <typename T, size_t N>
// struct Matrix {
//  T m[N][N];
//  constexpr static uint32_t n = N;
//};

inline void Identity(value::matrix2d *mat) {
  memset(mat->m, 0, sizeof(value::matrix2d));
  for (size_t i = 0; i < 2; i++) {
    mat->m[i][i] = static_cast<double>(1);
  }
}

inline void Identity(value::matrix3d *mat) {
  memset(mat->m, 0, sizeof(value::matrix3d));
  for (size_t i = 0; i < 3; i++) {
    mat->m[i][i] = static_cast<double>(1);
  }
}

inline void Identity(value::matrix4d *mat) {
  memset(mat->m, 0, sizeof(value::matrix4d));
  for (size_t i = 0; i < 4; i++) {
    mat->m[i][i] = static_cast<double>(1);
  }
}

// ret = m x n
template <typename MTy, typename STy, size_t N>
MTy Mult(const MTy &m, const MTy &n) {
  MTy ret;
  memset(ret.m, 0, sizeof(MTy));

  for (size_t j = 0; j < N; j++) {
    for (size_t i = 0; i < N; i++) {
      STy value = static_cast<STy>(0);
      for (size_t k = 0; k < N; k++) {
        value += m.m[k][i] * n.m[j][k];
      }
      ret.m[j][i] = value;
    }
  }

  return ret;
}

// typedef uint16_t float16;
float half_to_float(value::half h);
value::half float_to_half_full(float f);

struct Extent {
  value::float3 lower{{std::numeric_limits<float>::infinity(),
                       std::numeric_limits<float>::infinity(),
                       std::numeric_limits<float>::infinity()}};

  value::float3 upper{{-std::numeric_limits<float>::infinity(),
                       -std::numeric_limits<float>::infinity(),
                       -std::numeric_limits<float>::infinity()}};

  Extent() = default;

  Extent(const value::float3 &l, const value::float3 &u) : lower(l), upper(u) {}

  bool Valid() const {
    if (lower[0] > upper[0]) return false;
    if (lower[1] > upper[1]) return false;
    if (lower[2] > upper[2]) return false;

    return std::isfinite(lower[0]) && std::isfinite(lower[1]) &&
           std::isfinite(lower[2]) && std::isfinite(upper[0]) &&
           std::isfinite(upper[1]) && std::isfinite(upper[2]);
  }

  std::array<std::array<float, 3>, 2> to_array() const {
    std::array<std::array<float, 3>, 2> ret;
    ret[0][0] = lower[0];
    ret[0][1] = lower[1];
    ret[0][2] = lower[2];
    ret[1][0] = upper[0];
    ret[1][1] = upper[1];
    ret[1][2] = upper[2];

    return ret;
  }
};

#if 0
struct ConnectionPath {
  bool is_input{false};  // true: Input connection. false: Output connection.

  Path path;  // original Path information in USD

  std::string token;  // token(or string) in USD
  int64_t index{-1};  // corresponding array index(e.g. the array index to
                      // `Scene.shaders`)
};

// struct Connection {
//   int64_t src_index{-1};
//   int64_t dest_index{-1};
// };
//
// using connection_id_map =
//     std::unordered_map<std::pair<std::string, std::string>, Connection>;
#endif

// Relation
class Relation {
 public:
  // monostate(empty(define only)), string, Path or PathVector
  // tinyusdz::variant<tinyusdz::monostate, std::string, Path,
  // std::vector<Path>> targets;

  // For some reaon, using tinyusdz::variant will cause double-free in some
  // environemt on clang, so use old-fashioned way for a while.
  enum class Type { Empty, String, Path, PathVector };

  Type type{Type::Empty};
  std::string targetString;
  Path targetPath;
  std::vector<Path> targetPathVector;

  static Relation MakeEmpty() {
    Relation r;
    r.SetEmpty();
    return r;
  }

  void SetEmpty() { type = Type::Empty; }

  void Set(const std::string &s) {
    targetString = s;
    type = Type::String;
  }

  void Set(const Path &p) {
    targetPath = p;
    type = Type::Path;
  }

  void Set(const std::vector<Path> &pv) {
    targetPathVector = pv;
    type = Type::PathVector;
  }

  bool IsEmpty() const { return type == Type::Empty; }

  bool IsString() const { return type == Type::String; }

  bool IsPath() const { return type == Type::Path; }

  bool IsPathVector() const { return type == Type::PathVector; }
};

//
// Connection is a typed version of Relation
//
template <typename T>
class Connection {
 public:
  using type = typename value::TypeTrait<T>::value_type;

  static std::string type_name() { return value::TypeTrait<T>::type_name(); }

  // Connection() = delete;
  // Connection(const T &v) : fallback(v) {}

  nonstd::optional<Path> target;
};

// Typed TimeSamples value
//
// double radius.timeSamples = { 0: 1.0, 1: None, 2: 3.0 }
//
// in .usd, are represented as
// 
// 0: (1.0, false)
// 1: (2.0, true)
// 2: (3.0, false)
//

template <typename T>
struct TypedTimeSamples {

  struct Sample {
    double t;
    T value;
    bool blocked{false};
  };

 public:

  bool empty() const {
    return _samples.empty();
  }

  // TODO: Implement.
  nonstd::optional<T> TryGet(double t = 0.0) const;
#if 0
    if (empty()) {
      return nonstd::nullopt;
    }

    if (_dirty) {
      // TODO: Sort by time
      _dirty = false;
    }

    // TODO: Fetch value then lineary interpolate value.
    return nonstd::nullopt;
  }
#endif

  void AddSample(const Sample &s) {
    _samples.push_back(s);
    _dirty = true;
  }

  void AddSample(const double t, T &v) {
    _samples.push_back({t, v, false});
    _dirty = true;
  }

  void AddBlockedSample(const double t) {
    _samples.push_back({t, T(), true});
    _dirty = true;
  }

  const std::vector<Sample> &GetSamples() const {
    return _samples;
  }

 private:
  std::vector<Sample> _samples;
  bool _dirty{false};

};

template <typename T>
struct Animatable {
  // scalar
  T value;  
  bool blocked{false};

  // timesamples
  TypedTimeSamples<T> ts;

  bool IsTimeSampled() const {
    return !ts.empty();
  }

  bool IsScalar() const {
    return ts.empty();
  }

  // Scalar
  bool IsBlocked() const {
    return blocked;
  }

#if 0  // TODO
  T Get() const { return value; }

  T Get(double t) {
    if (IsTimeSampled()) {
      // TODO: lookup value by t
      return timeSamples.Get(t);
    }
    return value;
  }
#endif

  Animatable() {}
  Animatable(const T &v) : value(v) {}
};

// PrimAttrib is a struct to hold generic attribute of a property(e.g. primvar)
struct PrimAttrib {
  std::string name;  // attrib name

  std::string type_name;  // name of attrib type(e.g. "float', "color3f")

  //ListEditQual list_edit{ListEditQual::ResetToExplicit}; // moved to Property

  Variability variability;

  // Interpolation interpolation{Interpolation::Invalid};

  AttrMeta meta;

  //
  // Qualifiers
  //
  bool uniform{false};  // `uniform`

  bool blocked{false}; // Attribute Block('None')
  primvar::PrimVar var;
};

///
/// Typed version of PrimAttrib(e.g. for `points`, `normals`, `velocities.timeSamples`
/// `inputs:st.connect`)
///
template <typename T>
class TypedAttribute {
 public:
  TypedAttribute() = default;

  explicit TypedAttribute(const T &fv) : fallback(fv) {}

  using type = typename value::TypeTrait<T>::value_type;

  static std::string type_name() { return value::TypeTrait<T>::type_name(); }

  // TODO: Use variant?
  nonstd::optional<Animatable<T>> value; // T or TimeSamples<T>
  nonstd::optional<Path> target;

  bool authored() const {
    if (define_only) {
      return true;
    }

    return (target || value);
  }

  bool set_define_only(bool onoff = true) { define_only = onoff; return define_only; }

  nonstd::optional<T> fallback;  // may have fallback
  AttrMeta meta;
  bool uniform{false};  // `uniform`
  bool define_only{false}; // Attribute must be define-only(no value or connection assigned). e.g. "float3 outputs:rgb"
  ListEditQual qual{ListEditQual::ResetToExplicit}; // default = "unqualified"
};

// Attribute or Relation/Connection. And has this property is custom or not
// (Need to lookup schema if the property is custom or not for Crate data)
class Property {
 public:
  enum class Type {
    EmptyAttrib,        // Attrib with no data.
    Attrib,             // contains actual data
    Relation,           // `rel` type
    NoTargetsRelation,  // `rel` with no targets.
    Connection,         // `.connect` suffix
  };

  PrimAttrib attrib;
  Relation rel;  // Relation(`rel`) or Connection(`.connect`)
  Type type{Type::EmptyAttrib};
  ListEditQual qual{ListEditQual::ResetToExplicit}; // List Edit qualifier

  bool has_custom{false};  // Qualified with 'custom' keyword?

  Property() = default;

  Property(bool custom) : has_custom(custom) { type = Type::EmptyAttrib; }

  Property(const PrimAttrib &a, bool custom) : attrib(a), has_custom(custom) {
    type = Type::Attrib;
  }

  Property(PrimAttrib &&a, bool custom)
      : attrib(std::move(a)), has_custom(custom) {
    type = Type::Attrib;
  }

  Property(const Relation &r, bool isConnection, bool custom)
      : rel(r), has_custom(custom) {
    if (isConnection) {
      type = Type::Connection;
    } else {
      type = Type::Relation;
    }
  }

  Property(Relation &&r, bool isConnection, bool custom)
      : rel(std::move(r)), has_custom(custom) {
    if (isConnection) {
      type = Type::Connection;
    } else {
      type = Type::Relation;
    }
  }

  bool IsAttrib() const {
    return (type == Type::EmptyAttrib) || (type == Type::Attrib);
  }
  bool IsEmpty() const {
    return (type == Type::EmptyAttrib) || (type == Type::NoTargetsRelation);
  }
  bool IsRel() const {
    return (type == Type::Relation) || (type == Type::NoTargetsRelation);
  }
  bool IsConnection() const { return type == Type::Connection; }

  nonstd::optional<Path> GetConnectionTarget() const {
    if (!IsConnection()) {
      return nonstd::nullopt;
    }

    if (rel.IsPath()) {
      return rel.targetPath;
    }

    return nonstd::nullopt;
  }

  bool HasCustom() const { return has_custom; }
};

// Orient: axis/angle expressed as a quaternion.
// NOTE: no `quath`, `matrix4f`
//using XformOpValueType =
//    tinyusdz::variant<float, value::float3, value::quatf, double,
//                      value::double3, value::quatd, value::matrix4d>;

struct XformOp {
  enum class OpType {
    // matrix
    Transform,

    // vector3
    Translate,
    Scale,

    // scalar
    RotateX,
    RotateY,
    RotateZ,

    // vector3
    RotateXYZ,
    RotateXZY,
    RotateYXZ,
    RotateYZX,
    RotateZXY,
    RotateZYX,

    // quaternion
    Orient,

    // Special token
    ResetXformStack,  // !resetXformStack!
  };

  // OpType op;
  OpType op;
  bool inverted{false};  // true when `!inverted!` prefix
  std::string
      suffix;  // may contain nested namespaces. e.g. suffix will be
               // ":blender:pivot" for "xformOp:translate:blender:pivot". Suffix
               // will be empty for "xformOp:translate"
  //XformOpValueType value_type;  
  std::string type_name;

  value::TimeSamples var;

  // TODO: Check if T is valid type.
  template <class T>
  void set_scalar(const T &v) {
    var.times.clear();
    var.values.clear();
  
    var.values.push_back(v);
    type_name = value::TypeTrait<T>::type_name();
  }

  void set_timesamples(const value::TimeSamples &v) {
    var = v;

    if (var.values.size()) {
      type_name = var.values[0].type_name();
    }
  }

  void set_timesamples(value::TimeSamples &&v) {
    var = std::move(v);
    if (var.values.size()) {
      type_name = var.values[0].type_name();
    }
  }

  bool IsTimeSamples() const {
    return (var.times.size() > 0) && (var.times.size() == var.values.size());
  }

  // Type-safe way to get concrete value.
  template <class T>
  nonstd::optional<T> get_scalar_value() const {

    if (IsTimeSamples()) {
      return nonstd::nullopt;
    }

    if (value::TypeTrait<T>::type_id == var.values[0].type_id()) {
      //return std::move(*reinterpret_cast<const T *>(var.values[0].value()));
      auto pv = linb::any_cast<const T>(&var.values[0]);
      if (pv) {
        return (*pv);
      }
      return nonstd::nullopt;
    } else if (value::TypeTrait<T>::underlying_type_id == var.values[0].underlying_type_id()) {
      // `roll` type. Can be able to cast to underlying type since the memory
      // layout does not change.
      //return *reinterpret_cast<const T *>(var.values[0].value());

      // TODO: strict type check.
      return *linb::cast<const T>(&var.values[0]);
    }
    return nonstd::nullopt;
  }

};

// Interpolator for TimeSample data
enum class TimeSampleInterpolation
{
  Nearest,  // nearest neighbor
  Linear, // lerp 
  // TODO: more to support...
};

#if 0
template <typename T>
inline T lerp(const T a, const T b, const double t) {
  return (1.0 - t) * a + t * b;
}

template <typename T>
struct TimeSamples {
  std::vector<double> times;
  std::vector<T> values;
  // TODO: Support `none`

  void Set(T value, double t) {
    times.push_back(t);
    values.push_back(value);
  }

  T Get(double t) const {
    // Linear-interpolation.
    // TODO: Support other interpolation method for example cubic.
    auto it = std::lower_bound(times.begin(), times.end(), t);
    size_t idx0 = size_t(std::max(
        int64_t(0), std::min(int64_t(times.size() - 1),
                             int64_t(std::distance(times.begin(), it - 1)))));
    size_t idx1 = size_t(std::max(
        int64_t(0), std::min(int64_t(times.size() - 1), int64_t(idx0) + 1)));

    double tl = times[idx0];
    double tu = times[idx1];

    double dt = (t - tl);
    if (std::fabs(tu - tl) < std::numeric_limits<double>::epsilon()) {
      // slope is zero.
      dt = 0.0;
    } else {
      dt /= (tu - tl);
    }

    // Just in case.
    dt = std::max(0.0, std::min(1.0, dt));

    const T &p0 = values[idx0];
    const T &p1 = values[idx1];

    const T p = lerp(p0, p1, dt);

    return p;
  }

  bool Valid() const {
    return !times.empty() && (times.size() == values.size());
  }
};
#endif


// `def` with no type.
struct Model {
  std::string name;

  int64_t parent_id{-1};  // Index to parent node

  PrimMeta meta;

  std::vector<std::pair<ListEditQual, Reference>> references;

  std::map<std::string, Property> props;
};

// Generic "class" Node
// Mostly identical to GPrim
struct Klass {
  std::string name;
  int64_t parent_id{-1};  // Index to parent node

  std::vector<std::pair<ListEditQual, Reference>> references;

  std::map<std::string, Property> props;
};

struct MaterialBindingAPI {
  Path binding;            // rel material:binding
  Path bindingCorrection;  // rel material:binding:correction
  Path bindingPreview;     // rel material:binding:preview

  // TODO: allPurpose, preview, ...
};

//
// Predefined node classes
//

// USDZ Schemas for AR
// https://developer.apple.com/documentation/arkit/usdz_schemas_for_ar/schema_definitions_for_third-party_digital_content_creation_dcc

// UsdPhysics
struct Preliminary_PhysicsGravitationalForce {
  // physics::gravitatioalForce::acceleration
  value::double3 acceleration{{0.0, -9.81, 0.0}};  // [m/s^2]
};

struct Preliminary_PhysicsMaterialAPI {
  // preliminary:physics:material:restitution
  double restitution;  // [0.0, 1.0]

  // preliminary:physics:material:friction:static
  double friction_static;

  // preliminary:physics:material:friction:dynamic
  double friction_dynamic;
};

struct Preliminary_PhysicsRigidBodyAPI {
  // preliminary:physics:rigidBody:mass
  double mass{1.0};

  // preliminary:physics:rigidBody:initiallyActive
  bool initiallyActive{true};
};

struct Preliminary_PhysicsColliderAPI {
  // preliminary::physics::collider::convexShape
  Path convexShape;
};

struct Preliminary_InfiniteColliderPlane {
  value::double3 position{{0.0, 0.0, 0.0}};
  value::double3 normal{{0.0, 0.0, 0.0}};

  Extent extent;  // [-FLT_MAX, FLT_MAX]

  Preliminary_InfiniteColliderPlane() {
    extent.lower[0] = -(std::numeric_limits<float>::max)();
    extent.lower[1] = -(std::numeric_limits<float>::max)();
    extent.lower[2] = -(std::numeric_limits<float>::max)();
    extent.upper[0] = (std::numeric_limits<float>::max)();
    extent.upper[1] = (std::numeric_limits<float>::max)();
    extent.upper[2] = (std::numeric_limits<float>::max)();
  }
};

// UsdInteractive
struct Preliminary_AnchoringAPI {
  // preliminary:anchoring:type
  std::string type;  // "plane", "image", "face", "none";

  std::string alignment;  // "horizontal", "vertical", "any";

  Path referenceImage;
};

struct Preliminary_ReferenceImage {
  int64_t image_id{-1};  // asset image

  double physicalWidth{0.0};
};

struct Preliminary_Behavior {
  Path triggers;
  Path actions;
  bool exclusive{false};
};

struct Preliminary_Trigger {
  // uniform token info:id
  std::string info;  // Store decoded string from token id
};

struct Preliminary_Action {
  // uniform token info:id
  std::string info;  // Store decoded string from token id

  std::string multiplePerformOperation{
      "ignore"};  // ["ignore", "allow", "stop"]
};

struct Preliminary_Text {
  std::string content;
  std::vector<std::string> font;  // An array of font names

  float pointSize{144.0f};
  float width;
  float height;
  float depth{0.0f};

  std::string wrapMode{"flowing"};  // ["singleLine", "hardBreaks", "flowing"]
  std::string horizontalAlignmment{
      "center"};  // ["left", "center", "right", "justified"]
  std::string verticalAlignmment{
      "middle"};  // ["top", "middle", "lowerMiddle", "baseline", "bottom"]
};

// Simple volume class.
// Currently this is just an placeholder. Not implemented.

struct OpenVDBAsset {
  std::string fieldDataType{"float"};
  std::string fieldName{"density"};
  std::string filePath;  // asset
};

// MagicaVoxel Vox
struct VoxAsset {
  std::string fieldDataType{"float"};
  std::string fieldName{"density"};
  std::string filePath;  // asset
};

struct Volume {
  OpenVDBAsset vdb;
  VoxAsset vox;
};

// `Scope` is uncommon in graphics community, its something like `Group`.
// From USD doc: Scope is the simplest grouping primitive, and does not carry
// the baggage of transformability.
struct Scope {
  std::string name;

  int64_t parent_id{-1};

  PrimMeta meta;

  Animatable<Visibility> visibility{Visibility::Inherited};
  Purpose purpose{Purpose::Default};

  std::map<std::string, Property> props;
};

Interpolation InterpolationFromString(const std::string &v);
Orientation OrientationFromString(const std::string &v);

namespace value {

#include "define-type-trait.inc"

DEFINE_TYPE_TRAIT(Reference, "ref", TYPE_ID_REFERENCE, 1);
DEFINE_TYPE_TRAIT(Specifier, "specifier", TYPE_ID_SPECIFIER, 1);
DEFINE_TYPE_TRAIT(Permission, "permission", TYPE_ID_PERMISSION, 1);
DEFINE_TYPE_TRAIT(Variability, "variability", TYPE_ID_VARIABILITY, 1);

DEFINE_TYPE_TRAIT(Payload, "payload", TYPE_ID_PAYLOAD, 1);
DEFINE_TYPE_TRAIT(LayerOffset, "LayerOffset", TYPE_ID_LAYER_OFFSET, 1);

DEFINE_TYPE_TRAIT(ListOp<value::token>, "ListOpToken", TYPE_ID_LIST_OP_TOKEN,
                  1);
DEFINE_TYPE_TRAIT(ListOp<std::string>, "ListOpString", TYPE_ID_LIST_OP_STRING,
                  1);
DEFINE_TYPE_TRAIT(ListOp<Path>, "ListOpPath", TYPE_ID_LIST_OP_PATH, 1);
DEFINE_TYPE_TRAIT(ListOp<Reference>, "ListOpReference",
                  TYPE_ID_LIST_OP_REFERENCE, 1);
DEFINE_TYPE_TRAIT(ListOp<int32_t>, "ListOpInt", TYPE_ID_LIST_OP_INT, 1);
DEFINE_TYPE_TRAIT(ListOp<uint32_t>, "ListOpUInt", TYPE_ID_LIST_OP_UINT, 1);
DEFINE_TYPE_TRAIT(ListOp<int64_t>, "ListOpInt64", TYPE_ID_LIST_OP_INT64, 1);
DEFINE_TYPE_TRAIT(ListOp<uint64_t>, "ListOpUInt64", TYPE_ID_LIST_OP_UINT64, 1);
DEFINE_TYPE_TRAIT(ListOp<Payload>, "ListOpPayload", TYPE_ID_LIST_OP_PAYLOAD, 1);

DEFINE_TYPE_TRAIT(Path, "Path", TYPE_ID_PATH, 1);
DEFINE_TYPE_TRAIT(Relation, "Relationship", TYPE_ID_RELATIONSHIP, 1);
// TODO(syoyo): Define PathVector as 1D array?
DEFINE_TYPE_TRAIT(std::vector<Path>, "PathVector", TYPE_ID_PATH_VECTOR, 1);

DEFINE_TYPE_TRAIT(std::vector<value::token>, "token[]",
                  TYPE_ID_TOKEN_VECTOR, 1);

DEFINE_TYPE_TRAIT(value::TimeSamples, "TimeSamples", TYPE_ID_TIMESAMPLES, 1);

DEFINE_TYPE_TRAIT(Model, "Model", TYPE_ID_MODEL, 1);
DEFINE_TYPE_TRAIT(Scope, "Scope", TYPE_ID_SCOPE, 1);

DEFINE_TYPE_TRAIT(StringData, "String", TYPE_ID_SCOPE, 1);

DEFINE_TYPE_TRAIT(CustomDataType, "customData", TYPE_ID_CUSTOMDATA, 1); // TODO: Unify with `dict`?

#undef DEFINE_TYPE_TRAIT
#undef DEFINE_ROLE_TYPE_TRAIT

}  // namespace value

// TODO(syoyo): Range, Interval, Rect2i, Frustum, MultiInterval
// and Quaternion?

/*
#define VT_GFRANGE_VALUE_TYPES                 \
((      GfRange3f,           Range3f        )) \
((      GfRange3d,           Range3d        )) \
((      GfRange2f,           Range2f        )) \
((      GfRange2d,           Range2d        )) \
((      GfRange1f,           Range1f        )) \
((      GfRange1d,           Range1d        ))

#define VT_RANGE_VALUE_TYPES                   \
    VT_GFRANGE_VALUE_TYPES                     \
((      GfInterval,          Interval       )) \
((      GfRect2i,            Rect2i         ))

#define VT_QUATERNION_VALUE_TYPES           \
((      GfQuaternion,        Quaternion ))

#define VT_NONARRAY_VALUE_TYPES                 \
((      GfFrustum,           Frustum))          \
((      GfMultiInterval,     MultiInterval))

*/

}  // namespace tinyusdz
