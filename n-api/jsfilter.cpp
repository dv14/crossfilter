#include "jsdimension.hpp"
#include "jsfilter.hpp"
#include "jsfeature.hpp"

napi_ref crossfilter::constructor;
napi_ref jsdimension::constructor;


template <typename Ret>
static napi_value create_iterable_dimension(napi_env env,
                                            js_function &jsf, int dim_type) {
  //  js_function jsf = extract_function(env, info, 1);
  crossfilter *filter = get_object<crossfilter>(env, jsf.jsthis);
  napi_ref this_ref;
  NAPI_CALL(napi_create_reference(env, jsf.jsthis, 1, &this_ref));
  napi_ref lambda_ref;
  NAPI_CALL(napi_create_reference(env, jsf.args[1], 1, &lambda_ref));
  jsdimension *obj = new jsdimension();
  obj->dim_type = dim_type;
  obj->env_ = env;
  int filter_type = filter->obj_type;
  obj->filter_type = filter_type;
  obj->is_iterable = true;
  obj->dim = new dimension_holder<js_array<Ret>, cross::iterable>(filter->filter.iterable_dimension(
      [obj, this_ref, lambda_ref, filter_type](auto v) -> js_array<Ret> {
        // napi_value value;
        // NAPI_CALL(napi_get_reference_value(obj->env_, v, &value));
        
        napi_value value = extract_value(obj->env_,v,filter_type);
        napi_value this_value;
        NAPI_CALL(napi_get_reference_value(obj->env_, this_ref, &this_value));
        napi_value lambda_value;
        NAPI_CALL(napi_get_reference_value(obj->env_, lambda_ref, &lambda_value));
        //napi_value result;
        js_array<Ret> arr;
        arr.env = obj->env_;
        NAPI_CALL(napi_call_function(obj->env_, this_value, lambda_value, 1,
                                     &value, &arr.array));
        uint32_t sz = 0;
        NAPI_CALL(napi_get_array_length(obj->env_, arr.array, &sz));
        arr.length = sz;

        // for (std::size_t i = 0; i < length; i++) {
        //   napi_value r;
        //   NAPI_CALL(napi_get_element(obj->env_, result, i, &r));
        //   out.push_back(convert_to<Ret>(obj->env_, r));
        // }
        return arr;
      }));
  //  obj->dim = dim;
  napi_value dim_this;
  NAPI_CALL(napi_create_object(env, &dim_this));
  NAPI_CALL(napi_wrap(env, dim_this, reinterpret_cast<void *>(obj),
                      jsdimension::Destructor,
                      nullptr, // finalize_hint
                      &obj->wrapper));
  add_function(env, dim_this, jsdimension::top, "top");
  add_function(env, dim_this, jsdimension::bottom, "bottom");
  add_function(env, dim_this, jsdimension::filter, "filter");
  add_function(env, dim_this, jsdimension::feature_count, "feature_count");
  add_function(env, dim_this, jsdimension::feature_sum, "feature_sum");
  add_function(env, dim_this, jsdimension::feature, "feature");
  add_function(env, dim_this, jsdimension::feature_all, "feature_all");
  add_function(env, dim_this, jsdimension::feature_all_count, "feature_all_count");
  add_function(env, dim_this, jsdimension::feature_all_sum, "feature_all_sum");
  return dim_this;
  //  return nullptr;
}
template <typename Ret>
static napi_value create_dimension(napi_env env,
                                   js_function &jsf, int dim_type) {
  // js_function jsf = extract_function(env, info, 1);
  crossfilter *filter = get_object<crossfilter>(env, jsf.jsthis);
  napi_ref this_ref;
  NAPI_CALL(napi_create_reference(env, jsf.jsthis, 1, &this_ref));
  napi_ref lambda_ref;
  NAPI_CALL(napi_create_reference(env, jsf.args[1], 1, &lambda_ref));
  jsdimension *obj = new jsdimension();
  obj->dim_type = dim_type;
  obj->env_ = env;
  obj->filter_type = filter->obj_type;
  obj->dim  = new dimension_holder<Ret,cross::non_iterable>(filter->filter.dimension([obj, this_ref,
                                                                                      lambda_ref](void* v) -> Ret {
                                                                                       napi_value value = extract_value(obj->env_, v, obj->filter_type);
                                                                                       napi_value this_value;
                                                                                       NAPI_CALL(napi_get_reference_value(obj->env_, this_ref, &this_value));
                                                                                       napi_value lambda_value;
                                                                                       NAPI_CALL(napi_get_reference_value(obj->env_, lambda_ref, &lambda_value));
                                                                                       napi_value result;
                                                                                       NAPI_CALL(napi_call_function(obj->env_, this_value, lambda_value, 1, &value,
                                                                                                                    &result));
                                                                                       napi_valuetype vt;
                                                                                       NAPI_CALL(napi_typeof(obj->env_, result, &vt));
                                                                                       return convert_to<Ret>(obj->env_, result);
                                                                                     }));

  napi_value dim_this;
  NAPI_CALL(napi_create_object(env, &dim_this));
  NAPI_CALL(napi_wrap(env, dim_this, reinterpret_cast<void *>(obj),
                      jsdimension::Destructor,
                      nullptr, // finalize_hint
                      &obj->wrapper));
  add_function(env, dim_this, jsdimension::top, "top");
  add_function(env, dim_this, jsdimension::bottom, "bottom");
  add_function(env, dim_this, jsdimension::filter, "filter");
  add_function(env, dim_this, jsdimension::feature_count, "feature_count");
  add_function(env, dim_this, jsdimension::feature_sum, "feature_sum");
  add_function(env, dim_this, jsdimension::feature, "feature");
  add_function(env, dim_this, jsdimension::feature_all, "feature_all");
  add_function(env, dim_this, jsdimension::feature_all_count, "feature_all_count");
  add_function(env, dim_this, jsdimension::feature_all_sum, "feature_all_sum");
  //    add_function(env, dim_this, dimension::filter, filter);

  return dim_this;
}
template <typename T>
static napi_value make_dimension(napi_env env,
                                 js_function &jsf, int dim_type) {
  bool is_iterable = false;
  if (jsf.args.size() == 3) {
    is_iterable = convert_to<bool>(env, jsf.args[2]);
  }
  if (is_iterable)
    return create_iterable_dimension<T>(env, jsf, dim_type);

  return create_dimension<T>(env, jsf, dim_type);
}

