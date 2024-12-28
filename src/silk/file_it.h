#ifndef SILK_FILE_IT_H
#define SILK_FILE_IT_H

#if defined(_WIN32)
#define SILK_INVALID_FILE_HANDLE INVALID_HANDLE_VALUE
#else
#define SILK_INVALID_FILE_HANDLE NULL
#endif

#define SILK_MAX_DIR_DEPTH 256

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
typedef HANDLE handle_t;
#else
typedef DIR* handle_t;
#endif

/* File iterator. Can be recursive. */
typedef struct silk_file_it silk_file_it;
struct silk_file_it {
	silk_bool recursive;
	silk_bool has_next;

	/* Stack used for recursion. */
	char current_file[SILK_MAX_PATH];

	/* Stack used for recursion, first item has 0 len, second item contains the lenght of the initial directory */
	silk_size dir_len_stack[SILK_MAX_DIR_DEPTH];
	silk_size stack_size;

	handle_t handle_stack[SILK_MAX_DIR_DEPTH];
#if defined(_WIN32)
	WIN32_FIND_DATAA find_data;
#else
	struct dirent* find_data;
#endif
};

SILK_API void silk_file_it_init(silk_file_it* it, const char* base_directory);

SILK_API void silk_file_it_init_recursive(silk_file_it* it, const char* base_directory);

SILK_API void silk_file_it_destroy(silk_file_it* it);

SILK_API const char* silk_file_it_current_file(silk_file_it* it);

SILK_API silk_bool silk_file_it_get_next(silk_file_it* it);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SILK_FILE_IT_H */

#ifdef SILK_IMPLEMENTATION

SILK_INTERNAL void
silk_file_it__push_dir(silk_file_it* it, const char* directory)
{
	silk_size new_directory_len = 0;
	silk_size n = 0;
	handle_t handle = SILK_INVALID_FILE_HANDLE;

	n += it->dir_len_stack[it->stack_size];
	n += silk_str_append_from(it->current_file, directory, n, SILK_MAX_PATH);
	n += silk_ensure_trailing_dir_separator(it->current_file, n);
	new_directory_len = n;

#if defined(_WIN32)

	/* add asterisk to read all files from the directory */
	silk_str_append_from(it->current_file, "*", n, SILK_MAX_PATH);
	handle = FindFirstFileA(it->current_file, &it->find_data);

	if (handle == SILK_INVALID_FILE_HANDLE)
	{
		silk_log_error("Could not open directory '%s'", it->current_file);

		it->has_next = silk_false;
		return;
	}

	/* remove the last asterisk '*' */
	it->current_file[new_directory_len] = '\0';

#else
	handle = opendir(it->current_file);

	if (handle == SILK_INVALID_FILE_HANDLE)
	{
		silk_log_error("Could not open directory '%s': %s.", directory, strerror(errno));
		it->has_next = silk_false;
		return;
	}

#endif
	it->stack_size++;

	it->handle_stack[it->stack_size] = handle;

	it->dir_len_stack[it->stack_size] = new_directory_len;

	it->has_next = silk_true;
}

SILK_INTERNAL void
silk_file_it_close_current_handle(silk_file_it* it)
{
	if (it->handle_stack[it->stack_size] != SILK_INVALID_FILE_HANDLE)
	{
#if defined(_WIN32)
		FindClose(it->handle_stack[it->stack_size]);
#else
		closedir(it->handle_stack[it->stack_size]);
#endif
		it->handle_stack[it->stack_size] = SILK_INVALID_FILE_HANDLE;
	}
}

SILK_INTERNAL const char*
silk_file_it__get_next_entry(silk_file_it* it)
{
	/* No more entries */
	if (it->stack_size == 0) { return NULL; }
#if defined(_WIN32)

	BOOL b = FindNextFileA(it->handle_stack[it->stack_size], &it->find_data);
	if (!b)
	{
		DWORD err = GetLastError();
		if (err != ERROR_SUCCESS && err != ERROR_NO_MORE_FILES)
		{
			silk_log_error("Could not go to the next entry.");
		}
		return NULL;
	}
	return it->find_data.cFileName;
#else
	it->find_data = readdir(it->handle_stack[it->stack_size]);

	return it->find_data != NULL ? it->find_data->d_name : NULL;
#endif
}

SILK_INTERNAL silk_bool
silk_file_it__current_entry_is_directory(silk_file_it* it)
{
#if defined(_WIN32)
	return !!(it->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else
	/* @FIXME: in c89 DT_DIR is not defined so we explicitely define it, this is not portable though. */
#ifndef DT_DIR
#define DT_DIR 4
#endif 
	return it->find_data->d_type == DT_DIR;
#endif
}

SILK_INTERNAL void
silk_file_it__pop_dir(silk_file_it* it)
{
	silk_size dir_len = 0;

	if (it->stack_size > 0)
	{
		silk_file_it_close_current_handle(it);

		it->stack_size -= 1;

		dir_len = it->dir_len_stack[it->stack_size];
		it->current_file[dir_len] = '\0'; /* set null term char */

		if (it->stack_size == 0)
		{
			it->has_next = silk_false;
		}
	}
}

SILK_API void
silk_file_it_init(silk_file_it* it, const char* base_directory)
{
	memset(it, 0, sizeof(silk_file_it));
	it->stack_size = 0;
	it->current_file[it->stack_size] = '\0';
	silk_file_it__push_dir(it, base_directory);
	
	it->recursive = silk_false;
}

SILK_API void
silk_file_it_init_recursive(silk_file_it* it, const char* base_directory)
{
	silk_file_it_init(it, base_directory);
	it->recursive = silk_true;
}

SILK_API void
silk_file_it_destroy(silk_file_it* it)
{
	while (it->stack_size > 0)
	{
		silk_file_it_close_current_handle(it);
		it->stack_size -= 1;
	}

	it->current_file[0] = '\0';
	it->has_next = silk_false;
}

SILK_API const char*
silk_file_it_current_file(silk_file_it* it)
{
	return it->current_file;
}

SILK_API silk_bool
silk_file_it_get_next(silk_file_it* it)
{
	silk_bool is_directory = silk_false;
	const char* found = 0;

	SILK_ASSERT(it->has_next);

	do
	{
		/* Check if there is remaining/next file, if not, just pop the stack.
		*  Do it multiple times if we are at the end of a directory and a parent directory.
		*/
		while ((found = silk_file_it__get_next_entry(it)) == NULL)
		{
			/* no parent directory so it's the end */
			if (it->stack_size == 0)
			{
				it->has_next = silk_false;
				return silk_false;
			}

			/* since there is no more file in the directory
			   we pop the directory and loop again to get the next file
			*/
			silk_file_it__pop_dir(it);
		}

		is_directory = silk_file_it__current_entry_is_directory(it);
	
		/* skip parent directory or current directory ..' or '.'*/
	} while (it->has_next
		&& is_directory
		&& found[0] == '.');

	/* build path with current file found */
	silk_str_append_from(it->current_file, found, it->dir_len_stack[it->stack_size], SILK_MAX_PATH);
	
	if (is_directory && it->recursive)
	{
		silk_file_it__push_dir(it, found);
	}

	return silk_true;
}

#endif /* SILK_IMPLEMENTATION */
