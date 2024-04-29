struct required_feature
{
    umm OffsetInStruct;
    const char* Name;
};

#define RequiredFeature(s, f) { OffsetOf(s, f), #f }

#define RequiredDeviceFeature(f) RequiredFeature(VkPhysicalDeviceFeatures, f)
static const required_feature RequiredDeviceFeatures[] =
{
    RequiredDeviceFeature(robustBufferAccess),
    RequiredDeviceFeature(fullDrawIndexUint32),
    RequiredDeviceFeature(geometryShader),
    RequiredDeviceFeature(dualSrcBlend),
    RequiredDeviceFeature(multiDrawIndirect),
    RequiredDeviceFeature(drawIndirectFirstInstance),
    RequiredDeviceFeature(depthClamp),
    RequiredDeviceFeature(depthBiasClamp),
    RequiredDeviceFeature(depthBounds),
    RequiredDeviceFeature(alphaToOne),
    RequiredDeviceFeature(samplerAnisotropy),
    RequiredDeviceFeature(textureCompressionBC),
    RequiredDeviceFeature(pipelineStatisticsQuery),
    RequiredDeviceFeature(fragmentStoresAndAtomics),
    RequiredDeviceFeature(shaderImageGatherExtended),
    RequiredDeviceFeature(shaderStorageImageExtendedFormats),
    RequiredDeviceFeature(shaderStorageImageMultisample),
    RequiredDeviceFeature(shaderStorageImageReadWithoutFormat),
    RequiredDeviceFeature(shaderStorageImageWriteWithoutFormat),
    RequiredDeviceFeature(shaderUniformBufferArrayDynamicIndexing),
    RequiredDeviceFeature(shaderSampledImageArrayDynamicIndexing),
    RequiredDeviceFeature(shaderStorageBufferArrayDynamicIndexing),
    RequiredDeviceFeature(shaderInt64),
    RequiredDeviceFeature(shaderInt16),
    RequiredDeviceFeature(shaderResourceResidency),
    RequiredDeviceFeature(shaderResourceMinLod),
    RequiredDeviceFeature(sparseBinding),
    RequiredDeviceFeature(sparseResidencyBuffer),
    RequiredDeviceFeature(sparseResidencyImage2D),
    RequiredDeviceFeature(sparseResidencyImage3D),
    RequiredDeviceFeature(sparseResidencyAliased),
};

#define RequiredVulkan11Feature(f) RequiredFeature(VkPhysicalDeviceVulkan11Features, f)
static const required_feature RequiredVulkan11Features[] = 
{
    RequiredVulkan11Feature(storageBuffer16BitAccess),
    RequiredVulkan11Feature(uniformAndStorageBuffer16BitAccess),
    RequiredVulkan11Feature(shaderDrawParameters),
};

#define RequiredVulkan12Feature(f) RequiredFeature(VkPhysicalDeviceVulkan12Features, f)
static const required_feature RequiredVulkan12Features[] =
{
    RequiredVulkan12Feature(drawIndirectCount),
    RequiredVulkan12Feature(storageBuffer8BitAccess),
    RequiredVulkan12Feature(uniformAndStorageBuffer8BitAccess),
    RequiredVulkan12Feature(descriptorIndexing),
    RequiredVulkan12Feature(shaderUniformTexelBufferArrayDynamicIndexing),
    RequiredVulkan12Feature(shaderStorageTexelBufferArrayDynamicIndexing),
    RequiredVulkan12Feature(shaderUniformBufferArrayNonUniformIndexing),
    RequiredVulkan12Feature(shaderSampledImageArrayNonUniformIndexing),
    RequiredVulkan12Feature(shaderStorageBufferArrayNonUniformIndexing),
    RequiredVulkan12Feature(shaderStorageImageArrayNonUniformIndexing),
    RequiredVulkan12Feature(shaderUniformTexelBufferArrayNonUniformIndexing),
    RequiredVulkan12Feature(shaderStorageTexelBufferArrayNonUniformIndexing),
    RequiredVulkan12Feature(descriptorBindingSampledImageUpdateAfterBind),
    RequiredVulkan12Feature(descriptorBindingStorageBufferUpdateAfterBind),
    RequiredVulkan12Feature(descriptorBindingUniformTexelBufferUpdateAfterBind),
    RequiredVulkan12Feature(descriptorBindingStorageTexelBufferUpdateAfterBind),
    RequiredVulkan12Feature(descriptorBindingUpdateUnusedWhilePending),
    RequiredVulkan12Feature(descriptorBindingPartiallyBound),
    RequiredVulkan12Feature(descriptorBindingVariableDescriptorCount),
    RequiredVulkan12Feature(runtimeDescriptorArray),
    RequiredVulkan12Feature(scalarBlockLayout),
    RequiredVulkan12Feature(imagelessFramebuffer),
    RequiredVulkan12Feature(uniformBufferStandardLayout),
    RequiredVulkan12Feature(separateDepthStencilLayouts),
    RequiredVulkan12Feature(hostQueryReset),
    RequiredVulkan12Feature(timelineSemaphore),
    RequiredVulkan12Feature(bufferDeviceAddress),
    RequiredVulkan12Feature(vulkanMemoryModel),
    RequiredVulkan12Feature(vulkanMemoryModelDeviceScope),
    RequiredVulkan12Feature(shaderOutputViewportIndex),
    RequiredVulkan12Feature(shaderOutputLayer),
    RequiredVulkan12Feature(subgroupBroadcastDynamicId),
};

