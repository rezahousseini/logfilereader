#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "logfile.hpp"

namespace pybind11 { namespace detail {

  template <> struct type_caster<node_t>
  {
    PYBIND11_TYPE_CASTER(node_t, _("node_t"));

    static handle
    cast(value_t&& value, return_value_policy policy, handle parent)
    {
      return std::visit([policy, parent](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, node_t>)
          {
            dict d;
            for (auto&& kv : arg)
              {
                auto key = reinterpret_steal<object>(make_caster<T>::cast(forward_like<T>(kv.first), policy, parent));
                auto value = reinterpret_steal<object>(make_caster<T>::cast(forward_like<T>(kv.second), policy, parent));
                if (!key or !value)
                  return handle();
                d[key] = value;
              }
            return d.release();
          }
        else if constexpr (std::is_same_v<T, std::vector<node_t>>)
          {
            list l(arg.size());
            size_t index = 0;
            for (auto&& element : arg)
              {
                auto value = reinterpret_steal<object>(make_caster<node_t>::cast(forward_like<node_t>(element), policy, parent));
                if (!value)
                  return handle();
                PyList_SET_ITEM(l.ptr(), (ssize_t) index++, value.release().ptr()); // steals a reference
              }
            return l.release(); 
          } 
        else
          return make_caster<T>::cast(forward_like<T>(arg), policy, parent); 
      }, value); 
    }

  }; 
}}

PYBIND11_MODULE(logfile, m)
{
  m.doc() = "Module to read MBC logfiles";
  m.def("read_header", &read_file_header, "Read header of MBC log file");
  m.def("read", &read_file, "Read header and body of MBC log file"); 
}
