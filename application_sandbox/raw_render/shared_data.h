#pragma once
#include <windows.h>

const char* imageMapping = "Global\\TestFileMappingObject";
const size_t k_file_mapping_size = 4096 * 4096 * 4 + 4096;

struct image_mapping_header {
  uint64_t width;
  uint64_t height;
  uint64_t num_images;
  uint64_t frame_num;
  LONG64 header_lock;
  uint64_t image_to_read;
  uint64_t image_being_read;
  uint64_t image_being_written;
  uint64_t image_offsets[];
};