// template <typename F>
// static void add_function(napi_env env, napi_value obj, F func,
//                                       const std::string &name) {
//   napi_value fn;
//   napi_create_function(env, nullptr, 0, func, nullptr, &fn);
//   napi_set_named_property(env, obj, name.c_str(), fn);
// }

void crossfilter::Destructor(napi_env env, void* nativeObject, void* finalize_hint) {
  auto obj = reinterpret_cast<crossfilter*>(nativeObject);
  obj->~crossfilter();
}

void crossfilter::print_obj(napi_env env, napi_value & object) {
  napi_value names;
  NAPI_CALL(napi_get_property_names(env, object, &names));
  uint32_t ns;
  NAPI_CALL(napi_get_array_length(env, names, &ns));
  std:: cout << " { ";
  for(std::size_t i = 0; i < ns; i++) {
    napi_value v;
    NAPI_CALL(napi_get_element(env, names, i, &v));
    char name[256];
    std::size_t sz;
    NAPI_CALL(napi_get_value_string_latin1(env, v, name, 255, &sz));
    name[sz] = 0;
    napi_value vv;
    NAPI_CALL(napi_get_property(env, object, v, &vv));
    int r;
    NAPI_CALL(napi_get_value_int32(env, vv, &r));
    std::cout << name << ":" << r << ",";
  }
  std::cout << "}" << std::endl;
}


static void add_value(napi_env env, napi_value value, crossfilter * obj, bool allow_duplicates) {
    napi_valuetype valuetype;
    NAPI_CALL(napi_typeof(env, value, &valuetype));  

    if(valuetype == napi_object) {
      bool is_array = false;
      napi_is_array(env, value, &is_array);
      if(!is_array) {
        napi_ref v;
        napi_status status = napi_create_reference(env, value, 1, &v);
        assert(status == napi_ok);
        obj->filter.add((void*)v,allow_duplicates);
        obj->obj_type = is_object;
      } else {
        uint32_t size = 0;
        napi_get_array_length(env, value, &size);
        for(uint32_t i = 0; i < size; i++) {
          napi_value v;
          napi_get_element(env, value, i, & v);
          add_value(env, v, obj, allow_duplicates);
        }
      }
    } else {
      switch(valuetype) {
        case napi_boolean:
          obj->obj_type = is_bool;
          obj->filter.add((void*)new bool(convert_to<bool>(env, value)), allow_duplicates);
          break;
        case napi_number:
          obj->obj_type = is_double;
          obj->filter.add((void*)new double(convert_to<double>(env, value)), allow_duplicates);
          break;
        case napi_string: {
          obj->obj_type = is_string;
          std::string * ptr = new std::string(convert_to<std::string>(env, value));
          obj->filter.add((void*)ptr, allow_duplicates);
          break;
        }
        default:
          break;
      }
    }
  
}
napi_value crossfilter::add(napi_env env, napi_callback_info info) {
    js_function jsf = extract_function(env, info, 2);
    crossfilter* obj = get_object<crossfilter>(env, jsf.jsthis);

    napi_valuetype valuetype1;
    NAPI_CALL(napi_typeof(env, jsf.args[1], &valuetype1));


    bool allow = true;
    if(valuetype1 != napi_undefined) {
      NAPI_CALL(napi_get_value_bool(env,jsf.args[1],&allow));
    }
    add_value(env, jsf.args[0], obj, allow);
    napi_value num;
    int32_t i = 1;
    NAPI_CALL(napi_create_int32(env, i, &num));
    return num;
  }

