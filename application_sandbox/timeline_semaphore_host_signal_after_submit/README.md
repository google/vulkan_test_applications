# Timeline Semaphore Host Signal After Submit

Draws a simple cube on the screen. Inserts a timeline semaphore between
the VkAcquireNextImage and main Submission, and uses the same timeline
semaphore between the main submission and a dummy submission to signal
VkQueuePresentKHR.

As opposed to Timeline Semaphore Simple, this causes the queue to stall
for approx 100ms after the call to VkQueueSubmit.