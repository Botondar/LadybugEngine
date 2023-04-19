lbfn VkResult InitializeVulkan(vulkan* Vulkan)
{
    VkResult Result = VK_SUCCESS;

    const char* RequiredInstanceLayers[] = 
    {
        "VK_LAYER_KHRONOS_synchronization2",
        "VK_LAYER_KHRONOS_validation",
    };

    const char* RequiredInstanceExtensions[] = 
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface",
    };

    const char* RequiredDeviceExtensions[] = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME,
    };

    VkApplicationInfo AppInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0),
    };

    VkInstanceCreateInfo Info = 
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &AppInfo,
        .enabledLayerCount = CountOf(RequiredInstanceLayers),
        .ppEnabledLayerNames = RequiredInstanceLayers,
        .enabledExtensionCount= CountOf(RequiredInstanceExtensions),
        .ppEnabledExtensionNames = RequiredInstanceExtensions,
    };

    Result = vkCreateInstance(&Info, nullptr, &Vulkan->Instance);
    if (Result == VK_SUCCESS)
    {
        constexpr u32 MaxPhysicalDeviceCount = 8;
        u32 PhysicalDeviceCount = MaxPhysicalDeviceCount;
        VkPhysicalDevice PhysicalDevices[MaxPhysicalDeviceCount];
        Result = vkEnumeratePhysicalDevices(Vulkan->Instance, &PhysicalDeviceCount, PhysicalDevices);
        if (Result == VK_SUCCESS)
        {
            // TODO(boti): device selection
            Vulkan->PhysicalDevice = PhysicalDevices[0];
            vkGetPhysicalDeviceProperties(Vulkan->PhysicalDevice, &Vulkan->DeviceProps);
            Assert(Vulkan->DeviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

            Vulkan->DescriptorIndexingFeatures = 
            { 
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
                .pNext = nullptr,
            };
            Vulkan->Vulkan13Features = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                .pNext = &Vulkan->DescriptorIndexingFeatures,
            };
            Vulkan->DeviceFeatures = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &Vulkan->Vulkan13Features,
            };
            
            vkGetPhysicalDeviceFeatures2(Vulkan->PhysicalDevice, &Vulkan->DeviceFeatures);

            // TODO(boti): This code is incorrect on shared memory devices
            vkGetPhysicalDeviceMemoryProperties(Vulkan->PhysicalDevice, &Vulkan->MemoryProps);
            for (u32 i = 0; i < Vulkan->MemoryProps.memoryTypeCount; i++)
            {
                const VkMemoryType* Type =  Vulkan->MemoryProps.memoryTypes + i;
                bool bDeviceLocal = Type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                bool bHostVisible = Type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                bool bHostCoherent = Type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

                if (bDeviceLocal && !bHostVisible)
                {
                    Vulkan->GPUMemTypes |= (1 << i);
                }
                else if (bHostVisible && !bDeviceLocal)
                {
                    Assert(bHostCoherent);
                    Vulkan->SharedMemTypes |= (1 << i);
                }
                else if (bHostVisible && bDeviceLocal)
                {
                    Assert(bHostCoherent);
                    Vulkan->BARMemTypes |= (1 << i);
                }
            }


            Vulkan->QueueFamilyCount = Vulkan->MaxQueueFamilyCount;
            vkGetPhysicalDeviceQueueFamilyProperties(Vulkan->PhysicalDevice, &Vulkan->QueueFamilyCount, Vulkan->QueueFamilies);
            Vulkan->GraphicsQueueIdx = ~0u;
            Vulkan->TransferQueueIdx = ~0u;
            for (u32 i = 0; i < Vulkan->QueueFamilyCount; i++)
            {
                const VkQueueFamilyProperties* Props = Vulkan->QueueFamilies + i;

                VkQueueFlags Flags = Props->queueFlags;
                if ((Flags & VK_QUEUE_GRAPHICS_BIT) && 
                    (Flags & VK_QUEUE_COMPUTE_BIT) && 
                    (Flags & VK_QUEUE_TRANSFER_BIT) && 
                    (Flags & VK_QUEUE_SPARSE_BINDING_BIT))
                {
                    Vulkan->GraphicsQueueIdx = i;
                }
                else if ((Flags & VK_QUEUE_TRANSFER_BIT) && 
                         (Flags & VK_QUEUE_SPARSE_BINDING_BIT) && 
                         !(Flags & VK_QUEUE_GRAPHICS_BIT) && 
                         !(Flags & VK_QUEUE_COMPUTE_BIT))
                {
                    Vulkan->TransferQueueIdx = i;
                }
            }
            Assert(Vulkan->GraphicsQueueIdx != ~0u);
            Assert(Vulkan->TransferQueueIdx != ~0u);

            f32 QueuePriorityMax = 1.0f;

            VkDeviceQueueCreateInfo QueueInfos[] = 
            {
                // Graphics
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .queueFamilyIndex = Vulkan->GraphicsQueueIdx,
                    .queueCount = 1,
                    .pQueuePriorities = &QueuePriorityMax,
                },
                // Transfer
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .queueFamilyIndex = Vulkan->TransferQueueIdx,
                    .queueCount = 1,
                    .pQueuePriorities = &QueuePriorityMax,
                },
            };

            VkDeviceCreateInfo DeviceInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &Vulkan->DeviceFeatures,
                .flags = 0,
                .queueCreateInfoCount = CountOf(QueueInfos),
                .pQueueCreateInfos = QueueInfos,
                .enabledLayerCount = 0,
                .ppEnabledLayerNames = nullptr,
                .enabledExtensionCount = CountOf(RequiredDeviceExtensions),
                .ppEnabledExtensionNames = RequiredDeviceExtensions,
                .pEnabledFeatures = nullptr,
            };

            Result = vkCreateDevice(Vulkan->PhysicalDevice, &DeviceInfo, nullptr, &Vulkan->Device);
            if (Result == VK_SUCCESS)
            {
                vkGetDeviceQueue(Vulkan->Device, Vulkan->GraphicsQueueIdx, 0, &Vulkan->GraphicsQueue);
                vkGetDeviceQueue(Vulkan->Device, Vulkan->TransferQueueIdx, 0, &Vulkan->TransferQueue);
            }
            else
            {
                UnhandledError("Failed to create Vulkan device");
            }
        }
        else
        {
            UnhandledError("Failed to enumerate Vulkan physical devices");
        }
    }
    else
    {
        UnhandledError("Failed to create Vulkan instance");
    }

    return Result;
}