napi_value crossfilter::check_function(napi_env env, napi_callback_info info) {
    // js_function jsf = extract_function(env, info, 1);
    // napi_value argv;
    // crossfilter* obj = get_object<crossfilter>(env, jsf.jsthis);
    // auto all = obj->filter.all();
    // napi_get_reference_value(env, all[0], & argv);
    // napi_value result;
    // napi_call_function(env,jsf.jsthis,jsf.args[0],1,&argv,&result);
    // return result;
  return nullptr;
  }

//  bool crossfilter::convert_to_bool(napi_env env, napi_value v) {
//    bool b = false;
//    NAPI_CALL(napi_get_value_bool(env, v, &b));
//    return b;
//  }
// bool crossfilter::convert_to_int32(napi_env env, napi_value v) {
//     int32_t i = 0;
//     NAPI_CALL(napi_get_value_int32(env, v, &i));
//     return i;
//   }
// int64_t crossfilter::convert_to_int64(napi_env env, napi_value v) {
//     int64_t i = 0;
//     NAPI_CALL(napi_get_value_int64(env, v, &i));
//     return i;
//   }

// double crossfilter::convert_to_double(napi_env env, napi_value v) {
//     double d = 0;
//     NAPI_CALL(napi_get_value_double(env, v, &d));
//     return d;
//   }

napi_value crossfilter::dimension(napi_env env, napi_callback_info info ) {
  js_function jsf = extract_function(env, info, 3);
  int dim_type = convert_to<int32_t>(env, jsf.args[0]);
  switch(dim_type) {
    case is_int64:
      return make_dimension<int64_t>(env,jsf, dim_type);
    case is_int32:
      return make_dimension<int32_t>(env,jsf, dim_type);
    case is_bool:
      return make_dimension<bool>(env,jsf, dim_type);
    case is_double:
      return make_dimension<double>(env,jsf, dim_type);
    case is_uint64:
      return make_dimension<uint64_t>(env,jsf, dim_type);
    case is_string:
      return make_dimension<std::string>(env,jsf, dim_type);
  }
  return nullptr;
}

napi_value crossfilter::remove( napi_env env, napi_callback_info info){
    js_function jsf = extract_function(env, info, 1);
    crossfilter* obj = get_object<crossfilter>(env, jsf.jsthis);
    if(jsf.args.size() == 1) {
      obj->filter.remove([&jsf, &env, &obj](auto & v, int) {
                           napi_value value = extract_value(env,v, obj->obj_type);
                           napi_value result;
                           napi_call_function(env,jsf.jsthis,jsf.args[0],1,&value,&result);
                           bool bresult = false;
                           napi_get_value_bool(env,result,&bresult);
                           return bresult;
                         });
    } else {
      obj->filter.remove();
    }
    napi_value num;
    int32_t i = 1;
    NAPI_CALL(napi_create_int32(env, i, &num));
    return num;
  }

napi_value crossfilter::all(napi_env env, napi_callback_info info) {
    napi_value jsthis;
    NAPI_CALL(napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr));

    crossfilter* obj = get_object<crossfilter>(env, jsthis);

    auto data = obj->filter.all();
    napi_value result;
    NAPI_CALL(napi_create_array_with_length(env, data.size(), &result));
    int i = 0;
    for(auto & v : data) {
      napi_value value = extract_value(env, v, obj->obj_type);
      NAPI_CALL(napi_set_element(env, result, i, value));
      i++;
    }
    return result;
  }

