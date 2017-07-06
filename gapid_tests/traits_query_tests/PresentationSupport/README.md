# Physical Device Presentation Support

## vkGetPhysicalDeviceXcbPresentationSupportKHR
```c++
VkBool32 vkGetPhysicalDeviceXcbPresentationSupportKHR(
    VkPhysicalDevice                            physicalDevice,
    uint32_t                                    queueFamilyIndex,
    xcb_connection_t*                           connection,
    xcb_visualid_t                              visual_id);
```

According to Vulkan Spec
- `connection` is a pointer to an `xcb_connection_t` to the X server.
- `visual_id` is an X11 visual (`xcb_visualid_t`)
- `queueFamilyIndex` **must** be less than `pQueueFamilyPropertyCount` returned
  by `vkGetPhysicalDeviceQueueFamilyProperties` for the given `physicalDevice`.

These tests should test the following cases:
- [ ] `physicalDevice` of valid value
- [ ] `queueFamilyIndex` of valid value
- [ ] `connect` of valid value
- [ ] `visual_id` of valid value

## vkGetPhysicalDeviceWin32PresentationSupportKHR
```c++
VkBool32 vkGetPhysicalDeviceWin32PresentationSupportKHR(
    VkPhysicalDevice                            physicalDevice,
    uint32_t                                    queueFamilyIndex);
```

According to Vulkan Spec
- `queueFamilyIndex` **must** be less than `pQueueFamilyPropertyCount` returned
  by `vkGetPhysicalDeviceQueueFamilyProperties` for the given `physicalDevice`.

These tests should test the following cases:
- [ ] `physicalDevice` of valid value
- [ ] `queueFamilyIndex` of valid value
