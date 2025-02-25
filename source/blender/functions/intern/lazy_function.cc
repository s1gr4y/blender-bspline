/* SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup fn
 */

#include "BLI_array.hh"

#include "FN_lazy_function.hh"

namespace blender::fn::lazy_function {

std::string LazyFunction::name() const
{
  return debug_name_;
}

std::string LazyFunction::input_name(int index) const
{
  return inputs_[index].debug_name;
}

std::string LazyFunction::output_name(int index) const
{
  return outputs_[index].debug_name;
}

void *LazyFunction::init_storage(LinearAllocator<> &UNUSED(allocator)) const
{
  return nullptr;
}

void LazyFunction::destruct_storage(void *storage) const
{
  BLI_assert(storage == nullptr);
  UNUSED_VARS_NDEBUG(storage);
}

bool LazyFunction::always_used_inputs_available(const Params &params) const
{
  for (const int i : inputs_.index_range()) {
    const Input &fn_input = inputs_[i];
    if (fn_input.usage == ValueUsage::Used) {
      if (params.try_get_input_data_ptr(i) == nullptr) {
        return false;
      }
    }
  }
  return true;
}

void Params::set_default_remaining_outputs()
{
  for (const int i : fn_.outputs().index_range()) {
    if (this->output_was_set(i)) {
      continue;
    }
    const Output &fn_output = fn_.outputs()[i];
    const CPPType &type = *fn_output.type;
    void *data_ptr = this->get_output_data_ptr(i);
    type.value_initialize(data_ptr);
    this->output_set(i);
  }
}

bool Params::try_enable_multi_threading_impl()
{
  return false;
}

}  // namespace blender::fn::lazy_function
