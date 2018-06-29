#include "core/providers/cpu/activation/activations.h"

namespace Lotus {

#define REGISTER_UNARY_ELEMENTWISE_KERNEL_ALIAS(alias, x, sinceVersion)           \
  REGISTER_KERNEL(KernelDefBuilder(#alias)                                        \
                      .Domain(LotusIR::kOnnxDomain)                               \
                      .SinceVersion(sinceVersion)                                 \
                      .Provider(LotusIR::kCpuExecutionProvider)                   \
                      .MayInplace(0, 0)                                           \
                      .TypeConstraint("T", DataTypeImpl::GetTensorType<float>()), \
                  x<float>)

#define REGISTER_UNARY_ELEMENTWISE_KERNEL(x, sinceVersion) \
  REGISTER_UNARY_ELEMENTWISE_KERNEL_ALIAS(x, x, sinceVersion)

REGISTER_UNARY_ELEMENTWISE_KERNEL(Elu, 1);
REGISTER_UNARY_ELEMENTWISE_KERNEL(HardSigmoid, 6);
REGISTER_UNARY_ELEMENTWISE_KERNEL(LeakyRelu, 1);
REGISTER_UNARY_ELEMENTWISE_KERNEL(ParametricSoftplus, 1);
REGISTER_UNARY_ELEMENTWISE_KERNEL(Relu, 1);
REGISTER_UNARY_ELEMENTWISE_KERNEL(ScaledTanh, 1);
REGISTER_UNARY_ELEMENTWISE_KERNEL(Selu, 6);
REGISTER_UNARY_ELEMENTWISE_KERNEL(Sigmoid, 1);
// SoftPlus is the default case for ParametricSoftPlus
REGISTER_UNARY_ELEMENTWISE_KERNEL_ALIAS(Softplus, ParametricSoftplus, 1);
REGISTER_UNARY_ELEMENTWISE_KERNEL(Softsign, 1);
REGISTER_UNARY_ELEMENTWISE_KERNEL(Tanh, 1);
REGISTER_UNARY_ELEMENTWISE_KERNEL(ThresholdedRelu, 1);

}  // namespace Lotus