static napi_value  create_feature(napi_env env, jsfeature * feature) {
  napi_value f_this;
  NAPI_CALL(napi_create_object(env, &f_this));
  NAPI_CALL(napi_wrap(env, f_this, reinterpret_cast<void *>(feature),
                      jsfeature::Destructor,
                      nullptr, // finalize_hint
                      &feature->wrapper));
  add_function(env, f_this,jsfeature::all, "all");
  add_function(env, f_this,jsfeature::top, "top");
  add_function(env, f_this,jsfeature::value,"value");
  add_function(env, f_this,jsfeature::size,"size");
  add_function(env, f_this,jsfeature::order,"order");
  add_function(env, f_this,jsfeature::order_natural,"order_natural");
  return f_this;
}


napi_value crossfilter::feature_count(napi_env env, napi_callback_info info) {
  js_function jsf = extract_function(env, info, 0);
  crossfilter* obj = get_object<crossfilter>(env, jsf.jsthis);
  jsfeature * feature = new jsfeature();
  feature->key_type = is_uint64;
  feature->value_type = is_uint64;
  feature->dim_type = is_cross;
  feature->ptr = new feature_holder<uint64_t,uint64_t,cross::filter<void*, std::function<uint64_t(void*)>>, true>(obj->filter.feature_count());
  feature->is_group_all  = true;
  return create_feature(env, feature);

  
}
template<typename T>
static auto make_value(napi_env env, napi_ref & this_ref, napi_ref value_ref, int object_type) {
  return [env, this_ref,
          value_ref, object_type](void* v) -> T {
           napi_value value = extract_value(env,v,object_type);
           //           NAPI_CALL(napi_get_reference_value(env, v, &value));
           napi_value this_value;
           NAPI_CALL(napi_get_reference_value(env, this_ref, &this_value));
           napi_value value_value;
           NAPI_CALL(napi_get_reference_value(env, value_ref, &value_value));
           napi_value result;
           NAPI_CALL(napi_call_function(env, this_value, value_value, 1, &value,
                                        &result));
           return convert_to<T>(env, result);};
}


template<typename V>
static napi_value feature_sum_(napi_env env, js_function & jsf, crossfilter * obj, int value_type) {
  napi_ref this_ref;
  NAPI_CALL(napi_create_reference(env, jsf.jsthis, 1, &this_ref));
  napi_ref value_ref;
  NAPI_CALL(napi_create_reference(env, jsf.args[1], 1, &value_ref));
  
  jsfeature * feature = new jsfeature();
  feature->key_type = is_uint64;
  feature->value_type = value_type;
  feature->dim_type = is_cross;
  feature->ptr = new feature_holder<uint64_t,V,cross::filter<void*, std::function<uint64_t(void*)>>, true>(std::move(obj->filter.feature_sum(make_value<V>(env, this_ref, value_ref, obj->obj_type))));
  return create_feature(env,feature);
}

napi_value crossfilter::feature_sum(napi_env env, napi_callback_info info) {
  js_function jsf = extract_function(env, info, 2);
  crossfilter* obj = get_object<crossfilter>(env, jsf.jsthis);
  int32_t value_type = convert_to<int32_t>(env, jsf.args[0]);
  switch(value_type) {
    case is_int64:
      return feature_sum_<int64_t>(env, jsf, obj, value_type);
    case is_int32:
      return feature_sum_<int32_t>(env, jsf, obj, value_type);
    case is_bool:
      return feature_sum_<bool>(env, jsf, obj, value_type);
    case is_double:
      return feature_sum_<double>(env, jsf, obj, value_type);
    case is_uint64:
      return feature_sum_<uint64_t>(env, jsf, obj, value_type);
  }
  return nullptr;
}