#define RequiredVulkan13Feature(f) RequiredFeature(VkPhysicalDeviceVulkan13Features, f)
static const required_feature RequiredVulkan13Features[] = 
{
    RequiredVulkan13Feature(robustImageAccess),
    RequiredVulkan13Feature(inlineUniformBlock),
    RequiredVulkan13Feature(descriptorBindingInlineUniformBlockUpdateAfterBind),
    RequiredVulkan13Feature(synchronization2),
    RequiredVulkan13Feature(shaderZeroInitializeWorkgroupMemory),
    RequiredVulkan13Feature(dynamicRendering),
    RequiredVulkan13Feature(maintenance4),

};

#define RequiredDescriptorBufferFeatures(f) RequiredFeature(VkPhysicalDeviceDescriptorBufferFeaturesEXT, f)
static const required_feature RequiredDescriptorBufferFeatures[] =
{
    RequiredDescriptorBufferFeatures(descriptorBuffer),
};

static const char* RequiredInstanceExtensions[] =
{
    VK_KHR_SURFACE_EXTENSION_NAME,
    "VK_KHR_win32_surface",
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};

internal b32
CheckRequiredFeatureSupport(const void* FeatureStruct, u32 FeatureCount, const required_feature* RequiredFeatures)
{
    b32 Result = true;
    for (u32 FeatureIndex = 0; FeatureIndex < FeatureCount; FeatureIndex++)
    {
        const required_feature* Feature = RequiredFeatures + FeatureIndex;
        VkBool32 IsFeatureSupported = *(VkBool32*)OffsetPtr(FeatureStruct, Feature->OffsetInStruct);
        if (!IsFeatureSupported)
        {
            Result = false;
            break;
        }
    }
    return(Result);
}

internal void
SetRequiredFeatures(void* FeatureStruct, u32 FeatureCount, const required_feature* Features)
{
    for (u32 FeatureIndex = 0; FeatureIndex < FeatureCount; FeatureIndex++)
    {
        *(VkBool32*)OffsetPtr(FeatureStruct, Features[FeatureIndex].OffsetInStruct) = VK_TRUE;
    }
}

