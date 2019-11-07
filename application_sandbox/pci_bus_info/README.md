# PCI Bus Info

This sample gets `VkPhysicalDevicePCIBusInfoPropertiesEXT` and prints the
information out to the console. The querying of this information is enabled by
requesting the instance extension `VK_KHR_get_physical_device_properties2` and
the device extension `VK_EXT_pci_bus_info`.