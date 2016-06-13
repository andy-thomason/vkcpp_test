
#include "include/vulkan/vulkan.h"
#include "include/vulkan/vk_cpp.hpp"

#include <iostream>

namespace vku {
  class instance : public vk::Instance {
  public:
    instance(bool enableValidation=true) {
      /*std::vector<vk::ExtensionProperties> ext_props = vk::enumerateInstanceExtensionProperties();
      for (auto &p : ext_props) {
        std::cout << p.extensionName << "\n";
      }*/

      std::vector<const char*> instanceExtensions;
      std::vector<const char*> instanceLayers;
      if (enableValidation) {
        instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
      }

      std::vector<const char*> deviceExtensions;
      std::vector<const char*> deviceLayers;
      if (enableValidation) {
        deviceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
      }

      vk::ApplicationInfo appInfo;
      
      vk::InstanceCreateInfo instanceCreateInfo;
      instanceCreateInfo.setPApplicationInfo(&appInfo);
      instanceCreateInfo.setPpEnabledExtensionNames(instanceExtensions.data());
      instanceCreateInfo.setEnabledExtensionCount((uint32_t)instanceExtensions.size());
      instanceCreateInfo.setPpEnabledLayerNames(instanceLayers.data());
      instanceCreateInfo.setEnabledLayerCount((uint32_t)instanceLayers.size());

      vk::Instance inst = vk::createInstance(instanceCreateInfo);
      static_cast<vk::Instance&>(*this) = inst;

      if (enableValidation) {
        auto vkCreateDebugReportCallbackEXT =
          (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
            inst,"vkCreateDebugReportCallbackEXT"
          );
        if (vkCreateDebugReportCallbackEXT) { 
          vk::DebugReportCallbackCreateInfoEXT info(
            vk::DebugReportFlagsEXT(
              vk::DebugReportFlagBitsEXT::eInformation|
              vk::DebugReportFlagBitsEXT::eWarning|
              vk::DebugReportFlagBitsEXT::ePerformanceWarning|
              vk::DebugReportFlagBitsEXT::eError|
              vk::DebugReportFlagBitsEXT::eDebug
            ),
            &debugReportCallback,
            nullptr
          );
          //inst.createDebugReportCallbackEXT(info);
          VkDebugReportCallbackEXT callback = nullptr;
          auto pinfo = reinterpret_cast<const VkDebugReportCallbackCreateInfoEXT*>( &info );
          vkCreateDebugReportCallbackEXT(*this, pinfo, nullptr, &callback);
        }
      }

      std::cout << "instance created\n";
      std::cout << *this << "\n";

      physicalDevices_ = enumeratePhysicalDevices();
      std::cout << physicalDevices_.size() << "\n";

      for (auto &p : physicalDevices_) {
        std::cout << p.getProperties().deviceName << "\n";
      }

      auto p = physicalDevices_[0];
      int32_t graphics_queue_index = getQueueFamilyIndex(p, vk::QueueFlagBits::eGraphics);
      int32_t compute_queue_index = getQueueFamilyIndex(p, vk::QueueFlagBits::eCompute);

      if (graphics_queue_index == -1) {
        throw std::runtime_error("no graphics queues found");
      }
      
      if (compute_queue_index == -1) {
        throw std::runtime_error("no compute queues found");
      }

      float qp[] = { 1.0f };
      vk::DeviceQueueCreateFlags qflags;
      vk::DeviceQueueCreateInfo cci(qflags, graphics_queue_index, 1, qp);
      
      vk::DeviceCreateFlags flags;
      vk::DeviceCreateInfo dci(flags, 1, &cci, 0, nullptr, 0, nullptr, nullptr);

      vk::Device device = p.createDevice(dci);
      std::cout << device << "\n";

      graphicsQueue_ = device.getQueue(graphics_queue_index, 0);
      computeQueue_ = device.getQueue(compute_queue_index, 0);
      std::cout << graphicsQueue_ << "\n";

      devices_.push_back(device);
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(
      VkDebugReportFlagsEXT flags,
      VkDebugReportObjectTypeEXT objectType,
      uint64_t object,
      size_t location,
      int32_t messageCode,
      const char* pLayerPrefix,
      const char* pMessage,
      void* pUserData
    ) {
      std::cout << "debugCallback: " << pMessage << "\n";
      return VK_FALSE;
    }

    int32_t getQueueFamilyIndex(const vk::PhysicalDevice &p, vk::QueueFlagBits flags) const {
      auto qfp = p.getQueueFamilyProperties();
      for (int32_t i = 0; i != qfp.size(); ++i) {
        auto &q = qfp[i];
        //std::cout << "queue " << i << " " << q.queueCount << " " << vk::to_string(q.queueFlags) << "\n";
        if (q.queueFlags & flags) {
          return i;
        }
      }
      return -1;
    }

    std::vector<vk::PhysicalDevice> physicalDevices_;
    std::vector<vk::Device> devices_;
    vk::Queue graphicsQueue_;
    vk::Queue computeQueue_;
  };
};

int main() {
  vku::instance instance(true);

   
  
/*
    // start by doing a full dump of layers and extensions enabled in Vulkan.
    uint32_t num_layers = 0;
    vkEnumerateInstanceLayerProperties(&num_layers, nullptr);
    layerProperties_.resize(num_layers);
    vkEnumerateInstanceLayerProperties(&num_layers, layerProperties_.data());

    for (auto &p : layerProperties_) {
      uint32_t num_exts = 0;
      vkEnumerateInstanceExtensionProperties(p.layerName, &num_exts, nullptr);
      size_t old_size = instanceExtensionProperties_.size();
      instanceExtensionProperties_.resize(old_size + num_exts);
      vkEnumerateInstanceExtensionProperties(p.layerName, &num_exts, instanceExtensionProperties_.data() + old_size);
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vku";
    appInfo.pEngineName = "vku";

    // Temporary workaround for drivers not supporting SDK 1.0.3 upon launch
    // todo : Use VK_API_VERSION 
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 2);

    std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
    std::vector<const char*> instanceLayers;

    #if defined(VK_KHR_WIN32_SURFACE_EXTENSION_NAME)
      instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    #elif defined(VK_KHR_XCB_SURFACE_EXTENSION_NAME)
      instanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    #elif defined(VK_KHR_MIR_SURFACE_EXTENSION_NAME)
      instanceExtensions.push_back(VK_KHR_MIR_SURFACE_EXTENSION_NAME);
    #endif

    // todo : check if all extensions are present

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    if (enableValidation) {
      instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
      instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
    }

    // todo: filter out extensions and layers that do not exist.
    instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    instanceCreateInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
    instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
    VkResult err = vkCreateInstance(&instanceCreateInfo, nullptr, &instance_);
*/
}


