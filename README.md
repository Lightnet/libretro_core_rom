# libretro_core_rom

# License : MIT

# Information:
  Sample test for library miniz to load content aka rom or zip file test.

  This has software render test sample.

  Current set for zip base on miniz lib for easy zip read file.

# Libretro core api:

```c
// Called to get system information
void retro_get_system_info(struct retro_system_info *info) {
   memset(info, 0, sizeof(*info));
   info->library_name = "Libretro Core API Rom";
   info->library_version = "1.0";
   info->need_fullpath = true; // need to file path to read. 
   info->block_extract = true; // skip extract default
   info->valid_extensions = "zip"; // file types "rom | zip"
}
```
There is docs api reference and github samples.

# Guide:
- Main Menu > 
- load core
  - hello_world_core.dll
- local content
  - script.zip

# Bugs:
  There might be crash or frezze.

## zip:

script.lua
```
print("hello world text.")
```
  command zip
```
zip script.zip script.lua
``` 
  Use 7zip should be fine as long there no sub folder.


# Links:
 * https://github.com/libretro/libretro-samples/blob/master/midi/midi_test/libretro.c
 * https://github.com/libretro/RetroArch/issues/16802

