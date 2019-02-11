// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/cuda/cu_inc/common.cuh"
#include "upsample_impl.h"

namespace onnxruntime {
namespace cuda {

template <typename T>
__global__ void _UpampleNearestKernel(const size_t rank,
                                      const int64_t* input_pitches,
                                      const fast_divmod* output_div_pitches,
                                      const fast_divmod* scales_div,
                                      const T* input_data,
                                      T* output_data,
                                      const size_t N) {
  CALCULATE_ELEMENTWISE_INDEX_OR_EXIT(id, N);
  CUDA_LONG input_index = 0;
  CUDA_LONG output_index = id;

  int div, mod;
  for (int dim = 0; dim < rank; ++dim) {
    output_div_pitches[dim].divmod(output_index, div, mod);
    output_index = mod;
    if (scales_div[dim].d_ != 1 && div > 0) {
      scales_div[dim].divmod(div, div, mod); 
    }
    input_index += input_pitches[dim] * div;
  }
  output_data[id] = input_data[input_index];
}

template <typename T>
__global__ void _UpampleBilinearKernel(const int64_t input_dim2,
                                       const int64_t* input_pitches,
                                       const fast_divmod* output_div_pitches,
                                       const fast_divmod* scales_div,
                                       const T* input_data,
                                       T* output_data,
                                       const size_t N) {
  CALCULATE_ELEMENTWISE_INDEX_OR_EXIT(id, N);
  CUDA_LONG input_index = 0;

  // For bilinear mode, scales[0]=scales[1]=1
  int mod;
  int index_of_dim0, index_of_dim1, index_of_dim2, index_of_dim3;
  output_div_pitches[0].divmod(id, index_of_dim0, mod);
  output_div_pitches[1].divmod(mod, index_of_dim1, mod);
  output_div_pitches[2].divmod(mod, index_of_dim2, mod);
  index_of_dim3 = mod;
  int index_of_input_dim2, index_of_input_dim3, x_offset, y_offset;
  scales_div[2].divmod(index_of_dim2, index_of_input_dim2, y_offset);
  scales_div[3].divmod(index_of_dim3, index_of_input_dim3, x_offset);

  input_index = index_of_dim0 * input_pitches[0] +
                index_of_dim1 * input_pitches[1] +
                index_of_input_dim2 * input_pitches[2] +
                index_of_input_dim3;
  
  T x00 = input_data[input_index];
  T x10, x01, x11;

  bool end_of_dim2 = false;
  if (index_of_input_dim2 == (input_dim2 - 1)) {
    // It's the end in dimension 2
    x01 = x00;
    end_of_dim2 = true;
  } else {
    x01 = input_data[input_index + input_pitches[2]];
  }

  if (index_of_input_dim3 == (input_pitches[2] - 1)) {
    // It's the end in dimension 3
    x10 = x00;
    x11 = x01;
  }
  else {
    x10 = input_data[input_index + 1];
    x11 = end_of_dim2 ? x10 : input_data[input_index + input_pitches[2] + 1];
  }

  T y_offset_T = static_cast<T>(y_offset);
  T x_offset_T = static_cast<T>(x_offset);
  T scales_div2_T = static_cast<T>(scales_div[2].d_);
  T scales_div3_T = static_cast<T>(scales_div[3].d_);
  T y0 = x00 + static_cast<T>(y_offset_T * (x01 - x00) / scales_div2_T);
  T y1 = x10 + static_cast<T>(y_offset_T * (x11 - x10) / scales_div2_T);

  output_data[id] = y0 + static_cast<T>(x_offset_T * (y1 - y0) / scales_div3_T);
}

template <typename T>
void UpampleImpl(const onnxruntime::UpsampleMode upsample_mode,
                 const size_t rank,
                 const int64_t input_dim2,
                 const int64_t* input_pitches,
                 const fast_divmod* output_div_pitches,
                 const fast_divmod* scales_div,
                 const T* input_data,
                 T* output_data,
                 const size_t N) {
  int blocksPerGrid = (int)(ceil(static_cast<float>(N) / GridDim::maxThreadsPerBlock));
  if (onnxruntime::UpsampleMode::NN == upsample_mode) {
    _UpampleNearestKernel<T><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
        rank, input_pitches, output_div_pitches, scales_div,
        input_data, output_data, N);
  } else if (onnxruntime::UpsampleMode::LINEAR == upsample_mode) {
    _UpampleBilinearKernel<T><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
        input_dim2, input_pitches, output_div_pitches, scales_div,
        input_data, output_data, N);
  }
}

#define SPECIALIZED_IMPL(T)                                                     \
  template void UpampleImpl<T>(const onnxruntime::UpsampleMode upsample_mode,   \
                               const size_t rank,                               \
                               const int64_t input_dim2,                        \
                               const int64_t* input_pitches,                    \
                               const fast_divmod* output_div_pitches,           \
                               const fast_divmod* scales_div,                   \
                               const T* input_data,                             \
                               T* output_data,                                  \
                               const size_t N);

SPECIALIZED_IMPL(float)
SPECIALIZED_IMPL(double)
SPECIALIZED_IMPL(half)
SPECIALIZED_IMPL(int32_t)
SPECIALIZED_IMPL(uint8_t)

}  // namespace cuda
}  // namespace onnxruntime
