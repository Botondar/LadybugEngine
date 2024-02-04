internal VkResult InitializeVulkan(vulkan* Vulkan)
{
    VkResult Result = VK_SUCCESS;

    constexpr b32 UseValidation = false;
    const char* RequiredInstanceLayers_[] = 
    {
        "VK_LAYER_KHRONOS_validation",
    };
    u32 RequiredInstanceLayerCount_ = CountOf(RequiredInstanceLayers_);

    u32 RequiredInstanceLayerCount = UseValidation ? RequiredInstanceLayerCount_ : 0;
    const char** RequiredInstanceLayers = UseValidation ? RequiredInstanceLayers_ : nullptr;

    const char* RequiredInstanceExtensions[] = 
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface",
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    const char* RequiredDeviceExtensions[] = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
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
        .enabledLayerCount = RequiredInstanceLayerCount,
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
            VkPhysicalDevice SelectedDevice = VK_NULL_HANDLE;
            VkPhysicalDeviceProperties DeviceProps = {};
            // TODO(boti): better device selection
            for (u32 DeviceIndex = 0; DeviceIndex < PhysicalDeviceCount; DeviceIndex++)
            {
                vkGetPhysicalDeviceProperties(PhysicalDevices[DeviceIndex], &DeviceProps);
                if (DeviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    SelectedDevice = PhysicalDevices[DeviceIndex];
                    break;
                }
            }

            if (SelectedDevice)
            {
                Vulkan->PhysicalDevice = PhysicalDevices[0];
                Vulkan->DeviceProps = DeviceProps;
            }
            else
            {
                UnhandledError("Failed to find appropriate device");
            }

            // TODO(boti): Fold this into the device selection loop
            {
                Vulkan->DescriptorBufferProps = 
                {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT,
                    .pNext = nullptr,
                };
                VkPhysicalDeviceProperties2 Props2 = 
                {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                    .pNext = &Vulkan->DescriptorBufferProps,
                };
                vkGetPhysicalDeviceProperties2(SelectedDevice, &Props2);
            }

            Vulkan->TimestampPeriod = Vulkan->DeviceProps.limits.timestampPeriod;

            Vulkan->TexelBufferAlignment = Vulkan->DeviceProps.limits.minTexelBufferOffsetAlignment;
            Vulkan->ConstantBufferAlignment = Vulkan->DeviceProps.limits.minUniformBufferOffsetAlignment;
            Vulkan->StorageBufferAlignment = Vulkan->DeviceProps.limits.minStorageBufferOffsetAlignment;

            Vulkan->MaxTexelBufferCount = Vulkan->DeviceProps.limits.maxTexelBufferElements;
            Vulkan->MaxConstantBufferByteSize = Vulkan->DeviceProps.limits.maxUniformBufferRange;
            Vulkan->MaxStorageBufferByteSize = Vulkan->DeviceProps.limits.maxStorageBufferRange;
            Vulkan->MaxPushConstantByteSize = Vulkan->DeviceProps.limits.maxPushConstantsSize;

            if (Vulkan->MaxConstantBufferByteSize < (1 << 16))
            {
                UnimplementedCodePath;
            }

            Vulkan->DescriptorBufferFeatures = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
                .pNext = nullptr,

                .descriptorBuffer = VK_TRUE,
                .descriptorBufferCaptureReplay = VK_FALSE,
                .descriptorBufferImageLayoutIgnored = VK_FALSE,
                .descriptorBufferPushDescriptors = VK_FALSE,
            };
            Vulkan->Vulkan13Features = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                .pNext = &Vulkan->DescriptorBufferFeatures,

                .robustImageAccess = VK_TRUE,
                .inlineUniformBlock = VK_TRUE,
                .descriptorBindingInlineUniformBlockUpdateAfterBind = VK_TRUE,
                .synchronization2 = VK_TRUE,
                .shaderZeroInitializeWorkgroupMemory = VK_TRUE,
                .dynamicRendering = VK_TRUE,
                .maintenance4 = VK_TRUE,
            };
            Vulkan->Vulkan12Features = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                .pNext = &Vulkan->Vulkan13Features,

                .drawIndirectCount = VK_TRUE,
                .storageBuffer8BitAccess = VK_TRUE,
                .uniformAndStorageBuffer8BitAccess = VK_TRUE,
                .descriptorIndexing = VK_TRUE,
                .shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE,
                .shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE,
                .shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
                .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
                .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
                .shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
                .shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE,
                .shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE,
                //.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
                .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
                .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
                .descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE,
                .descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE,
                .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
                .descriptorBindingPartiallyBound = VK_TRUE,
                .descriptorBindingVariableDescriptorCount = VK_TRUE,
                .runtimeDescriptorArray = VK_TRUE,
                .scalarBlockLayout = VK_TRUE,
                .imagelessFramebuffer = VK_TRUE,
                .uniformBufferStandardLayout = VK_TRUE,
                .separateDepthStencilLayouts = VK_TRUE,
                .hostQueryReset = VK_TRUE,
                .timelineSemaphore = VK_TRUE,
                .bufferDeviceAddress = VK_TRUE,
                //.bufferDeviceAddressCaptureReplay = VK_TRUE,
                .vulkanMemoryModel = VK_TRUE,
                .shaderOutputViewportIndex = VK_TRUE,
                .shaderOutputLayer = VK_TRUE,
                .subgroupBroadcastDynamicId = VK_TRUE,
            };
            Vulkan->Vulkan11Features = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
                .pNext = &Vulkan->Vulkan12Features,

                .storageBuffer16BitAccess = VK_TRUE,
                .uniformAndStorageBuffer16BitAccess = VK_TRUE,
                .shaderDrawParameters = VK_TRUE,
            };
            Vulkan->DeviceFeatures = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &Vulkan->Vulkan11Features,
                .features = 
                {
                    .robustBufferAccess = VK_TRUE,
                    .fullDrawIndexUint32 = VK_TRUE,
                    .geometryShader = VK_TRUE,
                    .multiDrawIndirect = VK_TRUE,
                    .drawIndirectFirstInstance = VK_TRUE,
                    .depthClamp = VK_TRUE,
                    .depthBiasClamp = VK_TRUE,
                    .depthBounds = VK_TRUE,
                    .alphaToOne = VK_TRUE,
                    .samplerAnisotropy = VK_TRUE,
                    .textureCompressionBC = VK_TRUE,
                    .pipelineStatisticsQuery = VK_TRUE,
                    .fragmentStoresAndAtomics = VK_TRUE,
                    .shaderImageGatherExtended = VK_TRUE,
                    .shaderStorageImageExtendedFormats = VK_TRUE,
                    .shaderStorageImageMultisample = VK_TRUE,
                    .shaderStorageImageReadWithoutFormat = VK_TRUE,
                    .shaderStorageImageWriteWithoutFormat = VK_TRUE,
                    .shaderUniformBufferArrayDynamicIndexing = VK_TRUE,
                    .shaderSampledImageArrayDynamicIndexing = VK_TRUE,
                    .shaderStorageBufferArrayDynamicIndexing = VK_TRUE,
                    .shaderResourceResidency = VK_TRUE,
                    .shaderResourceMinLod = VK_TRUE,
                    .sparseBinding = VK_TRUE,
                    .sparseResidencyBuffer = VK_TRUE,
                    .sparseResidencyImage2D = VK_TRUE,
                    .sparseResidencyImage3D = VK_TRUE,
                    .sparseResidencyAliased = VK_TRUE,
                },
            };

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
            Vulkan->GraphicsQueueIdx = VK_QUEUE_FAMILY_IGNORED;
            Vulkan->ComputeQueueIdx = VK_QUEUE_FAMILY_IGNORED;
            Vulkan->TransferQueueIdx = VK_QUEUE_FAMILY_IGNORED;
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
                else if ((Flags & VK_QUEUE_COMPUTE_BIT) &&
                         !(Flags & VK_QUEUE_GRAPHICS_BIT))
                {
                    Vulkan->ComputeQueueIdx = i;
                }
                else if ((Flags & VK_QUEUE_TRANSFER_BIT) && 
                         (Flags & VK_QUEUE_SPARSE_BINDING_BIT) && 
                         !(Flags & VK_QUEUE_GRAPHICS_BIT) && 
                         !(Flags & VK_QUEUE_COMPUTE_BIT))
                {
                    Vulkan->TransferQueueIdx = i;
                }
            }
            Assert(Vulkan->GraphicsQueueIdx != VK_QUEUE_FAMILY_IGNORED);
            Assert(Vulkan->TransferQueueIdx != VK_QUEUE_FAMILY_IGNORED);

            // NOTE(boti): There's no way for me to test whether we're using async compute correctly or not,
            // so for now it's always disabled (even if the card supports that functionality).
