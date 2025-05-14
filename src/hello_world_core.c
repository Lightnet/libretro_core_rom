#include <libretro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include "font.h"
#include "miniz.h"
#include "miniz_zip.h"

// Framebuffer dimensions
#define WIDTH 320
#define HEIGHT 240
// Colors (RGB565)
#define COLOR_WHITE 0xFFFF // White
#define COLOR_RED   0xF800 // Red

// Global variables
static retro_environment_t environ_cb;
static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static uint16_t framebuffer[WIDTH * HEIGHT]; // RGB565
static bool initialized = false;
static bool contentless_set = false;
static int env_call_count = 0;
static FILE *log_file = NULL;
static int square_x = 0;
static int square_y = 0;
static char *zip_path = NULL; // Store ZIP file path
static size_t rom_size = 0;

// File-based logging (simple)
static void fallback_log(const char *level, const char *msg) {
   if (!log_file) {
      log_file = fopen("core.log", "w");
      if (!log_file) {
         fprintf(stderr, "[ERROR] Failed to open core.log\n");
         return;
      }
   }
   fprintf(log_file, "[%s] %s\n", level, msg);
   fflush(log_file);
   fprintf(stderr, "[%s] %s\n", level, msg);
}

// File-based logging (formatted)
static void fallback_log_format(const char *level, const char *fmt, ...) {
   if (!log_file) {
      log_file = fopen("core.log", "w");
      if (!log_file) {
         fprintf(stderr, "[ERROR] Failed to open core.log\n");
         return;
      }
   }
   va_list args;
   va_start(args, fmt);
   fprintf(log_file, "[%s] ", level);
   vfprintf(log_file, fmt, args);
   fprintf(log_file, "\n");
   fflush(log_file);
   va_end(args);
   va_start(args, fmt);
   fprintf(stderr, "[%s] ", level);
   vfprintf(stderr, fmt, args);
   fprintf(stderr, "\n");
   va_end(args);
}

// Clear framebuffer to black
static void clear_framebuffer() {
   memset(framebuffer, 0, WIDTH * HEIGHT * sizeof(uint16_t));
}

// Draw a single 8x8 character at (x, y) in RGB565 color
static void draw_char(int x, int y, char c, uint16_t color) {
   if (c < 32 || c > 126) {
      if (log_cb)
         log_cb(RETRO_LOG_WARN, "[DEBUG] Invalid character: %c", c);
      else
         fallback_log_format("WARN", "Invalid character: %c", c);
      return;
   }
   const uint8_t *glyph = font_8x8[c - 32];
   for (int gy = 0; gy < 8; gy++) {
      for (int gx = 0; gx < 8; gx++) {
         if (glyph[gy] & (1 << (7 - gx))) {
            int px = x + gx;
            int py = y + gy;
            if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
               framebuffer[py * WIDTH + px] = color;
            }
         }
      }
   }
}

// Draw a string at (x, y) in RGB565 color
static void draw_string(int x, int y, const char *str, uint16_t color) {
   int cx = x;
   for (size_t i = 0; str[i]; i++) {
      draw_char(cx, y, str[i], color);
      cx += 8;
   }
}

// Called by the frontend to set environment callbacks
void retro_set_environment(retro_environment_t cb) {
   environ_cb = cb;
   env_call_count++;
   if (!cb) {
      fallback_log("ERROR", "retro_set_environment: Null environment callback");
      return;
   }

  //  static const struct retro_controller_description controllers[] = {
  //     { "Nintendo DS", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
  //  };
  //  static const struct retro_controller_info ports[] = {
  //     { controllers, 1 },
  //     { NULL, 0 },
  //  };
  //  cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] retro_set_environment called (count: %d)", env_call_count);
   else
      fallback_log_format("DEBUG", "retro_set_environment called (count: %d)", env_call_count);

  // disable start core
  bool contentless = false;
  environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &contentless);

  // enable start core
  //  if (!contentless_set) {
  //     bool contentless = true;
  //     if (environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &contentless)) {
  //        contentless_set = true;
  //        if (log_cb)
  //           log_cb(RETRO_LOG_INFO, "[DEBUG] Content-less support enabled");
  //        else
  //           fallback_log("DEBUG", "Content-less support enabled");
  //     } else {
  //        if (log_cb)
  //           log_cb(RETRO_LOG_ERROR, "[ERROR] Failed to set content-less support");
  //        else
  //           fallback_log("ERROR", "Failed to set content-less support");
  //     }
  //  }
}

