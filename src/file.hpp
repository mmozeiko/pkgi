#pragma once

#include <string>
#include <vector>

#include <cstdint>

void pkgi_mkdirs(const char* path);
void pkgi_rm(const char* file);
void pkgi_delete_dir(const std::string& path);
int64_t pkgi_get_size(const char* path);

int pkgi_file_exists(const char* path);
void pkgi_rename(const char* from, const char* to);

// creates file (if it exists, truncates size to 0)
void* pkgi_create(const char* path);
// open existing file in read/write, fails if file does not exist
void* pkgi_openrw(const char* path);
// open file for writing, next write will append data to end of it
void* pkgi_append(const char* path);

void pkgi_close(void* f);

int64_t pkgi_seek(void* f, uint64_t offset);
int pkgi_read(void* f, void* buffer, uint32_t size);
int pkgi_write(void* f, const void* buffer, uint32_t size);

std::vector<uint8_t> pkgi_load(const std::string& path);
void pkgi_save(const std::string& path, const void* data, uint32_t size);

std::vector<std::string> pkgi_list_dir_contents(const std::string& path);