#if 1
            Vulkan->ComputeQueueIdx = Vulkan->GraphicsQueueIdx;
            u32 ComputeQueueIdx = 1;
            u32 GraphicsQueueCount = 2;
#else
            u32 ComputeQueueIdx = 0;
            u32 GraphicsQueueCount = 1;
            if (Vulkan->ComputeQueueIdx == VK_QUEUE_FAMILY_IGNORED)
            {
                Vulkan->ComputeQueueIdx = Vulkan->GraphicsQueueIdx;
                ComputeQueueIdx = 1;
                GraphicsQueueCount = 2;
            }
#endif

            f32 QueuePriorityMax = 1.0f;
            VkDeviceQueueCreateInfo QueueInfos[] = 
            {
                // Graphics
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .queueFamilyIndex = Vulkan->GraphicsQueueIdx,
                    .queueCount = GraphicsQueueCount,
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
                // Compute
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .queueFamilyIndex = Vulkan->ComputeQueueIdx,
                    .queueCount = 1,
                    .pQueuePriorities = &QueuePriorityMax,
                },
            };
            u32 QueueInfoCount = (Vulkan->ComputeQueueIdx == Vulkan->GraphicsQueueIdx) ? 
                CountOf(QueueInfos) - 1 : CountOf(QueueInfos);

            VkDeviceCreateInfo DeviceInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &Vulkan->DeviceFeatures,
                .flags = 0,
                .queueCreateInfoCount = QueueInfoCount,
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
                vkGetDeviceQueue(Vulkan->Device, Vulkan->ComputeQueueIdx, ComputeQueueIdx, &Vulkan->ComputeQueue);
                vkGetDeviceQueue(Vulkan->Device, Vulkan->TransferQueueIdx, 0, &Vulkan->TransferQueue);

                vkCmdBeginDebugUtilsLabelEXT_   = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(Vulkan->Device, "vkCmdBeginDebugUtilsLabelEXT");
                vkCmdEndDebugUtilsLabelEXT_     = (PFN_vkCmdEndDebugUtilsLabelEXT)  vkGetDeviceProcAddr(Vulkan->Device, "vkCmdEndDebugUtilsLabelEXT");
                vkSetDebugUtilsObjectNameEXT_   = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(Vulkan->Device, "vkSetDebugUtilsObjectNameEXT");

                vkCmdBindDescriptorBuffersEXT_              = (PFN_vkCmdBindDescriptorBuffersEXT)           vkGetDeviceProcAddr(Vulkan->Device, "vkCmdBindDescriptorBuffersEXT");
                vkCmdSetDescriptorBufferOffsetsEXT_         = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)      vkGetDeviceProcAddr(Vulkan->Device, "vkCmdSetDescriptorBufferOffsetsEXT");
                vkGetDescriptorEXT_                         = (PFN_vkGetDescriptorEXT)                      vkGetDeviceProcAddr(Vulkan->Device, "vkGetDescriptorEXT");
                vkGetDescriptorSetLayoutBindingOffsetEXT_   = (PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)vkGetDeviceProcAddr(Vulkan->Device, "vkGetDescriptorSetLayoutBindingOffsetEXT");
                vkGetDescriptorSetLayoutSizeEXT_            = (PFN_vkGetDescriptorSetLayoutSizeEXT)         vkGetDeviceProcAddr(Vulkan->Device, "vkGetDescriptorSetLayoutSizeEXT");
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