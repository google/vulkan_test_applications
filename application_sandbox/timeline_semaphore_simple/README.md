# Timeline Semaphore Simple

Draws a simple cube on the screen. Inserts a timeline semaphore between
the VkAcquireNextImage and main Submission, and uses the same timeline
semaphore between the main submission and a dummy submission to signal
VkQueuePresentKHR.