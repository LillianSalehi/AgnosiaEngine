#pragma once

#include <source_location>
#define VK_NO_PROTOTYPES
#include "volk.h"
#include <stdexcept>
#include <string>


static void VK_CHECK(VkResult result, std::source_location location = std::source_location::current()) {
  if(result != VK_SUCCESS) {
    std::string const fileName = location.file_name();
    std::string const line = std::to_string(location.line());
    std::string const function = location.function_name();
    
    throw std::runtime_error("VkResult was not VK_SUCCESS at: " + fileName + ":" + line + ", " + function);    
  }
}
template<class T> [[nodiscard]] T* Address(T&& v) {
  return std::addressof(v);
}
