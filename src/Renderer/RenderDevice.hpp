#pragma once

struct vulkan
{
    //
    // Handles
    //
    VkInstance Instance;
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;

    VkQueue GraphicsQueue;
    VkQueue TransferQueue;

    //
    // Constants
    //
    u32 GraphicsQueueIdx;
    u32 TransferQueueIdx;

    u32 GPUMemTypes;
    u32 BARMemTypes;
    u32 SharedMemTypes;

    //
    // Info
    //
    static constexpr u32 MaxQueueFamilyCount = 8;
    u32 QueueFamilyCount;
    VkQueueFamilyProperties QueueFamilies[MaxQueueFamilyCount];
    VkPhysicalDeviceMemoryProperties MemoryProps;
    VkPhysicalDeviceProperties DeviceProps;

    VkPhysicalDeviceFeatures2 DeviceFeatures;
    VkPhysicalDeviceVulkan13Features Vulkan13Features;
    VkPhysicalDeviceVulkan12Features Vulkan12Features;
    VkPhysicalDeviceVulkan11Features Vulkan11Features;
    VkPhysicalDeviceDescriptorIndexingFeatures DescriptorIndexingFeatures;
};

VkResult InitializeVulkan(vulkan* Vulkan);