// Called by the frontend to set video refresh callback
void retro_set_video_refresh(retro_video_refresh_t cb) {
   video_cb = cb;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Video refresh callback set");
   else
      fallback_log("DEBUG", "Video refresh callback set");
}

// Input callbacks
void retro_set_input_poll(retro_input_poll_t cb) {
   input_poll_cb = cb;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Input poll callback set");
   else
      fallback_log("DEBUG", "Input poll callback set");
}

void retro_set_input_state(retro_input_state_t cb) {
   input_state_cb = cb;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Input state callback set");
   else
      fallback_log("DEBUG", "Input state callback set");
}

// Stubbed callbacks
void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { (void)cb; }

// Called when the core is initialized
void retro_init(void) {
  printf("[[init game]]\n");
   initialized = true;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Hello World core initialized");
   else
      fallback_log("DEBUG", "Hello World core initialized");
   clear_framebuffer();

   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
   if (environ_cb && environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[DEBUG] Pixel format set: RGB565");
      else
         fallback_log("DEBUG", "Pixel format set: RGB565");
   } else {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] Failed to set pixel format: RGB565");
      else
         fallback_log("ERROR", "Failed to set pixel format: RGB565");
      if (environ_cb)
         environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
   }

   struct retro_log_callback logging;
   if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
      log_cb = logging.log;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[DEBUG] Logging callback initialized");
   } else {
      if (log_cb)
         log_cb(RETRO_LOG_WARN, "[WARN] Failed to get log interface");
      else
         fallback_log("WARN", "Failed to get log interface");
   }
}

// Called when the core is deinitialized
void retro_deinit(void) {
   if (zip_path) {
      free(zip_path);
      zip_path = NULL;
      rom_size = 0;
   }
   if (log_file) {
      fclose(log_file);
      log_file = NULL;
   }
   initialized = false;
   contentless_set = false;
   env_call_count = 0;
   square_x = 0;
   square_y = 0;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Core deinitialized");
   else
      fallback_log("DEBUG", "Core deinitialized");
}

// Called to get system information
void retro_get_system_info(struct retro_system_info *info) {
   memset(info, 0, sizeof(*info));
   info->library_name = "Hello World Core Rom";
   info->library_version = "1.0";
   info->need_fullpath = true;
   info->block_extract = true; // this need to load with exts
   info->valid_extensions = "zip";
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] System info: %s v%s, need_fullpath=%d, block_extract=%d",
             info->library_name, info->library_version, info->need_fullpath, info->block_extract);
   else
      fallback_log_format("DEBUG", "System info: %s v%s, need_fullpath=%d, block_extract=%d",
                         info->library_name, info->library_version, info->need_fullpath, info->block_extract);
}

// Called to get system AV information
void retro_get_system_av_info(struct retro_system_av_info *info) {
   memset(info, 0, sizeof(*info));
   info->geometry.base_width = WIDTH;
   info->geometry.base_height = HEIGHT;
   info->geometry.max_width = WIDTH;
   info->geometry.max_height = HEIGHT;
   info->geometry.aspect_ratio = (float)WIDTH / HEIGHT;
   info->timing.fps = 60.0;
   info->timing.sample_rate = 48000.0;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] AV info: %dx%d, %.2f fps", WIDTH, HEIGHT, info->timing.fps);
   else
      fallback_log_format("DEBUG", "AV info: %dx%d, %.2f fps", WIDTH, HEIGHT, info->timing.fps);
}

// Called when the core is loaded
void retro_set_controller_port_device(unsigned port, unsigned device) {
   (void)port;
   (void)device;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Controller port device set: port=%u, device=%u", port, device);
   else
      fallback_log_format("DEBUG", "Controller port device set: port=%u, device=%u", port, device);
}

// Called to reset the core
void retro_reset(void) {
   clear_framebuffer();
   square_x = 0;
   square_y = 0;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Core reset");
   else
      fallback_log("DEBUG", "Core reset");
}