template<typename T>
static auto make_add(napi_env env, napi_ref & this_ref, napi_ref & add_ref, int object_type) {
  return [env, this_ref, object_type,
               add_ref](T & r, void* const& rec, bool b ) -> T {
                napi_value args[3];
                args[0] = convert_from(env,r);
                //NAPI_CALL(napi_get_reference_value(env, rec, &args[1]));
                args[1] = extract_value(env, rec, object_type);
                args[2] = convert_from(env, b);

                napi_value this_value;
                NAPI_CALL(napi_get_reference_value(env, this_ref, &this_value));
                napi_value add_;
                NAPI_CALL(napi_get_reference_value(env, add_ref, &add_));
                napi_value result;
                NAPI_CALL(napi_call_function(env, this_value, add_, 3, args,
                                             &result));
                return convert_to<T>(env, result);};
}
template<typename T>
static auto make_remove(napi_env env, napi_ref & this_ref, napi_ref & remove_ref, int object_type) {
  return [env, this_ref, object_type,
          remove_ref](T & r, void* const & rec, bool b ) -> T {
           napi_value args[3];
           args[0] = convert_from(env, r);
           //NAPI_CALL(napi_get_reference_value(env, rec, &args[1]));
           args[1] = extract_value(env, rec, object_type);
           args[2] = convert_from(env, b);
           napi_value this_value;
           NAPI_CALL(napi_get_reference_value(env, this_ref, &this_value));
           napi_value remove_;
           NAPI_CALL(napi_get_reference_value(env, remove_ref, &remove_));
           napi_value result;
           NAPI_CALL(napi_call_function(env, this_value, remove_, 3, args,
                                        &result));
           return convert_to<T>(env, result);};
}
template<typename T>
static auto make_init(napi_env env, napi_ref & this_ref, napi_ref & init_ref) {
  return [env, this_ref,
          init_ref]() -> T {
           napi_value this_value;
           NAPI_CALL(napi_get_reference_value(env, this_ref, &this_value));
           napi_value init_;
           NAPI_CALL(napi_get_reference_value(env, init_ref, &init_));
           napi_value result;
           NAPI_CALL(napi_call_function(env, this_value, init_, 0, nullptr,
                                        &result));
           return convert_to<T>(env, result);};

}

template<typename V>
 napi_value feature_all_(napi_env env, js_function& jsf, crossfilter * obj,  int value_type) {
    jsfeature * feature = new jsfeature();
    feature->key_type = is_uint64;
    feature->value_type = value_type;
    feature->dim_type = is_cross;

    napi_ref this_ref;
    NAPI_CALL(napi_create_reference(env, jsf.jsthis, 1, &this_ref));
    napi_ref remove_ref;
    napi_ref add_ref;
    napi_ref init_ref;
    NAPI_CALL(napi_create_reference(env, jsf.args[1], 1, &add_ref));
    NAPI_CALL(napi_create_reference(env, jsf.args[2], 1, &remove_ref));
    NAPI_CALL(napi_create_reference(env, jsf.args[3], 1, &init_ref));
    feature->ptr = new feature_holder<uint64_t,V,cross::filter<void*,std::function<uint64_t(void*)>>, true>(obj->filter.feature(make_add<V>(env,  this_ref, add_ref, obj->obj_type),
                                                                                                 make_remove<V>(env, this_ref, remove_ref, obj->obj_type),
                                                                                                 make_init<V>(env, this_ref, init_ref)));
    feature->is_group_all  = true;
    return create_feature(env, feature);
  }

napi_value crossfilter::feature(napi_env env, napi_callback_info info) {
  auto jsf = extract_function(env,info,4);
  crossfilter* obj = get_object<crossfilter>(env, jsf.jsthis);
  int32_t value_type = convert_to<int32_t>(env, jsf.args[0]);

  switch(value_type) {
    case is_int64:
      return feature_all_<int64_t>(env, jsf, obj, value_type);
    case is_int32:
      return feature_all_<int32_t>(env, jsf, obj, value_type);
    case is_bool:
      return feature_all_<bool>(env, jsf, obj, value_type);
    case is_double:
      return feature_all_<double>(env, jsf, obj, value_type);
    case is_uint64:
      return feature_all_<uint64_t>(env, jsf, obj, value_type);
  }
  return nullptr;
}

napi_value crossfilter::size(napi_env env, napi_callback_info info) {
  auto jsf = extract_function(env, info, 0);
  crossfilter * obj = get_object<crossfilter>(env, jsf.jsthis);
  std::size_t sz = obj->filter.size();
  return convert_from<int64_t>(env, sz);
}

napi_value crossfilter::all_filtered(napi_env env, napi_callback_info info) {
  auto jsf = extract_function(env, info, 1);
  crossfilter * obj = get_object<crossfilter>(env, jsf.jsthis);
  if(jsf.args.empty()) {
    auto data = obj->filter.all_filtered();
    napi_value result;
    NAPI_CALL(napi_create_array_with_length(env, data.size(), &result));
    int i = 0;
    for(auto & v : data) {
      napi_value value = extract_value(env, v, obj->obj_type);
      NAPI_CALL(napi_set_element(env, result, i, value));
      i++;
    }
    return result;
  }
  return nullptr;
}

napi_value crossfilter::is_element_filtered(napi_env env, napi_callback_info info) {
  auto jsf = extract_function(env, info, 1);
  crossfilter * obj = get_object<crossfilter>(env, jsf.jsthis);
  std::size_t index = convert_to<uint64_t>(env, jsf.args[0]);
  return convert_from<bool>(env, obj->filter.is_element_filtered(index));
}
