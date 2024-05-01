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
    VkQueue ComputeQueue;
    VkQueue TransferQueue;

    VkDebugUtilsMessengerEXT DebugMessenger;

    //
    // Constants
    //
    u32 GraphicsQueueIdx;
    u32 ComputeQueueIdx;
    u32 TransferQueueIdx;

    u32 GPUMemTypes;
    u32 BARMemTypes;
    u32 TransferMemTypes;
    u32 ReadbackMemTypes;

    f32 TimestampPeriod;

    u64 TexelBufferAlignment;
    u64 ConstantBufferAlignment;
    u64 StorageBufferAlignment;

    u64 MaxTexelBufferCount;
    u64 MaxConstantBufferByteSize;
    u64 MaxStorageBufferByteSize;
    u64 MaxPushConstantByteSize;

    //
    // Info
    //
    static constexpr u32 MaxQueueFamilyCount = 8;
    u32 QueueFamilyCount;
    VkQueueFamilyProperties QueueFamilies[MaxQueueFamilyCount];
    VkPhysicalDeviceMemoryProperties MemoryProps;
    VkPhysicalDeviceProperties2 DeviceProps;
    VkPhysicalDeviceVulkan11Properties Vulkan11Props;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT DescriptorBufferProps;

    VkPhysicalDeviceFeatures2 DeviceFeatures;
    VkPhysicalDeviceDescriptorBufferFeaturesEXT DescriptorBufferFeatures;
    VkPhysicalDeviceVulkan13Features Vulkan13Features;
    VkPhysicalDeviceVulkan12Features Vulkan12Features;
    VkPhysicalDeviceVulkan11Features Vulkan11Features;

    VkPhysicalDeviceLimits Limits;
};

internal VkResult InitializeVulkan(vulkan* Vulkan);