// Called to load a game
bool retro_load_game(const struct retro_game_info *game) {
  printf("[[ LOADING.....%s ]]\n", game->path);

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] retro_load_game started");
   else
      fallback_log("DEBUG", "retro_load_game started");

   if (!game) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] Null game info provided");
      else
         fallback_log("ERROR", "Null game info provided");
      return false;
   }

   if (!game->path) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] No game path provided");
      else
         fallback_log("ERROR", "No game path provided");
      return false;
   }

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Loading game: %s", game->path);
   else
      fallback_log_format("DEBUG", "Loading game: %s", game->path);

   const char *ext = strrchr(game->path, '.');
   if (!ext) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] No file extension found");
      else
         fallback_log("ERROR", "No file extension found");
      return false;
   }

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] File extension: %s", ext);
   else
      fallback_log_format("DEBUG", "File extension: %s", ext);

   if (_stricmp(ext, ".zip") != 0) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] Only ZIP files are supported, got: %s", ext);
      else
         fallback_log_format("ERROR", "Only ZIP files are supported, got: %s", ext);
      return false;
   }

   // Validate ZIP and check for script.lua
   mz_zip_archive zip_archive = {0};
   if (!mz_zip_reader_init_file(&zip_archive, game->path, 0)) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] Failed to open ZIP: %s", game->path);
      else
         fallback_log_format("ERROR", "Failed to open ZIP: %s", game->path);
      return false;
   }

   bool found_lua = false;
   for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
      mz_zip_archive_file_stat file_stat;
      if (mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
         if (_stricmp(file_stat.m_filename, "script.lua") == 0) {
            found_lua = true;
            rom_size = file_stat.m_uncomp_size;
            if (log_cb)
               log_cb(RETRO_LOG_INFO, "[DEBUG] Found script.lua in ZIP, size: %zu bytes", rom_size);
            else
               fallback_log_format("DEBUG", "Found script.lua in ZIP, size: %zu bytes", rom_size);
            break;
         }
      }
   }

   mz_zip_reader_end(&zip_archive);

   if (!found_lua) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] No script.lua found in ZIP: %s", game->path);
      else
         fallback_log_format("ERROR", "No script.lua found in ZIP: %s", game->path);
      return false;
   }

   // Store ZIP path
   if (zip_path) {
      free(zip_path);
      zip_path = NULL;
   }
   zip_path = strdup(game->path);
   if (!zip_path) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] Failed to allocate memory for ZIP path");
      else
         fallback_log("ERROR", "Failed to allocate memory for ZIP path");
      return false;
   }

   // Initialize framebuffer
   clear_framebuffer();
   draw_string(50, 100, "Loaded ZIP with script.lua", COLOR_WHITE);

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] retro_load_game completed, ZIP path stored: %s", zip_path);
   else
      fallback_log_format("DEBUG", "retro_load_game completed, ZIP path stored: %s", zip_path);
   return true;
}

