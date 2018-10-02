# Copyright 2017 Google Inc.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from gapit_test_framework import gapit_test, require, require_equal, require_true
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest, get_read_offset_function
from struct_offsets import VulkanStruct, UINT32_T, ARRAY, UINT64_T, POINTER, HANDLE
from vulkan_constants import *

EVENT_CREATE_INFO = [
    ("sType", UINT32_T), ("pNext", POINTER), ("flags", UINT32_T)
]

BUFFER_SIZE = 4


def GetCreatedEvent(architecture, create_event):
    require_equal(VK_SUCCESS, int(create_event.return_val))
    require_not_equal(0, create_event.int_device)
    require_not_equal(0, create_event.hex_pCreateInfo)
    require_not_equal(0, create_event.hex_pEvent)

    create_info = VulkanStruct(architecture, EVENT_CREATE_INFO,
                               get_read_offset_function(
                                   create_event, create_event.hex_pCreateInfo))
    require_equal(VK_STRUCTURE_TYPE_EVENT_CREATE_INFO, create_info.sType)
    require_equal(0, create_info.pNext)
    require_equal(0, create_info.flags)

    event = little_endian_bytes_to_int(require(create_event.get_write_data(
        create_event.hex_pEvent, NON_DISPATCHABLE_HANDLE_SIZE)))
    return event


def GetMappedLocation(test):
    require(test.nth_call_of("vkDestroyBuffer", 3))
    map_coherent_memory = require(test.next_call_of("vkMapMemory"))
    require_equal(VK_SUCCESS, int(map_coherent_memory.return_val))
    require_not_equal(0, map_coherent_memory.int_device)
    require_not_equal(0, map_coherent_memory.int_memory)
    # All vkMapMemory called in this test has offset 0.
    require_equal(0, map_coherent_memory.int_offset)
    require_true(map_coherent_memory.int_size >= BUFFER_SIZE)
    require_equal(0, map_coherent_memory.int_flags)
    require_not_equal(0, map_coherent_memory.hex_ppData)
    pData = little_endian_bytes_to_int(require(map_coherent_memory.get_write_data(
        map_coherent_memory.hex_ppData, test.architecture.int_pointerSize)))
    return pData


def GetStart(test, index):
    while index > 0:
        get_device_proc = require(test.next_call_of("vkGetDeviceProcAddr"))
        if get_device_proc.pName == "TAG":
            index -= 1


@gapit_test("Event_test")
class BasicHostSideCommandTest(GapitTest):

    def expect(self):
        architecture = self.architecture
        GetStart(self, 1)
        create_event = require(self.next_call_of("vkCreateEvent"))
        device = create_event.int_device
        first_status = require(self.next_call_of("vkGetEventStatus"))
        set_event = require(self.next_call_of("vkSetEvent"))
        second_status = require(self.next_call_of("vkGetEventStatus"))
        reset_event = require(self.next_call_of("vkResetEvent"))
        third_status = require(self.next_call_of("vkGetEventStatus"))
        destroy_event = require(self.next_call_of("vkDestroyEvent"))

        # Check vkCreateEvent
        event = GetCreatedEvent(architecture, create_event)
        require_not_equal(0, event)

        # Check the first status
        require_equal(device, first_status.int_device)
        require_equal(event, first_status.int_event)
        require_equal(VK_EVENT_RESET, int(first_status.return_val))

        # Check the vkSetEvent
        require_equal(device, set_event.int_device)
        require_equal(event, set_event.int_event)
        require_equal(VK_SUCCESS, int(set_event.return_val))

        # Check the second status
        require_equal(device, second_status.int_device)
        require_equal(event, second_status.int_event)
        require_equal(VK_EVENT_SET, int(second_status.return_val))

        # Check the vkResetEvent
        require_equal(device, reset_event.int_device)
        require_equal(event, reset_event.int_event)
        require_equal(VK_SUCCESS, int(reset_event.return_val))

        # Check the third status
        require_equal(device, third_status.int_device)
        require_equal(event, third_status.int_event)
        require_equal(VK_EVENT_RESET, int(third_status.return_val))

        # Check the vkDestroyEvent
        require_equal(device, destroy_event.int_device)
        require_equal(event, destroy_event.int_event)


