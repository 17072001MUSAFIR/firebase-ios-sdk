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

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_REMOTE_BUFFERED_WRITER_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_REMOTE_BUFFERED_WRITER_H_

#include <queue>

#include "grpcpp/generic/generic_stub.h"
#include "grpcpp/support/byte_buffer.h"

#include "Firestore/core/src/firebase/firestore/remote/stream_operation.h"
#include "Firestore/core/src/firebase/firestore/util/async_queue.h"

namespace firebase {
namespace firestore {
namespace remote {

class GrpcStream;

/**
 * `BufferedWriter` accepts GRPC write operations ("writes") on its queue and
 * writes them one by one. Only one write may be in progress ("active") at any
 * given time.
 *
 * Writes are put on the queue using `EnqueueWrite`; if no other write is
 * currently in progress, it will become active immediately, otherwise, it will
 * be "buffered" (put on the queue in this `BufferedWriter`). When a write
 * becomes active, it is executed (via `Execute`); a write is active from the
 * moment it is executed and until `DequeueNextWrite` is called on the
 * `BufferedWriter`. `DequeueNextWrite` makes the next write active, if any.
 *
 * `BufferedWriter` does not own any operations it stores.
 *
 * This class exists to help Firestore streams adhere to the gRPC requirement
 * that only one write operation may be active at any given time.
 */
class BufferedWriter {
 public:
  explicit BufferedWriter(GrpcStream* stream,
                          grpc::GenericClientAsyncReaderWriter* call,
                          util::AsyncQueue* firestore_queue)
      : stream_{stream}, call_{call}, firestore_queue_{firestore_queue} {
  }

  bool empty() const {
    return queue_.empty();
  }

  StreamWrite* EnqueueWrite(grpc::ByteBuffer&& write);
  StreamWrite* DequeueNextWrite();

  // Doesn't affect the write that is currently in progress.
  void DiscardUnstartedWrites();

 private:
  StreamWrite* TryStartWrite();

  // This are needed to create new `StreamWrite`s.
  GrpcStream* stream_ = nullptr;
  grpc::GenericClientAsyncReaderWriter* call_ = nullptr;
  util::AsyncQueue* firestore_queue_ = nullptr;

  std::queue<grpc::ByteBuffer> queue_;
  bool has_active_write_ = false;
};

}  // namespace remote
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_REMOTE_BUFFERED_WRITER_H_
