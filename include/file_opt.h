#ifndef FILE_OPT_H
#define FILE_OPT_H

int read_file_to_buffer(FILE *src_fp, char *buffer, int buffer_size);

int open_write_file(const char *filename, const char *mode, char *data, int size);
int open_read_file(const char *filename, const char *mode, char *data, int size);
int open_write_file_mul(const char *filename, const char *mode, char *data, int size, int num);

int clear_file(const char *filename);
int get_size_file(const char *filename);

#endif // FILE_OPT_H