@gapit_test("Event_test")
class SingleThreadTest(GapitTest):

    def expect(self):
        pData = GetMappedLocation(self)
        GetStart(self, 2)
        require(self.next_call_of("vkAllocateCommandBuffers"))
        require(self.next_call_of("vkCreateBuffer"))
        require(self.next_call_of("vkCreateBuffer"))
        require(self.next_call_of("vkCreateEvent"))
        require(self.next_call_of("vkCreateEvent"))

        # submit -> update -> set -> wait idle
        require(self.next_call_of("vkCmdWaitEvents"))
        require(self.next_call_of("vkCmdCopyBuffer"))
        require(self.next_call_of("vkQueueSubmit"))
        set_event = require(self.next_call_of("vkSetEvent"))
        require(self.next_call_of("vkQueueWaitIdle"))
        require(self.next_call_of("vkResetEvent"))
        require_equal(0x11111111, little_endian_bytes_to_int(require(
            set_event.get_read_data(pData, BUFFER_SIZE))))

        # update -> set -> submit -> wait idle
        require(self.next_call_of("vkSetEvent"))
        require(self.next_call_of("vkCmdWaitEvents"))
        submit = require(self.next_call_of("vkQueueSubmit"))
        require(self.next_call_of("vkQueueWaitIdle"))
        require(self.next_call_of("vkResetEvent"))
        require_equal(0x22222222, little_endian_bytes_to_int(require(
            submit.get_read_data(pData, BUFFER_SIZE))))

        # submit [cmdSetEvent (multiple), ... cmdWaitEvents]
        require(self.next_call_of("vkCmdSetEvent"))
        require(self.next_call_of("vkCmdSetEvent"))
        require(self.next_call_of("vkCmdWaitEvents"))
        require(self.next_call_of("vkCmdCopyBuffer"))
        submit = require(self.next_call_of("vkQueueSubmit"))
        require(self.next_call_of("vkQueueWaitIdle"))
        require(self.next_call_of("vkResetEvent"))
        require(self.next_call_of("vkResetEvent"))
        require_equal(0x33333333, little_endian_bytes_to_int(require(
            submit.get_read_data(pData, BUFFER_SIZE))))

        # submit [cmdSetEvent] -> submit [cmdWaitEvents]
        require(self.next_call_of("vkCmdSetEvent"))
        require(self.next_call_of("vkQueueSubmit"))
        require(self.next_call_of("vkCmdWaitEvents"))
        require(self.next_call_of("vkCmdCopyBuffer"))
        submit = require(self.next_call_of("vkQueueSubmit"))
        require(self.next_call_of("vkQueueWaitIdle"))
        require(self.next_call_of("vkResetEvent"))
        require_equal(0x44444444, little_endian_bytes_to_int(require(
            submit.get_read_data(pData, BUFFER_SIZE))))


@gapit_test("Event_test")
class MultipleThreadTest(GapitTest):

    def expect(self):
        pData = GetMappedLocation(self)
        GetStart(self, 3)
        require(self.next_call_of("vkAllocateCommandBuffers"))
        require(self.next_call_of("vkCreateBuffer"))
        require(self.next_call_of("vkCreateBuffer"))
        require(self.next_call_of("vkCreateEvent"))
        require(self.next_call_of("vkCreateEvent"))

        # Thread 1: submit [vkCmdWaitEvents] ->        -> queue wait idle
        # Thread 2:                            setEvent
        require(self.next_call_of("vkCmdWaitEvents"))
        require(self.next_call_of("vkCmdCopyBuffer"))
        require(self.next_call_of("vkQueueSubmit"))
        set_event = require(self.next_call_of("vkSetEvent"))
        require(self.next_call_of("vkQueueWaitIdle"))
        require(self.next_call_of("vkResetEvent"))
        require_equal(0x11111111, little_endian_bytes_to_int(require(
            set_event.get_read_data(pData, BUFFER_SIZE))))

        # Thread 1: submit [vkCmdWaitEvents (multiple events)] ->    -> queue idle
        # Thread 2:                                           setEvent(s)
        require(self.next_call_of("vkCmdWaitEvents"))
        require(self.next_call_of("vkCmdCopyBuffer"))
        require(self.next_call_of("vkQueueSubmit"))
        require(self.next_call_of("vkSetEvent"))
        second_set_event = require(self.next_call_of("vkSetEvent"))
        require(self.next_call_of("vkQueueWaitIdle"))
        require(self.next_call_of("vkResetEvent"))
        require(self.next_call_of("vkResetEvent"))
        require_equal(0x33333333, little_endian_bytes_to_int(require(
            second_set_event.get_read_data(pData, BUFFER_SIZE))))

        # Thread 1: submit [wait, reset, semaphore, fence]-> submit [wait]-> idle
        # Thread 2: setEvent -> setEvent
        require(self.next_call_of("vkCmdWaitEvents"))
        require(self.next_call_of("vkCmdCopyBuffer"))
        require(self.next_call_of("vkCmdResetEvent"))
        require(self.next_call_of("vkQueueSubmit"))
        require(self.next_call_of("vkCmdWaitEvents"))
        require(self.next_call_of("vkCmdCopyBuffer"))
        require(self.next_call_of("vkQueueSubmit"))

        first_set = require(self.next_call_of("vkSetEvent"))
        second_set = require(self.next_call_of("vkSetEvent"))
        require(self.next_call_of("vkQueueWaitIdle"))
        require(self.next_call_of("vkResetEvent"))
        require_equal(0x44444444, little_endian_bytes_to_int(require(
            first_set.get_read_data(pData, BUFFER_SIZE))))
        require_equal(0x55555555, little_endian_bytes_to_int(require(
            second_set.get_read_data(pData, BUFFER_SIZE))))

        # Thread 1: submit [wait x, wait y] -> idle
        # Thread 2:                set y -> set x
        require(self.next_call_of("vkCmdWaitEvents"))
        require(self.next_call_of("vkCmdWaitEvents"))
        require(self.next_call_of("vkCmdCopyBuffer"))
        require(self.next_call_of("vkQueueSubmit"))
        require(self.next_call_of("vkSetEvent"))
        second_set = require(self.next_call_of("vkSetEvent"))
        require(self.next_call_of("vkQueueWaitIdle"))
        require(self.next_call_of("vkResetEvent"))
        require(self.next_call_of("vkResetEvent"))

        require_equal(0x77777777, little_endian_bytes_to_int(require(
            second_set.get_read_data(pData, BUFFER_SIZE))))


