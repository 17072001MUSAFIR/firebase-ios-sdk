/*
 * Copyright 2018 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_EXECUTOR_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_EXECUTOR_H_

#include <chrono>  // NOLINT(build/c++11)
#include <functional>
#include <string>
#include <utility>

namespace firebase {
namespace firestore {
namespace util {

// A handle to an operation scheduled for future execution. The handle may
// outlive the operation, but it *cannot* outlive the executor that created it.
class DelayedOperation {
 public:
  DelayedOperation() {
  }
  // Internal use only.
  explicit DelayedOperation(std::function<void()>&& cancel_func)
      : cancel_func_{std::move(cancel_func)} {
  }

  // If the operation has not been run yet, cancels the operation. Otherwise,
  // this function is a no-op.
  void Cancel() {
    cancel_func_();
  }

 private:
  std::function<void()> cancel_func_;
};

namespace internal {

// An interface to a platform-specific executor of asynchronous tasks
// ("operations").
class Executor {
 public:
  using Tag = int;
  using Operation = std::function<void()>;
  using Milliseconds = std::chrono::milliseconds;

  // Operations scheduled for future execution are tagged to allow retreiving
  // the later. The tag is entirely opaque for the executor; in particular,
  // uniqueness of tags is not enforced.
  struct TaggedOperation {
    Tag tag{};
    Operation operation;
  };

  virtual ~Executor() {
  }

  // Schedules the `operation` to be asynchronously executed as soon as
  // possible. If called in quick succession, the operations will be
  // FIFO-ordered.
  virtual void Execute(Operation&& operation) = 0;
  // Like `Enqueue`, but blocks until the `operation` finishes, consequently
  // draining immediate operations from the executor.
  virtual void ExecuteBlocking(Operation&& operation) = 0;
  // Scheduled the given `operation` to be executed after `delay` milliseconds
  // from now, and returns a handle that allows to cancel the operation
  // (provided it hasn't been run already). The operation is tagged to allow
  // retrieving it later.
  //
  // `delay` must be non-negative; use `Execute` to schedule operations for
  // immediate execution.
  virtual DelayedOperation ScheduleExecution(Milliseconds delay,
                                             TaggedOperation&& operation) = 0;

  // Checks for the caller whether it is being invoked by this executor.
  virtual bool IsAsyncCall() const = 0;
  // Returns some sort of an identifier for the current execution context. The
  // only guarantee is that it will return different values depending on whether
  // this function is invoked by this executor or not.
  virtual std::string GetInvokerId() const = 0;

  // Checks whether an operation tagged with the given `tag` is currently
  // scheduled for future execution.
  virtual bool IsScheduled(Tag tag) const = 0;
  // Checks whether there are any scheduled operations pending execution.
  // Operations scheduled for immediate execution don't count, even if they
  // haven't been run already.
  virtual bool IsScheduleEmpty() const = 0;
  // Removes the nearest due scheduled operation from the schedule and returns
  // it to the caller. This function may be used to reschedule operations.
  //
  // Precondition: schedule must be non-empty.
  virtual TaggedOperation PopFromSchedule() = 0;
};

}  // namespace internal
}  // namespace util
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_EXECUTOR_H_
