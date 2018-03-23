#include "core/providers/cpu/math/element_wise_ops.h"

namespace Lotus {

template <typename T>
auto EigenMap(Tensor& t) { return EigenVectorMap<T>(t.mutable_data<T>(), t.shape().Size()); }
template <typename T>
auto EigenMap(const Tensor& t) { return ConstEigenVectorMap<T>(t.data<T>(), t.shape().Size()); }

int FindShapeSubsetAxis(const TensorShape& shape, const TensorShape& find) {
  int findCount = int(find.NumDimensions());

  for (int i = int(shape.NumDimensions()) - findCount; i > 0; i--) {
    int j = 0;
    for (; j < findCount; j++) {
      if (shape[i + j] != find[j])
        break;
    }
    if (j == findCount)
      return i;
  }
  LOTUS_THROW("Tensors have no common shape subset");
}

void VerifyShapeSubsetAxis(const TensorShape& shape, const TensorShape& find, int64_t axis) {
  LOTUS_ENFORCE(axis >= 0 && axis < shape.NumDimensions(), "Axis attribute out of range");
  int dimensions = int(find.NumDimensions());
  for (int i = 0; i < dimensions; i++) {
    if (shape[int(axis) + i] != find[i])
      LOTUS_THROW("Axis attribute doesn't refer to a valid subset");
  }
}

template <typename T, typename Op>
void Broadcast(const Tensor& input1, const Tensor& input2, Tensor& output, int axis, Op op) {
  // If the axis_ attribute exists, use and verify it, otherwise look for the matching suffix
  if (axis == -1)
    axis = FindShapeSubsetAxis(input1.shape(), input2.shape());
  else
    VerifyShapeSubsetAxis(input1.shape(), input2.shape(), axis);

  // If the input tensor has dimensions like [2][3][4][5][6] and the second input has dimensions like [4][5]
  // Then we want to access the second as though the first two and last index is ignored, like this: [x][x][4][5][x] ('x' means value has no effect)
  // Since we're iterating sequentially through both tensors, we can do this by incrementing the index into
  // the second tensor every '2*3' elements (thus ignoring the first two dimensions),
  // and resetting the index every '2*3*4*5' elements (thus ignoring the last dimension)

  int64_t incrementPitch = 1;
  for (int i = 0; i < axis; i++)
    incrementPitch *= input1.shape()[i];

  int64_t resetPitch = input2.shape().Size();

  const T* input1_data = input1.data<T>();
  const T* input2_data = input2.data<T>();
  float* output_data = output.mutable_data<T>();
  auto outputSize = output.shape().Size();

  // Do the operation
  int input2_index = 0;
  int incrementCount = 0;
  for (int i = 0; i < outputSize; i++) {
    *output_data++ = op(*input1_data++, input2_data[input2_index]);

    if (++incrementCount == incrementPitch) {
      incrementCount = 0;
      if (++input2_index == resetPitch) {
        input2_index = 0;
      }
    }
  }
}

template <>
void Add<float>::compute(OpKernelContext* ctx) {
  auto& A = *ctx->input<Tensor>(0);
  auto& B = *ctx->input<Tensor>(1);
  auto& C = *ctx->output(0, A.shape());

  if (broadcast_)
    Broadcast<float>(A, B, C, int(axis_), [](float a, float b) { return a + b; });
  else {
    LOTUS_ENFORCE(A.shape() == B.shape(), "Inputs must have the same shape");
    EigenMap<float>(C) = EigenMap<float>(A) + EigenMap<float>(B);
  }
}

template <>
void Sub<float>::compute(OpKernelContext* ctx) {
  auto& A = *ctx->input<Tensor>(0);
  auto& B = *ctx->input<Tensor>(1);
  auto& C = *ctx->output(0, A.shape());

  if (broadcast_)
    Broadcast<float>(A, B, C, int(axis_), [](float a, float b) { return a - b; });
  else {
    LOTUS_ENFORCE(A.shape() == B.shape(), "Inputs must have the same shape");
    EigenMap<float>(C) = EigenMap<float>(A) - EigenMap<float>(B);
  }
}

template <>
void Mul<float>::compute(OpKernelContext* ctx) {
  auto& A = *ctx->input<Tensor>(0);
  auto& B = *ctx->input<Tensor>(1);
  auto& C = *ctx->output(0, A.shape());

  if (broadcast_)
    Broadcast<float>(A, B, C, int(axis_), [](float a, float b) { return a * b; });
  else {
    LOTUS_ENFORCE(A.shape() == B.shape(), "Inputs must have the same shape");
    EigenMap<float>(C) = EigenMap<float>(A).cwiseProduct(EigenMap<float>(B));
  }
}

template <>
void Reciprocal<float>::compute(OpKernelContext* ctx) {
  auto& X = *ctx->input<Tensor>(0);
  auto& Y = *ctx->output(0, X.shape());

  EigenMap<float>(Y) = EigenMap<float>(X).cwiseInverse();
}

template <>
void Sum<float>::compute(OpKernelContext* ctx) {
  auto inputCount = node().InputArgCount().front();
  LOTUS_ENFORCE(inputCount >= 1, "Must have 1 or more inputs");
  auto& data_0 = *ctx->input<Tensor>(0);
  auto& shape = data_0.shape();
  auto sum = EigenMap<float>(*ctx->output(0, shape));

  if (inputCount == 1) {
    sum = EigenMap<float>(data_0);
    return;
  }

  auto& data_1 = *ctx->input<Tensor>(1);
  LOTUS_ENFORCE(data_1.shape() == shape, "All inputs must have the same shape");

  sum = EigenMap<float>(data_0) + EigenMap<float>(data_1);
  for (int index = 2; index < inputCount; index++) {
    auto& data_n = *ctx->input<Tensor>(index);
    LOTUS_ENFORCE(data_n.shape() == shape, "All inputs must have the same shape");
    sum += EigenMap<float>(data_n);
  }
}

}  // namespace Lotus