internal VkResult InitializeVulkan(vulkan* Vulkan)
{
    VkResult Result = VK_SUCCESS;

    constexpr b32 UseValidation = true;
    const char* RequiredInstanceLayers_[] = 
    {
        "VK_LAYER_KHRONOS_validation",
    };
    u32 RequiredInstanceLayerCount_ = CountOf(RequiredInstanceLayers_);

    u32 RequiredInstanceLayerCount = UseValidation ? RequiredInstanceLayerCount_ : 0;
    const char** RequiredInstanceLayers = UseValidation ? RequiredInstanceLayers_ : nullptr;

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
            Vulkan->DescriptorBufferFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
            Vulkan->Vulkan13Features = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                .pNext = &Vulkan->DescriptorBufferFeatures,
            };
            Vulkan->Vulkan12Features = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                .pNext = &Vulkan->Vulkan13Features,
            };
            Vulkan->Vulkan11Features = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
                .pNext = &Vulkan->Vulkan12Features,
            };
            Vulkan->DeviceFeatures = 
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &Vulkan->Vulkan11Features,
            };


            VkPhysicalDevice SelectedDevice = VK_NULL_HANDLE;
            VkPhysicalDeviceProperties DeviceProps = {};
            // TODO(boti): better device selection
            for (u32 DeviceIndex = 0; DeviceIndex < PhysicalDeviceCount; DeviceIndex++)
            {
                vkGetPhysicalDeviceProperties(PhysicalDevices[DeviceIndex], &DeviceProps);
                vkGetPhysicalDeviceFeatures2(PhysicalDevices[DeviceIndex], &Vulkan->DeviceFeatures);

                b32 AllRequiredFeaturesSupported = CheckRequiredFeatureSupport(&Vulkan->DeviceFeatures.features, CountOf(RequiredDeviceFeatures), RequiredDeviceFeatures);
                AllRequiredFeaturesSupported &= CheckRequiredFeatureSupport(&Vulkan->Vulkan11Features, CountOf(RequiredVulkan11Features), RequiredVulkan11Features);
                AllRequiredFeaturesSupported &= CheckRequiredFeatureSupport(&Vulkan->Vulkan12Features, CountOf(RequiredVulkan12Features), RequiredVulkan12Features);
                AllRequiredFeaturesSupported &= CheckRequiredFeatureSupport(&Vulkan->Vulkan13Features, CountOf(RequiredVulkan13Features), RequiredVulkan13Features);
                AllRequiredFeaturesSupported &= CheckRequiredFeatureSupport(&Vulkan->DescriptorBufferFeatures, CountOf(RequiredDescriptorBufferFeatures), RequiredDescriptorBufferFeatures);
                if ((DeviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && AllRequiredFeaturesSupported)
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
            
            // NOTE(boti): Keeps sType and pNext intact
            #define ClearVulkanStruct(s) memset(OffsetPtr((s), sizeof(VkBaseOutStructure)), 0, sizeof(*(s)) - sizeof(VkBaseOutStructure))

            ClearVulkanStruct(&Vulkan->DescriptorBufferFeatures);
            ClearVulkanStruct(&Vulkan->Vulkan13Features);
            ClearVulkanStruct(&Vulkan->Vulkan12Features);
            ClearVulkanStruct(&Vulkan->Vulkan11Features);
            ClearVulkanStruct(&Vulkan->DeviceFeatures);
            
            #undef ClearVulkanStruct

            // NOTE(boti): Only request the required features
            SetRequiredFeatures(&Vulkan->DeviceFeatures.features, CountOf(RequiredDeviceFeatures), RequiredDeviceFeatures);
            SetRequiredFeatures(&Vulkan->Vulkan11Features, CountOf(RequiredVulkan11Features), RequiredVulkan11Features);
            SetRequiredFeatures(&Vulkan->Vulkan12Features, CountOf(RequiredVulkan12Features), RequiredVulkan12Features);
            SetRequiredFeatures(&Vulkan->Vulkan13Features, CountOf(RequiredVulkan13Features), RequiredVulkan13Features);
            SetRequiredFeatures(&Vulkan->DescriptorBufferFeatures, CountOf(RequiredDescriptorBufferFeatures), RequiredDescriptorBufferFeatures);

            // TODO(boti): This code is incorrect on shared memory devices
            vkGetPhysicalDeviceMemoryProperties(Vulkan->PhysicalDevice, &Vulkan->MemoryProps);
            for (u32 MemoryTypeIndex = 0; MemoryTypeIndex < Vulkan->MemoryProps.memoryTypeCount; MemoryTypeIndex++)
            {
                const VkMemoryType* Type =  Vulkan->MemoryProps.memoryTypes + MemoryTypeIndex;
                bool IsDeviceLocal  = Type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                bool IsHostVisible  = Type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                bool IsHostCoherent = Type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                bool IsHostCached   = Type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

                if (IsDeviceLocal && !IsHostVisible)
                {
                    Vulkan->GPUMemTypes |= (1 << MemoryTypeIndex);
                }
                else if (IsHostVisible)
                {
                    if (IsHostCoherent)
                    {
                        if      (IsDeviceLocal) Vulkan->BARMemTypes         |= (1 << MemoryTypeIndex);
                        else if (IsHostCached)  Vulkan->ReadbackMemTypes    |= (1 << MemoryTypeIndex);
                        else                    Vulkan->TransferMemTypes    |= (1 << MemoryTypeIndex);
                    }
                    else
                    {
                        // NOTE(boti): We don't support non-coherent memory types,
                    }
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