#include <miniz.h>
#include <stdio.h>

int main() {
   mz_zip_archive zip_archive = {0};
   if (!mz_zip_reader_init_file(&zip_archive, "D:\\arom\\script.zip", 0)) {
      printf("Failed to open ZIP\n");
      return 1;
   }
   mz_zip_archive_file_stat file_stat;
   for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
      if (mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
         printf("File: %s, Size: %zu\n", file_stat.m_filename, file_stat.m_uncomp_size);
      }
   }
   mz_zip_reader_end(&zip_archive);
   return 0;
}