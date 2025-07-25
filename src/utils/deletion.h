#pragma once
#include <deque>
#include <functional>
class DeletionQueue {
  public:
    static DeletionQueue& get() {
      if(nullptr == instance) instance = new DeletionQueue;
      return *instance;
    }

    DeletionQueue(const DeletionQueue&) = delete;
    DeletionQueue& operator=(const DeletionQueue&) = delete;
    static void destruct() {
      delete instance;
      instance = nullptr;
    }

    void push_function(std::function<void()>&& func) {
      deletors.push_back(func);
    }
    void flush() {
      for(auto it = deletors.rbegin(); it != deletors.rend(); it++) {
        (*it)();
      }
      deletors.clear();
    }
  private:
    // No public constructor or destructor
    DeletionQueue() = default;
    ~DeletionQueue() = default;
    static DeletionQueue* instance;
    std::deque<std::function<void()>> deletors;
};