def GetCreatedBuffer(create_buffer):
    require_equal(VK_SUCCESS, int(create_buffer.return_val))
    require_not_equal(0, create_buffer.int_device)
    require_not_equal(0, create_buffer.hex_pCreateInfo)
    require_not_equal(0, create_buffer.hex_pBuffer)

    buf = little_endian_bytes_to_int(require(create_buffer.get_write_data(
        create_buffer.hex_pBuffer, NON_DISPATCHABLE_HANDLE_SIZE)))
    return buf


def GetCreatedImage(create_image):
    require_equal(VK_SUCCESS, int(create_image.return_val))
    require_not_equal(0, create_image.int_device)
    require_not_equal(0, create_image.hex_pCreateInfo)
    require_not_equal(0, create_image.hex_pImage)

    img = little_endian_bytes_to_int(require(create_image.get_write_data(
        create_image.hex_pImage, NON_DISPATCHABLE_HANDLE_SIZE)))
    return img


@gapit_test("Event_test")
class MemoryBarrierTest(GapitTest):

    def expect(self):
        architecture = self.architecture
        GetStart(self, 4)
        require(self.next_call_of("vkAllocateCommandBuffers"))
        src_buf = GetCreatedBuffer(
            require(self.next_call_of("vkCreateBuffer")))
        dst_buf = GetCreatedBuffer(
            require(self.next_call_of("vkCreateBuffer")))
        img = GetCreatedImage(require(self.next_call_of("vkCreateImage")))
        event = GetCreatedEvent(architecture,
                                require(self.next_call_of("vkCreateEvent")))

        wait_events = require(self.next_call_of("vkCmdWaitEvents"))
        require(self.next_call_of("vkCmdCopyBuffer"))
        require(self.next_call_of("vkQueueSubmit"))
        require(self.next_call_of("vkSetEvent"))
        require(self.next_call_of("vkQueueWaitIdle"))

        # Check vkCmdWaitEvents
        require_equal(1, wait_events.int_eventCount)
        require_not_equal(0, wait_events.hex_pEvents)
        require_equal(
            event,
            little_endian_bytes_to_int(require(wait_events.get_read_data(
                wait_events.hex_pEvents, NON_DISPATCHABLE_HANDLE_SIZE))))
        require_equal(VK_PIPELINE_STAGE_HOST_BIT, wait_events.int_srcStageMask)
        require_equal(VK_PIPELINE_STAGE_TRANSFER_BIT,
                      wait_events.int_dstStageMask)

        require_equal(1, wait_events.int_memoryBarrierCount)
        memory_barrier = VulkanStruct(architecture, [
            ("sType", UINT32_T),
            ("pNext", POINTER),
            ("srcAccessMask", UINT32_T),
            ("dstAccessMask", UINT32_T),
        ], get_read_offset_function(wait_events, wait_events.hex_pMemoryBarriers))
        require_equal(VK_STRUCTURE_TYPE_MEMORY_BARRIER, memory_barrier.sType)
        require_equal(0, memory_barrier.pNext)
        require_equal(VK_ACCESS_HOST_WRITE_BIT, memory_barrier.srcAccessMask)
        require_equal(VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                      memory_barrier.dstAccessMask)

        BUFFER_MEMORY_BARRIER = [
            ("sType", UINT32_T),
            ("pNext", POINTER),
            ("srcAccessMask", UINT32_T),
            ("dstAccessMask", UINT32_T),
            ("srcQueueFamilyIndex", UINT32_T),
            ("dstQueueFamilyIndex", UINT32_T),
            ("buffer", HANDLE),
            ("offset", UINT64_T),
            ("size", UINT64_T),
        ]

        require_equal(2, wait_events.int_bufferMemoryBarrierCount)
        first_buffer_barrier = VulkanStruct(
            architecture, BUFFER_MEMORY_BARRIER, get_read_offset_function(
                wait_events, wait_events.hex_pBufferMemoryBarriers))
        require_equal(VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                      first_buffer_barrier.sType)
        require_equal(0, first_buffer_barrier.pNext)
        require_equal(0, first_buffer_barrier.srcAccessMask)
        require_equal(VK_ACCESS_TRANSFER_WRITE_BIT,
                      first_buffer_barrier.dstAccessMask)
        require_equal(VK_QUEUE_FAMILY_IGNORED,
                      first_buffer_barrier.srcQueueFamilyIndex)
        require_equal(VK_QUEUE_FAMILY_IGNORED,
                      first_buffer_barrier.dstQueueFamilyIndex)
        require_equal(dst_buf, first_buffer_barrier.buffer)
        require_equal(0, first_buffer_barrier.offset)
        require_equal(BUFFER_SIZE, first_buffer_barrier.size)

        second_buffer_barrier = VulkanStruct(
            architecture, BUFFER_MEMORY_BARRIER, get_read_offset_function(
                wait_events, wait_events.hex_pBufferMemoryBarriers +
                first_buffer_barrier.total_size))
        require_equal(VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                      second_buffer_barrier.sType)
        require_equal(0, second_buffer_barrier.pNext)
        require_equal(VK_ACCESS_HOST_WRITE_BIT,
                      second_buffer_barrier.srcAccessMask)
        require_equal(VK_ACCESS_TRANSFER_READ_BIT,
                      second_buffer_barrier.dstAccessMask)
        require_equal(VK_QUEUE_FAMILY_IGNORED,
                      second_buffer_barrier.srcQueueFamilyIndex)
        require_equal(VK_QUEUE_FAMILY_IGNORED,
                      second_buffer_barrier.dstQueueFamilyIndex)
        require_equal(src_buf, second_buffer_barrier.buffer)
        require_equal(0, second_buffer_barrier.offset)
        require_equal(BUFFER_SIZE, second_buffer_barrier.size)

        require_equal(1, wait_events.int_imageMemoryBarrierCount)
        image_barrier = VulkanStruct(architecture, [
            ("sType", UINT32_T),
            ("pNext", POINTER),
            ("srcAccessMask", UINT32_T),
            ("dstAccessMask", UINT32_T),
            ("oldLayout", UINT32_T),
            ("newLayout", UINT32_T),
            ("srcQueueFamilyIndex", UINT32_T),
            ("dstQueueFamilyIndex", UINT32_T),
            ("image", HANDLE),
            ("subresourceRange_aspectMask", UINT32_T),
            ("subresourceRange_baseMipLevel", UINT32_T),
            ("subresourceRange_levelCount", UINT32_T),
            ("subresourceRange_baseArrayLayer", UINT32_T),
            ("subresourceRange_layerCount", UINT32_T),
        ], get_read_offset_function(wait_events,
                                    wait_events.hex_pImageMemoryBarriers))
        require_equal(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                      image_barrier.sType)
        require_equal(0, image_barrier.pNext)
        require_equal(0, image_barrier.srcAccessMask)
        require_equal(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                      image_barrier.dstAccessMask)
        require_equal(VK_IMAGE_LAYOUT_UNDEFINED, image_barrier.oldLayout)
        require_equal(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      image_barrier.newLayout)
        require_equal(VK_QUEUE_FAMILY_IGNORED,
                      image_barrier.srcQueueFamilyIndex)
        require_equal(VK_QUEUE_FAMILY_IGNORED,
                      image_barrier.dstQueueFamilyIndex)
        require_equal(img, image_barrier.image)
        require_equal(VK_IMAGE_ASPECT_COLOR_BIT,
                      image_barrier.subresourceRange_aspectMask)
        require_equal(0, image_barrier.subresourceRange_baseMipLevel)
        require_equal(1, image_barrier.subresourceRange_levelCount)
        require_equal(0, image_barrier.subresourceRange_baseArrayLayer)
        require_equal(1, image_barrier.subresourceRange_layerCount)
