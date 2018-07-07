#pragma once

#include "core/common/common.h"
#include "core/framework/op_kernel.h"
#include "core/providers/cuda/cudnn_common.h"
#include "core/providers/cpu/nn/pool_base.h"

namespace Lotus {
namespace Cuda {

enum PoolType {
  MaxPool,
  AveragePool
};

template <typename T, PoolType type>
class Pool final : public CudaKernel, public PoolBase {
 public:
  Pool(OpKernelInfo info) : CudaKernel(info), PoolBase(info) {}

  Status Compute(OpKernelContext* context) const override;
};
}  // namespace Cuda
}  // namespace Lotus
