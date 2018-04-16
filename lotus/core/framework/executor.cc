#include "core/framework/executor.h"

#include <chrono>
#include <thread>

#include "core/common/common.h"
#include "core/common/logging/logging.h"
#include "core/framework/allocation_planner.h"
#include "core/framework/session_state.h"

namespace Lotus {
Common::Status Executor::Execute(const RunOptions& run_options,
                                 const NameMLValMap& feeds,
                                 const std::vector<std::string>& output_names,
                                 std::vector<MLValue>* p_fetches) {
  UNUSED_PARAMETER(run_options);
  UNUSED_PARAMETER(feeds);
  UNUSED_PARAMETER(output_names);
  UNUSED_PARAMETER(p_fetches);
  return Common::Status::OK();
}

// TODO move to its own file
class SequentialExecutor : public Executor {
 public:
  SequentialExecutor(const SessionState& session_state,
                     const NameMLValMap& feeds,
                     const std::vector<std::string>& output_names,
                     const std::vector<MLValue>& fetches,
                     const Logging::Logger& run_logger)
      : root_frame_(feeds, output_names, fetches, session_state),
        session_state_(session_state),
        run_logger_(run_logger) {
  }

  Common::Status Execute(const RunOptions& run_options,
                         const NameMLValMap& feeds,
                         const std::vector<std::string>& output_names,
                         std::vector<MLValue>* p_fetches) override {
    UNUSED_PARAMETER(run_options);
    UNUSED_PARAMETER(feeds);

    LOGS(run_logger_, INFO) << "Begin execution";
    const SequentialExecutionPlan* p_seq_exec_plan = session_state_.GetExecutionPlan();
    const auto& exec_plan_vec = p_seq_exec_plan->execution_plan;
    VLOGS(run_logger_, 1) << "Size of execution plan vector: " << exec_plan_vec.size();

    for (int i = 0; i < exec_plan_vec.size(); ++i) {
      const auto& node_exec_plan = exec_plan_vec[i];
      auto node_index = node_exec_plan.node_index;
      auto p_op_kernel = session_state_.GetKernel(node_index);

      // if a kernel has been added in the session state, it better be NON-null.
      LOTUS_ENFORCE(p_op_kernel != nullptr,
                    "Got nullptr from GetKernel for node: " +
                        session_state_.GetGraph()->GetNode(node_index)->Name());

      // construct OpKernelContext
      // TODO: log kernel inputs?
      OpKernelContext op_kernel_context(&root_frame_, p_op_kernel, run_logger_);
      // TODO: log kernel outputs?

      // call compute on the kernel
      // TODO Today the kernels don't return any status code.
      // They throw exceptions instead. We should change the compute
      // method to return a status code.
      VLOGS(run_logger_, 1) << "Computing kernel: " << p_op_kernel->Node().Name();
      LOTUS_RETURN_IF_ERROR(p_op_kernel->Compute(&op_kernel_context));

      // free ml-values corresponding to this node
      VLOGS(run_logger_, 1) << "Releasing node ML values after computing kernel: " << p_op_kernel->Node().Name();
      ReleaseNodeMLValues(p_seq_exec_plan, node_exec_plan);
    }

    VLOGS(run_logger_, 1) << "Fetching output.";
    LOTUS_RETURN_IF_ERROR(FetchOutput(output_names, p_fetches));

    return Common::Status::OK();
  }

  Common::Status FetchOutput(const std::vector<std::string>& output_names,
                             std::vector<MLValue>* p_fetches) {
    LOTUS_ENFORCE(p_fetches);  // this should've been checked before already.

    if (p_fetches->empty()) {
      p_fetches->resize(output_names.size());
    } else {
      // this should've been checked before already
      LOTUS_ENFORCE(output_names.size() == p_fetches->size(),
                    "output_names vector size: " + std::to_string(output_names.size()) +
                        " does not match that of fetches vector: " + std::to_string(p_fetches->size()));
    }

    auto idx = 0;
    for (const auto& oname : output_names) {
      VLOGS(run_logger_, 1) << "Attempting to fetch output with name: " << oname;
      int mlvalue_index;
      LOTUS_RETURN_IF_ERROR(session_state_.GetMLValueIdx(oname, &mlvalue_index));
      const MLValue& output_mlvalue = root_frame_.GetMLValue(mlvalue_index);
      VLOGS(run_logger_, 1) << "Copying fetched MLValue to output vector";
      (*p_fetches)[idx++] = output_mlvalue;
    }

    VLOGS(run_logger_, 1) << "Done with execution.";
    return Common::Status::OK();
  }

  void ReleaseNodeMLValues(const SequentialExecutionPlan* p_seq_exec_plan,
                           const SequentialExecutionPlan::NodeExecutionPlan& node_exec_plan) {
    for (auto i = node_exec_plan.free_from_index; i <= node_exec_plan.free_to_index; ++i) {
      auto mlvalue_idx = p_seq_exec_plan->to_be_freed[i];
      VLOGS(run_logger_, 1) << "Releasing mlvalue with index: " << mlvalue_idx;
      root_frame_.ReleaseMLValue(mlvalue_idx);
    }
  }

 private:
  LOTUS_DISALLOW_COPY_ASSIGN_AND_MOVE(SequentialExecutor);

  ExecutionFrame root_frame_;
  const SessionState& session_state_;
  const Logging::Logger& run_logger_;
};

std::unique_ptr<Executor> Executor::NewSequentialExecutor(const SessionState& session_state,
                                                          const NameMLValMap& feeds,
                                                          const std::vector<std::string>& output_names,
                                                          const std::vector<MLValue>& fetches,
                                                          const Logging::Logger& run_logger) {
  return std::unique_ptr<Executor>(new SequentialExecutor(session_state, feeds, output_names, fetches, run_logger));
}
}  // namespace Lotus
