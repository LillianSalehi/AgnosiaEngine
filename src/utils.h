#pragma once

#define VK_NO_PROTOTYPES
#include "volk.h"
#include <stdexcept>
#include <string>


static void VK_CHECK(VkResult result) {
  if(result != VK_SUCCESS) {
    throw std::runtime_error("VkResult was not VK_SUCCESS. Code: " + std::to_string(result));    
  }
}
