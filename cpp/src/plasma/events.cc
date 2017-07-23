// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "plasma/events.h"

#include <errno.h>

void EventLoop::file_event_callback(
    aeEventLoop* loop, int fd, void* context, int events) {
  FileCallback* callback = reinterpret_cast<FileCallback*>(context);
  (*callback)(events);
}

int EventLoop::timer_event_callback(aeEventLoop* loop, TimerID timer_id, void* context) {
  TimerCallback* callback = reinterpret_cast<TimerCallback*>(context);
  return (*callback)(timer_id);
}

constexpr int kInitialEventLoopSize = 1024;

EventLoop::EventLoop() {
  loop_ = aeCreateEventLoop(kInitialEventLoopSize);
}

bool EventLoop::add_file_event(int fd, int events, const FileCallback& callback) {
  if (file_callbacks_.find(fd) != file_callbacks_.end()) { return false; }
  auto data = std::unique_ptr<FileCallback>(new FileCallback(callback));
  void* context = reinterpret_cast<void*>(data.get());
  // Try to add the file descriptor.
  int err = aeCreateFileEvent(loop_, fd, events, EventLoop::file_event_callback, context);
  // If it cannot be added, increase the size of the event loop.
  if (err == AE_ERR && errno == ERANGE) {
    err = aeResizeSetSize(loop_, 3 * aeGetSetSize(loop_) / 2);
    if (err != AE_OK) { return false; }
    err = aeCreateFileEvent(loop_, fd, events, EventLoop::file_event_callback, context);
  }
  // In any case, test if there were errors.
  if (err == AE_OK) {
    file_callbacks_.emplace(fd, std::move(data));
    return true;
  }
  return false;
}

void EventLoop::remove_file_event(int fd) {
  aeDeleteFileEvent(loop_, fd, AE_READABLE | AE_WRITABLE);
  file_callbacks_.erase(fd);
}

void EventLoop::run() {
  aeMain(loop_);
}

int64_t EventLoop::add_timer(int64_t timeout, const TimerCallback& callback) {
  auto data = std::unique_ptr<TimerCallback>(new TimerCallback(callback));
  void* context = reinterpret_cast<void*>(data.get());
  int64_t timer_id =
      aeCreateTimeEvent(loop_, timeout, EventLoop::timer_event_callback, context, NULL);
  timer_callbacks_.emplace(timer_id, std::move(data));
  return timer_id;
}

int EventLoop::remove_timer(int64_t timer_id) {
  int err = aeDeleteTimeEvent(loop_, timer_id);
  timer_callbacks_.erase(timer_id);
  return err;
}