// Called every frame
void retro_run(void) {
   if (!initialized) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] Core not initialized in retro_run");
      else
         fallback_log("ERROR", "Core not initialized in retro_run");
      return;
   }

  //  if (log_cb)
  //     log_cb(RETRO_LOG_INFO, "[DEBUG] retro_run started");
  //  else
  //     fallback_log("DEBUG", "retro_run started");

   clear_framebuffer();

   // Handle input
   if (input_poll_cb)
      input_poll_cb();
   if (input_state_cb) {
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) {
         square_x += 1;
         if (square_x > WIDTH - 20) square_x = WIDTH - 20;
      }
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT)) {
         square_x -= 1;
         if (square_x < 0) square_x = 0;
      }
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN)) {
         square_y += 1;
         if (square_y > HEIGHT - 20) square_y = HEIGHT - 20;
      }
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP)) {
         square_y -= 1;
         if (square_y < 0) square_y = 0;
      }
   }

   // Draw red square
   for (int y = 0; y < 20; y++) {
      for (int x = 0; x < 20; x++) {
         if (square_x + x < WIDTH && square_y + y < HEIGHT)
            framebuffer[(y + square_y) * WIDTH + (x + square_x)] = COLOR_RED;
      }
   }

   // Draw status
   draw_string(50, 50, "Hello World", COLOR_WHITE);

   // Access script.lua from ZIP
   if (zip_path) {
      char status[64];
      snprintf(status, sizeof(status), "ZIP: %s", zip_path);
      draw_string(50, 100, status, COLOR_WHITE);

      // Read script.lua contents
      mz_zip_archive zip_archive = {0};
      if (mz_zip_reader_init_file(&zip_archive, zip_path, 0)) {
         mz_zip_archive_file_stat file_stat;
         for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
            if (mz_zip_reader_file_stat(&zip_archive, i, &file_stat) && _stricmp(file_stat.m_filename, "script.lua") == 0) {
               char *lua_content = (char *)malloc(file_stat.m_uncomp_size + 1);
               if (lua_content) {
                  if (mz_zip_reader_extract_file_to_mem(&zip_archive, file_stat.m_filename, lua_content, file_stat.m_uncomp_size, 0)) {
                     lua_content[file_stat.m_uncomp_size] = '\0';
                     char display_content[64];
                     size_t len = file_stat.m_uncomp_size < 63 ? file_stat.m_uncomp_size : 63;
                     memcpy(display_content, lua_content, len);
                     display_content[len] = '\0';
                     for (size_t j = 0; j < len; j++) {
                        if (display_content[j] < 32 || display_content[j] > 126)
                           display_content[j] = '.';
                     }
                     draw_string(50, 120, display_content, COLOR_WHITE);
                  }
                  free(lua_content);
               }
               break;
            }
         }
         mz_zip_reader_end(&zip_archive);
      } else {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "[ERROR] Failed to reopen ZIP in retro_run: %s", zip_path);
         else
            fallback_log_format("ERROR", "Failed to reopen ZIP in retro_run: %s", zip_path);
      }
   }

   if (video_cb) {
      video_cb(framebuffer, WIDTH, HEIGHT, WIDTH * sizeof(uint16_t));
   } else {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] No video callback set");
      else
         fallback_log("ERROR", "No video callback set");
   }
}

// Called to load special content
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) {
   (void)game_type;
   (void)info;
   (void)num_info;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] retro_load_game_special called (stubbed)");
   else
      fallback_log("DEBUG", "retro_load_game_special called (stubbed)");
   return false;
}

// Called to unload a game
void retro_unload_game(void) {
   if (zip_path) {
      free(zip_path);
      zip_path = NULL;
      rom_size = 0;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[DEBUG] ZIP path freed");
      else
         fallback_log("DEBUG", "ZIP path freed");
   }
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Game unloaded");
   else
      fallback_log("DEBUG", "Game unloaded");
}

// Called to get region
unsigned retro_get_region(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Region: NTSC");
   else
      fallback_log("DEBUG", "Region: NTSC");
   return RETRO_REGION_NTSC;
}

// Stubbed serialization functions
bool retro_serialize(void *data, size_t size) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Serialize called (stubbed)");
   else
      fallback_log("DEBUG", "Serialize called (stubbed)");
   (void)data;
   (void)size;
   return false;
}

bool retro_unserialize(const void *data, size_t size) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Unserialize called (stubbed)");
   else
      fallback_log("DEBUG", "Unserialize called (stubbed)");
   (void)data;
   (void)size;
   return false;
}

size_t retro_serialize_size(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Serialize size: 0");
   else
      fallback_log("DEBUG", "Serialize size: 0");
   return 0;
}

// Stubbed cheat functions
void retro_cheat_reset(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Cheat reset (stubbed)");
   else
      fallback_log("DEBUG", "Cheat reset (stubbed)");
}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Cheat set: index=%u, enabled=%d, code=%s (stubbed)", index, enabled, code);
   else
      fallback_log_format("DEBUG", "Cheat set: index=%u, enabled=%d, code=%s (stubbed)", index, enabled, code);
}

// Stubbed memory functions
void *retro_get_memory_data(unsigned id) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Memory data: id=%u (stubbed)", id);
   else
      fallback_log_format("DEBUG", "Memory data: id=%u (stubbed)", id);
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Memory size: id=%u (stubbed)", id);
   else
      fallback_log_format("DEBUG", "Memory size: id=%u (stubbed)", id);
   (void)id;
   return 0;
}

// Called to get API version
unsigned retro_api_version(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] API version: %u", RETRO_API_VERSION);
   else
      fallback_log_format("DEBUG", "API version: %u", RETRO_API_VERSION);
   return RETRO_API_VERSION;
}