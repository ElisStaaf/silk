#ifndef SILK_COPY_DIRECTORY_H
#define SILK_COPY_DIRECTORY_H

#include "file_it.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Recursively copy the content of the directory in another one, empty directories will be omitted. */
SILK_API silk_bool silk_copy_directory(const char* source_dir, const char* target_dir);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SILK_COPY_DIRECTORY_H */

#ifdef SILK_IMPLEMENTATION

SILK_API silk_bool
silk_copy_directory(const char* source_dir, const char* target_dir)
{
	silk_file_it it;
	silk_bool result = silk_true;
	silk_size tmp_save = 0; 
	char* dest_buffer = NULL;
	silk_size n = 0;
	const char* source_relative_path = NULL;

	tmp_save = silk_tmp_save();
	dest_buffer = silk_tmp_alloc(SILK_MAX_PATH);
	silk_file_it_init_recursive(&it, source_dir);

	while (silk_file_it_get_next(&it))
	{
		/* copy current directory */
		source_relative_path = it.current_file + it.dir_len_stack[1];

		n = 0;
		n += silk_str_append_from(dest_buffer, target_dir, n, SILK_MAX_PATH);
		n += silk_ensure_trailing_dir_separator(dest_buffer, n);
		n += silk_str_append_from(dest_buffer, source_relative_path, n, SILK_MAX_PATH);

		if(!silk_copy_file(it.current_file, dest_buffer))
		{
			silk_log_error("Could not copy directory '%s' to '%s'", source_dir, target_dir);
			silk_set_and_goto(result, silk_false, exit);
		}
	}

exit:

	silk_tmp_restore(tmp_save);
	return result;
}

#endif /* SILK_IMPLEMENTATION */
