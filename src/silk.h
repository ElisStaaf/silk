#ifndef RE_SILK_H
#define RE_SILK_H

#define SILK_IMPLEMENTATION

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h> /* va_start, va_end */

#if _WIN32
	#if !defined _CRT_SECURE_NO_WARNINGS
		#define _CRT_SECURE_NO_WARNINGS
	#endif
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <io.h> /* _get_oshandle */
	#define SILK_THREAD  __declspec( thread )
#else
	#include <unistd.h>   /* open, close, access */
    #include <sys/stat.h> /* mkdir */
	#include <fcntl.h>    /* O_RDONLY etc. */
	#include <errno.h>
	#include <sys/sendfile.h> /* sendfile */
	#include <sys/wait.h>     /* waitpid */
	#include <dirent.h>       /* opendir */

	#define SILK_THREAD __thread
#endif

/* Suppress some MSVC warnings. */
#ifdef _MSC_VER
/* byte padding */
#pragma warning(disable:4820)
/* Spectre mitigation */
#pragma warning(disable:5045)
#endif

/* in c89 va_copy does not exist */
#if defined(__GNUC__) || defined(__clang__)
#ifndef va_copy
#define va_copy(dest, src) (__builtin_va_copy(dest, src))
#endif
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#ifndef SILK_C99_OR_LATER
#define SILK_C99_OR_LATER
#endif
#endif

#ifndef SILK_API
#define SILK_API
#endif
#ifndef SILK_INTERNAL
#define SILK_INTERNAL static
#endif

#ifndef SILK_ASSERT
#include <assert.h> /* assert */
#define SILK_ASSERT assert
#endif

#ifndef SILK_MALLOC
#include <stdlib.h> /* malloc */
#define SILK_MALLOC malloc
#endif

#ifndef SILK_FREE
#include <stdlib.h> /* free */
#define SILK_FREE free
#endif

#define silk_true ((silk_bool)1)
#define silk_false ((silk_bool)0)

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int silk_id; /* hashed key, must be unsigned */
typedef unsigned int silk_bool;
typedef size_t silk_size;

/* All various typedef since c89 does not allow typedef redefinition */
typedef struct silk_toolchain silk_toolchain;
typedef struct silk_project_t silk_project_t;
typedef struct silk_strv silk_strv;
typedef struct silk_darr silk_darr;
typedef struct silk_kv silk_kv;
typedef struct silk_context silk_context;
typedef struct silk_process_handle silk_process_handle;

SILK_API void silk_init(void);
SILK_API void silk_destroy(void);

/* Delete projects */
SILK_API void silk_clear(void);

/* Set or create current project.  */
SILK_API silk_project_t* silk_project(const char* name); 

/* Wrapper around silk_project with string formatting. */
SILK_API silk_project_t* silk_project_f(const char* format, ...);

/* Add value for the specific key. */
SILK_API void silk_add(const char* key, const char* value);

/* Wrapper around silk_set with string formatting. */
SILK_API void silk_add_f(const char* key, const char* format, ...);

/* Add multiple string values. The last value must be a null value. */
SILK_API void silk_add_many_vnull(const char* key, ...);

/* Add multiple values using var args macro. */
#ifdef SILK_C99_OR_LATER
#define silk_add_many_v(key, ...) \
	silk_add_many(key \
    , (const char* []) { __VA_ARGS__ } \
	, (sizeof((const char* []) { __VA_ARGS__ }) / sizeof(const char*)))
#endif

/* Remove all previous values according to the key and set the new one. */
SILK_API void silk_set(const char* key, const char* value);

/* Wrapper around silk_set with string formatting. */
SILK_API void silk_set_f(const char* key, const char* format, ...);

/* Remove all values associated with the key. Returns number of removed values. */
SILK_API silk_size silk_remove_all(const char* key);

/* Wrapper around silk_remove_all with string formatting. */
SILK_API silk_size silk_remove_all_f(const char* format, ...);

/* Remove item with the exact key and value. */
SILK_API silk_bool silk_remove_one(const char* key, const char* value);

/* Wrapper around silk_remove_one with string formatting. */
SILK_API silk_bool silk_remove_one_f(const char* key, const char* format, ...);

/* Check if key/value already exists in the current project. */
SILK_API silk_bool silk_contains(const char* key, const char* value);

/* Returns the name of the result, which could be the path of a library or any other value depending on the toolchain. */
typedef const char* (*silk_toolchain_bake_t)(silk_toolchain* tc, const char*);

struct silk_toolchain {
	silk_toolchain_bake_t bake;
	/* Name of the toolchain, mostly for debugging purpose. */
	const char* name;
	/* Name of the default directory. */
	const char* default_directory_base;
};

/* Process the current project using the default toolchain.
   Returns the name of the result, which could be the path of a library or any other value depending on the toolchain.
   Use the default toolchain (gcc or msvc).
*/
SILK_API const char* silk_bake(void);

/* Same as silk_bake. Using an explicit projectname. */
SILK_API const char* silk_bake_project(const char* project_name);

/* Same as silk_bake. Take an explicit toolchain instead of using the default one. */
SILK_API const char* silk_bake_project_with(silk_toolchain toolchain, const char* project_name);

/* Run executable path. Path is double quoted before being run, in case path contains some space.
   Returns exit code. Returns -1 if command could not be executed.
*/
SILK_API int silk_run(const char* cmd);

/* Turn on/off debug messages */
SILK_API void silk_debug(silk_bool value);

SILK_API silk_toolchain silk_toolchain_default(void);

/* Run command and returns exit code. */
SILK_API int silk_process(const char* cmd);

/* Same as silk_process but provide a starting directory. */
SILK_API int silk_process_in_directory(const char* cmd, const char* directory);

/* Start command and wait for the process to end.
   stdout of the child process is printed into a buffer and accessible from silk_process_stdout_string(handle),
   if 'also_get_stderr' is true the stderr is printed to a buffer as well and is accessed from silk_process_stderr_string(handle).
   silk_process_end(handle) needs to be called to cleanup various resources.
   silk_process_stdout_string(handle) or silk_process_stderr_string(handle) are not accessible after silk_process_end(handle). */
SILK_API silk_process_handle* silk_process_to_string(const char* cmd, const char* starting_directory, silk_bool also_get_stderr);

/* Get c string content of stdout of the child process after silk_process_to_string has been called. */
SILK_API const char* silk_process_stdout_string(silk_process_handle* handle);

/* Get c string content of stderr of the child process after silk_process_to_string has been called. */
SILK_API const char* silk_process_stderr_string(silk_process_handle* handle);

/* Returns exit code of the process and clean up resources. */
SILK_API int silk_process_end(silk_process_handle* handle);

/* Commonly used properties (basically to make it discoverable with auto completion and avoid misspelling) */

/* keys */
extern const char* silk_BINARY_TYPE;         /* Exe, shared_lib or static_lib. */
extern const char* silk_CXFLAGS;             /* Extra flags to give to the C/C++ compiler. */
extern const char* silk_DEFINES;             /* Define preprocessing symbol. */
extern const char* silk_FILES;               /* Files to consume (could be .c, .cpp, etc.). */
extern const char* silk_INCLUDE_DIRECTORIES; /* Include directories. */
extern const char* silk_LINK_PROJECTS;       /* Other projects to link. */
extern const char* silk_LIBRARIES;           /* Libraries to link with. */
extern const char* silk_LFLAGS;              /* Extra flags to give to the linker. */
extern const char* silk_OUTPUT_DIR;          /* Ouput directory for the generated files. */
extern const char* silk_TARGET_NAME;         /* Name (basename) of the main generated file (.exe, .a, .lib, .dll, etc.). */
/* values */
extern const char* silk_EXE;                 /* silk_BINARY_TYPE value */
extern const char* silk_SHARED_LIBRARY;      /* silk_BINARY_TYPE value */
extern const char* silk_STATIC_LIBRARY;      /* silk_BINARY_TYPE value */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RE_SILK_H */

#ifdef SILK_IMPLEMENTATION
#ifndef SILK_IMPLEMENTATION_CPP
#define SILK_IMPLEMENTATION_CPP

#ifdef _WIN32
#define SILK_DEFAULT_DIR_SEPARATOR_CHAR '\\'
#define SILK_DEFAULT_DIR_SEPARATOR "\\"
#else
#define SILK_DEFAULT_DIR_SEPARATOR_CHAR '/'
#define SILK_DEFAULT_DIR_SEPARATOR "/"
#endif

#define SILK_PREFERRED_DIR_SEPARATOR_CHAR '/'
#define SILK_PREFERRED_DIR_SEPARATOR "/"
/* Table of content
*  ----------------------------------------------------------------
* 
* # Structures
*   - @TODO
* # Functions of internal structures
*   - silk_log     - write some logs
*   - silk_darr    - dynamic array
*   - silk_dstr    - dynamic string
*   - silk_kv      - key value for the multimap
*   - silk_mmap    - multimap containing key/value strings
* # Functions of the silk library
*   - silk_project(...) - @TODO explanation
*   - silk_set(...)     - @TODO explanation
*   - silk_add(...)     - @TODO explanation
*   - silk_bake(...)    - @TODO explanation
* # Toolchain
*   - msvc
*   - gcc
*/

/* keys */
const char* silk_BINARY_TYPE = "binary_type";
const char* silk_CXFLAGS = "cxflags";
const char* silk_DEFINES = "defines";
const char* silk_FILES = "files";
const char* silk_INCLUDE_DIRECTORIES = "include_directories";
const char* silk_LINK_PROJECTS = "link_projects";
const char* silk_LIBRARIES = "libraries";
const char* silk_LFLAGS = "lflags";
const char* silk_OUTPUT_DIR = "output_dir";
const char* silk_TARGET_NAME = "target_name";
const char* silk_WORKING_DIRECTORY = "working_directory";
/* values */
const char* silk_EXE = "exe";
const char* silk_SHARED_LIBRARY = "shared_library";
const char* silk_STATIC_LIBRARY = "static_library";

/* string view */
struct silk_strv {
	silk_size size;
	const char* data;
};

/* dynamic array
 * NOTE: silk_darr needs to start with the same component as silk_strv
 * because we need to compare them in the same way in silk_kv 
 */
struct silk_darr {
	silk_size size;
    char* data;
	silk_size capacity;
};

/* type safe dynamic array */
#define silk_darrT(type)        \
    union {                   \
        silk_darr base;         \
        struct  {             \
            silk_size size;     \
			type* data;       \
            silk_size capacity; \
        } darr;               \
    }

#define silk_darr_push_back(a, value) \
    do { \
        silk_darr* array_ = a; \
        void* value_ref_ = &(value); \
        silk_darr_insert_one(array_, array_->size, value_ref_, sizeof(value)); \
    } while (0)

#define silk_darrT_init(a) \
    silk_darr_init(&(a)->base)

#define silk_darrT_destroy(a) \
    silk_darr_destroy(&(a)->base)

#define silk_darrT_insert(a, index, value) \
    do {  \
        silk_darr_insert_one_space(&(a)->base, (index), sizeof(*(a)->darr.data)); \
		(a)->darr.data[(index)] = value; \
	} while (0)

#define silk_darrT_remove(a, index) \
    do {  \
        silk_darr_remove_one(&(a)->base, (index), sizeof(*(a)->darr.data)); \
	} while (0)

#define silk_darrT_push_back(a, value) \
    do {  \
        silk_size last__ = (a)->darr.size; \
        silk_darr_insert_one_space(&(a)->base, last__, sizeof(*(a)->darr.data)); \
		(a)->darr.data[last__] = value; \
	} while (0)

#define silk_darrT_at(a, index) \
    ((a)->darr.data[index])

#define silk_darrT_set(a, index, value) \
    (a)->darr.data[index] = (value)

#define silk_darrT_ptr(a, index) \
    (&(a)->darr.data[index])

#define silk_darrT_size(a) \
    ((a)->darr.size)

/* dynamic string */
typedef silk_darr silk_dstr;

/* key/value data used in the map and mmap struct */
struct silk_kv {
	silk_id hash;  /* hash of the key */
	silk_strv key; /* key */
	union {
		const void* ptr;
		silk_strv strv;
	} u; /* value */
};

/* multimap */
typedef silk_darrT(silk_kv) silk_mmap;

#define silk_rangeT(type) \
struct {                \
	type* begin;        \
	type* end;          \
	silk_size count;      \
}

typedef silk_rangeT(silk_kv) silk_kv_range;

struct silk_project_t {
	silk_strv name;
	/* @FIXME: rename this "props" or "properties". */
	silk_mmap mmap; /* multi map of strings - when you want to have multiple values per key */
};

/* context, the root which hold everything */
struct silk_context {
	silk_mmap projects;
	silk_project_t* current_project;
};

static silk_context default_ctx;
static silk_context* current_ctx;

/*-----------------------------------------------------------------------*/
/* utils */
/*-----------------------------------------------------------------------*/

static silk_bool silk_debug_enabled = silk_false;

#define silk_set_and_goto(result, value, goto_label) \
	do { \
		result = (value); \
		goto goto_label; \
	} while(0)

/*-----------------------------------------------------------------------*/
/* silk_log */
/*-----------------------------------------------------------------------*/

SILK_INTERNAL void
silk_log_v(FILE* file, const char* prefix, const char* fmt, va_list args)
{
	va_list args_copy;
	va_copy(args_copy, args);

	fprintf(file, "%s", prefix);
	vfprintf(file, fmt, args_copy);
	fprintf(file, "\n");
}

SILK_INTERNAL void
silk_log_error(const char* fmt, ...) { va_list args; va_start(args, fmt); silk_log_v(stderr, "[SILK-ERROR] ", fmt, args); va_end(args); }
SILK_INTERNAL void
silk_log_warning(const char* fmt, ...) { va_list args; va_start(args, fmt); silk_log_v(stderr, "[SILK-WARNING] ", fmt, args); va_end(args); }
SILK_INTERNAL void
silk_log_debug(const char* fmt, ...) { va_list args; va_start(args, fmt); if (silk_debug_enabled) { silk_log_v(stdout, "[SILK-DEBUG] ", fmt, args); } va_end(args); }
SILK_INTERNAL void
silk_log_important(const char* fmt, ...) { va_list args; va_start(args, fmt); silk_log_v(stdout, "", fmt, args); va_end(args); }

/*-----------------------------------------------------------------------*/
/* temporary allocation */
/*-----------------------------------------------------------------------*/

#ifndef SILK_TMP_CAPACITY
#define SILK_TMP_CAPACITY (8 * 1024 * 1024) /* Arbitrary size. Must be big enough. Can be increased if needed. */
#endif

static SILK_THREAD silk_size silk_tmp_size = 0;
static SILK_THREAD char silk_tmp_buffer[SILK_TMP_CAPACITY] = { 0 };

SILK_INTERNAL void*
silk_tmp_alloc(silk_size size)
{
	void* data = NULL;

	SILK_ASSERT((silk_tmp_size + size <= SILK_TMP_CAPACITY) && "Size of the temporary allocator is too small. Increase it if needed.");

	if (silk_tmp_size + size > SILK_TMP_CAPACITY)
	{
		return NULL;
	}

	data = &silk_tmp_buffer[silk_tmp_size];
	silk_tmp_size += size;
	return data;
}

SILK_INTERNAL void*
silk_tmp_calloc(silk_size size)
{
	void* data = silk_tmp_alloc(size);
	memset(data, 0, size);
	return data;
}

SILK_INTERNAL void
silk_tmp_reset(void)
{
	silk_tmp_size = 0;
}

SILK_INTERNAL silk_strv
silk_tmp_strv_vprintf(const char* format, va_list args)
{
	silk_strv sv;
	va_list args_copy;
	int n = 0;
	va_copy(args_copy, args);

	n = vsnprintf(NULL, 0, format, args);

	sv.data = silk_tmp_alloc(n + 1);

	vsnprintf((char*)sv.data, n + 1, format, args_copy);
	sv.size = n;
	return sv;
}

SILK_INTERNAL const char*
silk_tmp_vsprintf(const char* format, va_list args)
{
	silk_strv sv;
	va_list args_copy;
	va_copy(args_copy, args);
	sv = silk_tmp_strv_vprintf(format, args_copy);
	return sv.data;
}

SILK_INTERNAL const char*
silk_tmp_sprintf(const char* format, ...)
{
	va_list args;
	const char* data = NULL;
	va_start(args, format);
	data = silk_tmp_vsprintf(format, args);
	va_end(args);
	return data;
}

SILK_INTERNAL const char*
silk_tmp_strv_to_str(silk_strv sv)
{
	char* data = silk_tmp_alloc(sv.size + 1);
	memcpy(data, sv.data, sv.size + 1);
	data[sv.size] = '\0';
	return data;
}

SILK_INTERNAL silk_strv
silk_tmp_str_to_strv(const char* str)
{
	silk_strv sv;
	sv.size = strlen(str);
	sv.data = silk_tmp_alloc(sv.size + 1);
	memcpy((char*)sv.data, str, sv.size + 1);
	((char*)sv.data)[sv.size] = '\0';
	return sv;
}

SILK_INTERNAL const char*
silk_tmp_str(const char* str)
{
	silk_strv sv = silk_tmp_str_to_strv(str);
	return sv.data;
}

SILK_INTERNAL silk_size
silk_tmp_save(void)
{
	return silk_tmp_size;
}

SILK_INTERNAL void
silk_tmp_restore(silk_size index)
{
	silk_tmp_size = index;
}

/* Check if pointer is contained in the tmp buffer */
SILK_INTERNAL silk_bool
silk_tmp_contains(const void* ptr)
{
	return ptr >= (void*)silk_tmp_buffer
		&& ptr < (void*)(silk_tmp_buffer + SILK_TMP_CAPACITY);
}

/*-----------------------------------------------------------------------*/
/* silk_darr - dynamic array */
/*-----------------------------------------------------------------------*/

SILK_INTERNAL void
silk_darr_init(silk_darr* arr)
{
	arr->size = 0;
	arr->capacity = 0;
	arr->data = NULL;
}

SILK_INTERNAL void
silk_darr_destroy(silk_darr* arr)
{
	if (arr->data != NULL)
	{
		arr->size = 0;
		arr->capacity = 0;
		SILK_FREE(arr->data);
		arr->data = NULL;
	} 
}

SILK_INTERNAL void*
silk_darr_ptr(const silk_darr* arr, silk_size index, silk_size sizeof_vlaue)
{
	SILK_ASSERT(
		index < arr->size /* Within accessible item range */
		|| index == arr->size /* Also allow getting the item at the end */
	);

	return arr->data + (index * sizeof_vlaue);
}

SILK_INTERNAL char* silk_darr_end(const silk_darr* arr, silk_size sizeof_value) { return arr->data + (arr->size * sizeof_value); }

SILK_INTERNAL silk_size
silk_darr__get_new_capacity(const silk_darr* arr, silk_size count)
{
	silk_size new_capacity = arr->capacity ? (arr->capacity + arr->capacity / 2) : 8;
	return new_capacity > count ? new_capacity : count;
}

SILK_INTERNAL void
silk_darr_reserve(silk_darr* arr, silk_size new_capacity, silk_size sizeof_value)
{
	char* new_data = NULL;

	SILK_ASSERT(new_capacity > arr->capacity && "You should request more capacity, not less."); /* ideally we should ensure this before this call. */
	if (new_capacity <= arr->capacity)
	{
		return;
	}

	new_data = (char*)SILK_MALLOC(new_capacity * sizeof_value);
	SILK_ASSERT(new_data);
	if (arr->data != NULL) {
		memcpy(new_data, arr->data, arr->size * sizeof_value);
		SILK_FREE(arr->data);
	}
	arr->data = new_data;
	arr->capacity = new_capacity;
}

SILK_INTERNAL void
silk_darr__grow_if_needed(silk_darr* arr, silk_size needed, silk_size sizeof_value)
{
	if (needed > arr->capacity)
		silk_darr_reserve(arr, silk_darr__get_new_capacity(arr, needed), sizeof_value);
}

SILK_INTERNAL void
silk_darr_insert_many_space(silk_darr* arr, silk_size index, silk_size count, silk_size sizeof_value)
{
	silk_size count_to_move;

	SILK_ASSERT(arr != NULL);
	SILK_ASSERT(index <= arr->size);

	count_to_move = arr->size - index;
	silk_darr__grow_if_needed(arr, arr->size + count, sizeof_value);

	if (count_to_move > 0)
	{
		memmove(
			silk_darr_ptr(arr, index + count, sizeof_value),
			silk_darr_ptr(arr, index, sizeof_value),
			count_to_move * sizeof_value);
	}

	arr->size += count;
}

SILK_INTERNAL void
silk_darr_insert_one_space(silk_darr* arr, silk_size index, silk_size sizeof_value)
{
	silk_darr_insert_many_space(arr, index, 1, sizeof_value);
}

SILK_INTERNAL void
silk_darr_insert_many(silk_darr* arr, silk_size index, const void* value, silk_size count, silk_size sizeof_value)
{
	silk_darr_insert_many_space(arr, index, count, sizeof_value);

	memcpy(silk_darr_ptr(arr, index, sizeof_value), value, count * sizeof_value);
}

SILK_INTERNAL void
silk_darr_insert_one(silk_darr* arr, silk_size index, const void* value, silk_size sizeof_value)
{
	silk_darr_insert_many(arr, index, value, 1, sizeof_value);
}

SILK_INTERNAL void
silk_darr_push_back_many(silk_darr* arr, const void* values_ptr, silk_size count, silk_size sizeof_value)
{
	silk_darr_insert_many(arr, arr->size, values_ptr, count, sizeof_value);
}

SILK_INTERNAL void
silk_darr_remove_many(silk_darr* arr, silk_size index, silk_size count, silk_size sizeof_value)
{
	SILK_ASSERT(arr);
	SILK_ASSERT(index < arr->size);
	SILK_ASSERT(count <= arr->size);
	SILK_ASSERT(index + count <= arr->size);

	if (count <= 0)
		return;

	memmove(
		silk_darr_ptr(arr, index, sizeof_value),
		silk_darr_ptr(arr, index + count, sizeof_value),
		(arr->size - (index + count)) * sizeof_value
	);

	arr->size -= count;
}

SILK_INTERNAL void
silk_darr_remove_one(silk_darr* arr, silk_size index, silk_size sizeof_value)
{
	silk_darr_remove_many(arr, index, 1, sizeof_value);
}

typedef silk_bool(*silk_predicate_t)(const void* left, const void* right);

SILK_INTERNAL silk_size
silk_lower_bound_predicate(const void* void_ptr, silk_size left, silk_size right, const void* value, silk_size sizeof_value, silk_predicate_t pred)
{
	const char* ptr = (const char*)void_ptr;
	silk_size count = right - left;
	silk_size step;
	silk_size mid; /* index of the found value */

	SILK_ASSERT(left <= right);

	while (count > 0) {
		step = count >> 1; /* count divide by two using bit shift */

		mid = left + step;

		if (pred(ptr + (mid * sizeof_value), value)) {
			left = mid + 1;
			count -= step + 1;
		}
		else {
			count = step;
		}
	}
	return left;
}

/*-----------------------------------------------------------------------*/
/* silk_strv - string view */
/*-----------------------------------------------------------------------*/

SILK_INTERNAL silk_strv
silk_strv_make(const char* data, silk_size size)
{
	silk_strv s;
	s.data = data;
	s.size = size; 
	return s;
}

SILK_INTERNAL silk_strv
silk_strv_make_str(const char* str)
{
	return silk_strv_make(str, strlen(str));
}

SILK_INTERNAL int
silk_lexicagraphical_cmp(const char* left, silk_size left_count, const char* right, silk_size right_count)
{
	char left_ch, right_ch;
	silk_size min_size = left_count < right_count ? left_count : right_count;
	while (min_size-- > 0)
	{
		left_ch = *left++;
		right_ch = *right++;
		if (left_ch != right_ch)
			return left_ch < right_ch ? 1 : -1;
	};

	return left_count == right_count
		? 0
		: left_count > right_count ?  1 : -1;
}

SILK_INTERNAL int silk_strv_compare(silk_strv sv, const char* data, silk_size size) { return silk_lexicagraphical_cmp(sv.data, sv.size, data, size); }
SILK_INTERNAL int silk_strv_compare_strv(silk_strv sv, silk_strv other) { return silk_strv_compare(sv, other.data, other.size); }
SILK_INTERNAL int silk_strv_compare_str(silk_strv sv, const char* str) { return silk_strv_compare(sv, str, strlen(str)); }
SILK_INTERNAL silk_bool silk_strv_equals(silk_strv sv, const char* data, silk_size size) { return silk_strv_compare(sv, data, size) == 0; }
SILK_INTERNAL silk_bool silk_strv_equals_strv(silk_strv sv, silk_strv other) { return silk_strv_compare_strv(sv, other) == 0; }
SILK_INTERNAL silk_bool silk_strv_equals_str(silk_strv sv, const char* other) { return silk_strv_compare_strv(sv, silk_strv_make_str(other)) == 0; }

/*-----------------------------------------------------------------------*/
/* silk_str - c string utilities */
/*-----------------------------------------------------------------------*/

SILK_INTERNAL silk_bool silk_str_equals(const char* left, const char* right) { return silk_strv_equals_strv(silk_strv_make_str(left), silk_strv_make_str(right)); }

/*-----------------------------------------------------------------------*/
/* silk_dstr - dynamic string */
/*-----------------------------------------------------------------------*/

SILK_INTERNAL const char* silk_empty_string(void) { return "\0EMPTY_STRING"; }
SILK_INTERNAL void silk_dstr_init(silk_dstr* dstr) { silk_darr_init(dstr); dstr->data = (char*)silk_empty_string(); }
SILK_INTERNAL void silk_dstr_destroy(silk_dstr* dstr) { if (dstr->data == silk_empty_string()) { dstr->data = NULL; } silk_darr_destroy(dstr); }
/* does not free anything, just reset the size to 0 */
SILK_INTERNAL void silk_dstr_clear(silk_dstr* dstr) { if (dstr->data != silk_empty_string()) { dstr->size = 0; dstr->data[dstr->size] = '\0'; } }

SILK_INTERNAL void
silk_dstr_reserve(silk_dstr* s, silk_size new_string_capacity)
{
	silk_size new_mem_capacity = new_string_capacity + 1;
	char* new_data = NULL;

	SILK_ASSERT(new_string_capacity > s->capacity && "You should request more capacity, not less."); /* ideally we should ensure this before this call. */
	if (new_string_capacity <= s->capacity) { return; }

	new_data = (char*)SILK_MALLOC(new_mem_capacity * sizeof(char));
	SILK_ASSERT(new_data);
	if (!new_data) { return; }

	if (s->size)
	{
		memcpy(new_data, s->data, (s->size + 1) * sizeof(char)); /* +1 is for null-terminated string char */
		SILK_FREE(s->data);
	}

	s->data = new_data;
	s->capacity = new_string_capacity;
}

SILK_INTERNAL void
silk_dstr__grow_if_needed(silk_dstr* s, silk_size needed)
{
	if (needed > s->capacity) { 
		silk_dstr_reserve(s, silk_darr__get_new_capacity(s, needed));
	}
}

SILK_INTERNAL void
silk_dstr_append_from(silk_dstr* s, silk_size index, const char* data, silk_size size)
{
	silk_dstr__grow_if_needed(s, index + size);

	memcpy(s->data + index, (const void*)data, ((size) * sizeof(char)));
	s->size = index + size;
	s->data[s->size] = '\0';
}

SILK_INTERNAL silk_size
silk_dstr_append_from_fv(silk_dstr* s, silk_size index, const char* fmt, va_list args)
{
	va_list args_copy;
	int add_len = 0;
	va_copy(args_copy, args);

	/* Caluclate necessary len */
	add_len = vsnprintf(NULL, 0, fmt, args_copy);
	SILK_ASSERT(add_len >= 0);

	silk_dstr__grow_if_needed(s, s->size + add_len);

	add_len = vsnprintf(s->data + index, add_len + 1, fmt, args);

	s->size = index + add_len;
	return add_len;
}

SILK_INTERNAL void
silk_dstr_assign(silk_dstr* s, const char* data, silk_size size)
{
	silk_dstr_append_from(s, 0, data, size);
}

SILK_INTERNAL void
silk_dstr_assign_str(silk_dstr* s, const char* str)
{
	silk_dstr_assign(s, str, strlen(str));
}

SILK_INTERNAL void
silk_dstr_assign_f(silk_dstr* s, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	silk_dstr_append_from_fv(s, 0, fmt, args);
	va_end(args);
}

SILK_INTERNAL void
silk_dstr_append_str(silk_dstr* s, const char* str)
{
	silk_dstr_append_from(s, s->size, str, strlen(str));
}

SILK_INTERNAL void
silk_dstr_append_strv(silk_dstr* s, silk_strv sv)
{
	silk_dstr_append_from(s, s->size, sv.data, sv.size);
}

SILK_INTERNAL silk_size
silk_dstr_append_f(silk_dstr* s, const char* fmt, ...)
{
	va_list args;
	silk_size len = 0;

	va_start(args, fmt);
	len = silk_dstr_append_from_fv(s, s->size, fmt, args);
	va_end(args);
	return len;
}

/*-----------------------------------------------------------------------*/
/* silk_hash */
/*-----------------------------------------------------------------------*/

SILK_INTERNAL silk_id
djb2_strv(const char* str, silk_size count)
{
	silk_id hash = 5381;
	silk_size i = 0;
	while (i < count)
	{
		hash = ((hash << 5) + hash) + str[i]; /* hash * 33 + c */
		i++;
	}

	return hash;
}

SILK_INTERNAL silk_id
silk_hash_strv(silk_strv sv)
{
	return djb2_strv(sv.data, sv.size);
}

/*-----------------------------------------------------------------------*/
/* silk_kv */
/*-----------------------------------------------------------------------*/

SILK_INTERNAL void
silk_kv_init(silk_kv* kv, silk_strv sv)
{
	memset(kv, 0, sizeof(silk_kv));
	kv->hash = silk_hash_strv(sv);
	kv->key = sv;
}

SILK_INTERNAL silk_kv
silk_kv_make_with_strv(silk_strv sv, silk_strv value)
{
	silk_kv kv;
	silk_kv_init(&kv, sv);
	kv.u.strv = value;
	return kv;
}

SILK_INTERNAL silk_kv
silk_kv_make_with_str(silk_strv sv, const char* value)
{
	return silk_kv_make_with_strv(sv, silk_strv_make_str(value));
}

SILK_INTERNAL silk_kv
silk_kv_make_with_ptr(silk_strv sv, const void* ptr)
{
	silk_kv kv;
	silk_kv_init(&kv, sv);
	kv.u.ptr = ptr;
	return kv;
}

SILK_INTERNAL int
silk_kv_comp(const silk_kv* left, const silk_kv* right)
{
	return left->hash != right->hash
		? (right->hash < left->hash ? -1 : 1)
		: silk_strv_compare_strv(left->key, right->key);
}

SILK_INTERNAL silk_bool
silk_kv_less(const silk_kv* left, const silk_kv* right)
{
	return silk_kv_comp(left, right) < 0;
}

/*-----------------------------------------------------------------------*/
/* silk_mmap - a multimap */
/*-----------------------------------------------------------------------*/

SILK_INTERNAL void silk_mmap_init(silk_mmap* m) { silk_darrT_init(m); }
SILK_INTERNAL void silk_mmap_destroy(silk_mmap* m) { silk_darrT_destroy(m); }

SILK_INTERNAL silk_size
silk_mmap_lower_bound_predicate(const silk_mmap* m, const silk_kv* value)
{
	return silk_lower_bound_predicate(m->base.data, 0, m->base.size, value, sizeof(silk_kv), (silk_predicate_t)silk_kv_less);
}

SILK_INTERNAL void
silk_mmap_insert(silk_mmap* m, silk_kv kv)
{
	silk_size index = silk_mmap_lower_bound_predicate(m, &kv);

	silk_darrT_insert(m, index, kv);
}

SILK_INTERNAL silk_size
silk_mmap_find(const silk_mmap* m, const silk_kv* kv)
{
	silk_size index = silk_mmap_lower_bound_predicate(m, kv);

	if (index == m->base.size || silk_kv_less(kv, silk_darrT_ptr(m, index)))
	{
		index = m->base.size; /* not found */
	}

	return index;
}

SILK_INTERNAL silk_bool
silk_mmap_try_get_first(const silk_mmap* m, silk_strv key, silk_kv* kv)
{
	silk_kv key_item = silk_kv_make_with_str(key, "");
	silk_size index = silk_mmap_find(m, &key_item);

	if (index != m->darr.size) /* found */
	{
		*kv = silk_darrT_at(m, index);
		return silk_true;
	}

	return silk_false;
}

SILK_INTERNAL silk_kv_range
silk_mmap_get_range_all(const silk_mmap* m)
{
	silk_kv_range range;
	range.begin = m->darr.data;
	range.end = m->darr.data + m->darr.size;
	range.count = m->darr.size;
	return range;
}

SILK_INTERNAL silk_kv_range
silk_mmap_get_range(const silk_mmap* m, silk_strv key)
{
	silk_size starting_index;
	silk_kv_range result = { 0, 0, 0 };

	silk_kv key_item = silk_kv_make_with_str(key, "");

	silk_size index = silk_mmap_find(m, &key_item);

	/* No item found */
	if (index == m->darr.size)
	{
		return result;
	}
	
	starting_index = index;

	/* An item has been found */

	result.begin = silk_darrT_ptr(m, index);

	/* Check for other items */
	do
	{
		index += 1;
	} while (index < m->base.size
		&& silk_kv_comp(silk_darrT_ptr(m, index), &key_item) == 0);

	result.end = silk_darrT_ptr(m, index);
	result.count = index - starting_index;
	return result;
}

SILK_INTERNAL silk_kv_range
silk_mmap_get_range_str(const silk_mmap* m, const char* key)
{
	return silk_mmap_get_range(m, silk_strv_make_str(key));
}

SILK_INTERNAL silk_bool
silk_mmap_range_get_next(silk_kv_range* range, silk_kv* next)
{
	memset(next, 0, sizeof(silk_kv));

	SILK_ASSERT(range->begin <= range->end);

	if (range->begin < range->end)
	{
		*next = *range->begin;

		range->begin += 1;
		return silk_true;
	}

	return silk_false;
}

/* Remove all values found in keys, if the value was a dynamic string the dynamic string is destroyed */
SILK_INTERNAL silk_size
silk_mmap_remove(silk_mmap* m, silk_kv kv)
{
	silk_kv_range range = silk_mmap_get_range(m, kv.key);

	silk_size count_to_remove = range.count;

	while (range.begin < range.end)
	{
		range.end -= 1;
		/* Remove item from the tail. */
		silk_darrT_remove(m, range.end - m->darr.data);
	}
	
	return count_to_remove;
}

SILK_INTERNAL silk_bool
silk_mmap_get_from_kv(silk_mmap* map, const silk_kv* item, silk_kv* result)
{
	silk_size index = silk_mmap_find(map, item);
	if (index != map->base.size)
	{
		*result = silk_darrT_at(map, index);
		return silk_true;
	}

	return silk_false;
}

SILK_INTERNAL void
silk_mmap_insert_ptr(silk_mmap* map, silk_strv key, const void* value_ptr)
{
	silk_mmap_insert(map, silk_kv_make_with_ptr(key, value_ptr));
}

SILK_INTERNAL const void*
silk_mmap_get_ptr(silk_mmap* map, silk_strv key, const void* default_value)
{
	silk_kv key_item;
	silk_kv result;

	silk_kv_init(&key_item, key);

	return silk_mmap_get_from_kv(map, &key_item, &result) ? result.u.ptr : default_value;
}

SILK_INTERNAL silk_strv
silk_mmap_get_strv(silk_mmap* map, silk_strv key, silk_strv default_value)
{
	silk_kv key_item;
	silk_kv result;

	silk_kv_init(&key_item, key);

	return silk_mmap_get_from_kv(map, &key_item, &result) ? result.u.strv : default_value;
}

SILK_INTERNAL void
silk_context_init(silk_context* ctx)
{
	memset(ctx, 0, sizeof(silk_context));
	silk_mmap_init(&ctx->projects);
	ctx->current_project = NULL;
}

SILK_INTERNAL void
silk_context_destroy(silk_context* ctx)
{
	silk_mmap_destroy(&ctx->projects);
	silk_context_init(ctx);
}

SILK_INTERNAL void silk_project_destroy(silk_project_t* project);

SILK_INTERNAL void
silk_context_clear(silk_context* ctx)
{
	silk_size i = 0;
	silk_size size = silk_darrT_size(&ctx->projects);
	silk_kv kv = { 0 };
	silk_project_t* p = NULL;

	for (; i < size; ++i)
	{
		kv = silk_darrT_at(&ctx->projects, i);
		p = (silk_project_t*)kv.u.ptr;
		silk_project_destroy(p);
	}
	
	ctx->projects.darr.size = 0;

	ctx->current_project = NULL;
}

SILK_INTERNAL silk_context*
silk_current_context(void)
{
	SILK_ASSERT(current_ctx);
	return current_ctx;
}

SILK_INTERNAL silk_project_t*
silk_current_project(void)
{
	silk_context* ctx = silk_current_context();
	SILK_ASSERT(ctx->current_project);
	if (!ctx) { return NULL; }
	return ctx->current_project;
}

SILK_INTERNAL silk_bool
silk_try_find_project_by_name(silk_strv sv, silk_project_t** project)
{
	void* default_value = NULL;
	*project = (silk_project_t*)silk_mmap_get_ptr(&silk_current_context()->projects, sv, default_value);
	return (silk_bool)(*project != NULL);
}

SILK_INTERNAL silk_project_t*
silk_find_project_by_name(silk_strv sv)
{
	silk_project_t* project = 0;
	if (!silk_try_find_project_by_name(sv, &project))
	{
		silk_log_error("Project not found '%.*s'", sv.size, sv.data);
		return NULL;
	}

	return project;
}

SILK_INTERNAL silk_project_t* silk_find_project_by_name_str(const char* name) { return silk_find_project_by_name(silk_strv_make_str(name)); }
SILK_INTERNAL silk_bool silk_try_find_project_by_name_str(const char* name, silk_project_t** project) { return silk_try_find_project_by_name(silk_strv_make_str(name), project); }

SILK_INTERNAL void
silk_project_init(silk_project_t* project, silk_strv name)
{
	memset(project, 0, sizeof(silk_project_t));

	silk_mmap_init(&project->mmap);
	project->name = name;
}

SILK_INTERNAL void
silk_project_destroy(silk_project_t* project)
{
	SILK_ASSERT(project);
	silk_mmap_destroy(&project->mmap);
}

SILK_INTERNAL silk_project_t*
silk_create_project(const char* name)
{
	silk_strv name_sv = silk_strv_make_str(name);

	silk_project_t* project = (silk_project_t*)SILK_MALLOC(sizeof(silk_project_t));
	SILK_ASSERT(project);
	if (!project) { return NULL; }

	silk_project_init(project, silk_tmp_str_to_strv(name));

	silk_mmap_insert_ptr(&silk_current_context()->projects, name_sv, project);
	
    return project;
}

/* API */

#ifdef _WIN32
/* Any error would silently crash any application, this handler is just there to display a message and exit the application with a specific value */
__declspec(noinline) static LONG WINAPI exit_on_exception_handler(EXCEPTION_POINTERS* ex_ptr)
{
	(void)ex_ptr;
	int exit_code=1;
	printf("[SILK] Error: unexpected error. exited with code %d\n", exit_code);
	exit(exit_code);
}

/* Convert utf-8 string to utf-16 string. String is allocated using the temp buffer */
SILK_INTERNAL wchar_t*
silk_utf8_to_utf16(const char* str)
{
	/* Calculate len */
	int length = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	/* Allocate enough space */
	wchar_t* data = (wchar_t*)silk_tmp_alloc(length * sizeof(wchar_t));
	/* Write data */
	MultiByteToWideChar(CP_UTF8, 0, str, -1, data, length);
	/* Place null-terminated char to end the string */
	data[length] = L'\0';
	return data;
}

#endif

/* #file utils */

SILK_INTERNAL silk_bool silk_is_directory_separator(char c) { return (c == '/' || c == '\\'); }

#define SILK_NPOS ((silk_size)-1)

SILK_INTERNAL silk_size
silk_rfind(silk_strv s, char c)
{
	const char* begin = NULL;
	const char* end = NULL;
	if (s.size == 0) return SILK_NPOS;

	begin = s.data;
	end = s.data + s.size - 1;
	while (end >= begin && *end != c)
	{
		end--;
	}
	return end < begin ? SILK_NPOS : (silk_size)(end - begin);
}

SILK_INTERNAL silk_size
silk_rfind2(silk_strv s, char c1, char c2)
{
	const char* begin = NULL;
	const char* end = NULL;
	if (s.size == 0) return SILK_NPOS;

	begin = s.data;
	end = s.data + s.size - 1;
	while (end >= begin && *end != c1 && *end != c2)
	{
		end--;
	}
	return end < begin ? SILK_NPOS : (silk_size)(end - begin);
}

SILK_INTERNAL silk_strv
silk_path_filename(silk_strv path)
{
	silk_size pos = silk_rfind2(path, '/', '\\');
	if (pos != SILK_NPOS && pos > 0) {
		return silk_strv_make(path.data + pos + 1 /* plus one because we don't want the slash char */
			, path.size - pos);
	}
	return path;
}
SILK_INTERNAL silk_strv
silk_path_filename_str(const char* str)
{
	return silk_path_filename(silk_strv_make_str(str));
}

SILK_INTERNAL silk_strv
silk_path_basename(silk_strv s)
{
	silk_strv filename = silk_path_filename(s);

	if (!silk_strv_equals_str(filename, ".")
		&& !silk_strv_equals_str(filename, "..")) {
		silk_size pos = silk_rfind(filename, '.');
		if (pos != SILK_NPOS && pos > 0) {
			return silk_strv_make(filename.data, pos);
		}
	}
	return filename;
}

SILK_INTERNAL silk_strv
silk_path_basename_str(const char* str)
{
	return silk_path_basename(silk_strv_make_str(str));
}
/*  #file_iterator */

#define SILK_MAX_PATH 1024 /* this is an arbitrary limit */

#define silk_str_append_from(dst, src, index, max) silk_str_append_from_core(dst, src, index, max, __FILE__, __LINE__)

/* Returns the number of character writter. Null-terminating char is not counted. */
SILK_INTERNAL silk_size
silk_str_append_from_core(char* dst, const char* src, silk_size index, silk_size max, const char* file, int line_number)
{
	const char* begin = src;
	silk_size i = index;

	do
	{
		if (i >= (max - 1))
		{
			silk_log_error("Could not copy. String \"%s\" is too long. (file: '%s' line: '%d' max buffer length of '%d').", begin, line_number, file, max);
			SILK_ASSERT(0);
			break;
		}
		dst[i++] = *src++;
	} while (*src != '\0');

	dst[i] = '\0';

	return i - index;
}

/* return 1 if separator has been added 0 otherwise */
SILK_INTERNAL silk_size
silk_ensure_trailing_dir_separator(char* path, silk_size path_len)
{
	if (!path || path_len >= SILK_MAX_PATH) return silk_false;

	if (path_len)
	{
		path += path_len;
		if (path[-1] != '/' && path[-1] != '\\')
		{
			*path++ = SILK_PREFERRED_DIR_SEPARATOR_CHAR;
			*path = '\0';
			return 1;
		}
	}
	return 0;
}

SILK_INTERNAL char*
silk_path_combine(const char* left, const char* right)
{
	char* result = silk_tmp_calloc(SILK_MAX_PATH);
	silk_size n = 0;
	n += silk_str_append_from(result, left, n, SILK_MAX_PATH);
	n += silk_ensure_trailing_dir_separator(result, strlen(result));
	n += silk_str_append_from(result, right, n, SILK_MAX_PATH);
	return result;
}

#ifdef WIN32

SILK_INTERNAL silk_bool
silk_path__exists(const wchar_t* path)
{
	DWORD attr = GetFileAttributesW(path);
	return attr != INVALID_FILE_ATTRIBUTES;
}

SILK_INTERNAL silk_bool
silk_path_exists(const char* path)
{
	return silk_path__exists(silk_utf8_to_utf16(path));
}

SILK_INTERNAL silk_bool
silk__create_directory(const wchar_t* path)
{
	return CreateDirectoryW(path, NULL);
}

SILK_INTERNAL silk_bool
silk_create_directory(const char* path)
{
	return silk__create_directory(silk_utf8_to_utf16(path));
}
#else

SILK_INTERNAL silk_bool silk_path_exists(const char* path) { return access(path, F_OK) == 0; }
SILK_INTERNAL silk_bool silk_path__exists(const char* path) { return silk_path_exists(path); }

SILK_INTERNAL silk_bool silk_create_directory(const char* path) { return mkdir(path, 0777) == 0; }
SILK_INTERNAL silk_bool silk__create_directory(const char* path) { return silk_create_directory(path); }

#endif

SILK_INTERNAL silk_bool
silk_path_is_absolute(silk_strv path)
{
	silk_size len = path.size;

	if (len == 0) return silk_false;

#ifdef _WIN32
	/* Check drive C : */
	silk_size i = 0;
	if (isalpha(path.data[0]) && path.data[1] == ':')
	{
		i = 2;
	}

	return (path.data[i] == '/' || path.data[i] == '\\');
#else

	return (path.data[0] == '/');
#endif
}

#ifdef _WIN32
#define getcwd(buf, size) GetCurrentDirectoryA((DWORD)size, buf)
#else
#include <unistd.h> /* getcwd */
#endif

SILK_INTERNAL char*
silk_path_get_absolute_core(const char* path, silk_bool is_directory)
{
	silk_size n = 0;
	char* buffer = (char*)silk_tmp_calloc(FILENAME_MAX);
	char* cursor = NULL;

	if (!silk_path_is_absolute(silk_strv_make_str(path)))
	{
		/* skip ./ or .\ */
		if (path[0] == '.' && (path[1] == '\\' || path[1] == '/'))
			path += 2;

		getcwd(buffer, FILENAME_MAX);
		if (buffer == NULL)
		{
			silk_log_error("Could not get absolute path from '%s'", path);
			return NULL;
		}
		
		n = strlen(buffer);
		n += silk_ensure_trailing_dir_separator(buffer, n);
	}

	n += silk_str_append_from(buffer, path, n, SILK_MAX_PATH);

	/* Add trailing slash if it's a directory path */
	if (is_directory)
	{
		silk_ensure_trailing_dir_separator(buffer, strlen(buffer));
	}

	cursor = buffer + 2; /* start at 2 to dealing with volume name. */
	/* Replace slash with the preferred one. */
	while (*cursor != '\0')
	{
		if (*cursor == SILK_DEFAULT_DIR_SEPARATOR_CHAR)
		{
			*cursor = SILK_PREFERRED_DIR_SEPARATOR_CHAR;
		}
		cursor += 1;
	}

	return buffer;
}

SILK_INTERNAL char*
silk_path_get_absolute_file(const char* path)
{
	silk_bool is_directory = silk_false;
	return silk_path_get_absolute_core(path, is_directory);
}

SILK_INTERNAL char*
silk_path_get_absolute_dir(const char* path)
{
	silk_bool is_directory = silk_true;
	return silk_path_get_absolute_core(path, is_directory);
}

/* create directories recursively */
SILK_INTERNAL void
silk_create_directories_core(const char* path, silk_size size)
{
#ifdef _WIN32
	typedef wchar_t tchar;
	wchar_t* str = NULL;
#else
	typedef char tchar;
	char* str = NULL;
#endif

	if (path == NULL || size <= 0) {
		silk_log_error("Could not create directory. Path is empty.");
		return;
	}

	if (size > SILK_MAX_PATH) {
		silk_log_error("Could not create directory. Path is too long '%s'.", path);
		return;
	}

#ifdef _WIN32
	str = (wchar_t*)silk_utf8_to_utf16(path);
#else
	str = (char*)path;
#endif

	if (!silk_path__exists(str))
	{
		tchar* cur = str + 2; /* + 2 to avoid root on Windows (unix would require +1 for the original slash ) */
		tchar* end = str + size;
		while (cur < end)
		{
			/* go to next directory separator */
			while (*cur && *cur != '\\' && *cur != '/')
				cur++;

			if (*cur)
			{
				*cur = '\0'; /* terminate path at separator */
				if (!silk_path__exists(str))
				{
					if (!silk__create_directory(str))
					{
						silk_log_error("Could not create directory.");
						return;
					}
				}
				*cur = SILK_PREFERRED_DIR_SEPARATOR_CHAR; /* put the separator back */
			}
			cur++;
		}
	}
}

SILK_INTERNAL void
silk_create_directories(const char* path, silk_size size)
{
	silk_size index = silk_tmp_save();
	silk_create_directories_core(path, size);
	silk_tmp_restore(index);
}

SILK_INTERNAL silk_bool
silk_copy_file(const char* src_path, const char* dest_path)
{
#ifdef _WIN32
	/* create target directory if it does not exists */
	silk_create_directories(dest_path, strlen(dest_path));
	silk_log_debug("Copying '%s' to '%s'", src_path, dest_path);

	wchar_t* src_path_w = silk_utf8_to_utf16(src_path);
	wchar_t* dest_path_w = silk_utf8_to_utf16(dest_path);
	DWORD attr = GetFileAttributesW(src_path_w);

	if (attr == INVALID_FILE_ATTRIBUTES) {
		silk_log_error("Could not retieve file attributes of file '%s' (%d).", src_path, GetLastError());
		return silk_false;
	}

	silk_bool is_directory = attr & FILE_ATTRIBUTE_DIRECTORY;
	BOOL fail_if_exists = FALSE;
	if (!is_directory && !CopyFileW(src_path_w, dest_path_w, fail_if_exists)) {
		silk_log_error("Could not copy file '%s', %lu", src_path, GetLastError());
		return silk_false;
	}
	return silk_true;
#else

	int src_fd = -1;
	int dst_fd = -1;
	struct stat src_stat;
	off_t sendfile_off = 0;
	silk_size send_result = 0;
	silk_size total_bytes_copied = 0;
	silk_size bytes_copied = 0;
	silk_size bytes_left = 0;

	/* create target directory if it does not exists */
	silk_create_directories(dest_path, strlen(dest_path));
	silk_log_debug("Copying '%s' to '%s'", src_path, dest_path);

	src_fd = open(src_path, O_RDONLY);
	if (src_fd < 0)
	{
		silk_log_error("Could not open file '%s': %s", src_path, strerror(errno));
		return silk_false;
	}
	
    if (fstat(src_fd, &src_stat) < 0)
	{
        silk_log_error("Could not get fstat of file '%s': %s", src_path, strerror(errno));
		close(src_fd);
		return silk_false;
    }
	
	dst_fd = open(dest_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);

	if (dst_fd < 0)
	{
        silk_log_error("Could not open file '%s': %s", dest_path, strerror(errno));
		close(src_fd);
		return silk_false;
	}
	
    total_bytes_copied = 0;
    bytes_left = src_stat.st_size;
    while (bytes_left > 0)
    {
		sendfile_off = total_bytes_copied;
		send_result = (silk_size)sendfile(dst_fd, src_fd, &sendfile_off, bytes_left);
		if(send_result <= 0)
		{
			break;
		}
		bytes_copied = send_result;
		bytes_left -= bytes_copied;
		total_bytes_copied += bytes_copied;
    }
  
	close(src_fd);
	close(dst_fd);
	return bytes_left == 0;
#endif
}

SILK_INTERNAL silk_bool
silk_try_copy_file_to_dir(const char* file, const char* directory)
{
	silk_bool result = 0;
	silk_size tmp_index = silk_tmp_save();
	silk_strv filename;
	const char* destination_file = 0;

	if (silk_path_exists(file))
	{
		filename = silk_path_filename_str(file);
		destination_file = silk_tmp_sprintf("%s%.*s", directory, filename.size, filename.data);

		result = silk_copy_file(file, destination_file);
	}

	silk_tmp_restore(tmp_index);
	return result;
}

SILK_INTERNAL silk_bool
silk_copy_file_to_dir(const char* file, const char* directory)
{
	if (!silk_try_copy_file_to_dir(file, directory))
	{
		silk_log_error("Could not copy file '%s' to directory %s", file, directory);
		return silk_false;
	}
	return silk_true;
}


SILK_INTERNAL silk_bool
silk_delete_file(const char* src_path)
{
	silk_bool result = 0;
	silk_size tmp_index = silk_tmp_save();
	silk_log_debug("Deleting file '%s'.", src_path);
#ifdef _WIN32
	result = DeleteFileW(silk_utf8_to_utf16(src_path));
#else
	result = remove(src_path) != -1;
#endif
	if (!result) {
		silk_log_debug("Could not delete file '%s'.", src_path);
	}

	silk_tmp_restore(tmp_index);

	return result;
}

SILK_INTERNAL silk_bool
silk_move_file(const char* src_path, const char* dest_path)
{
	if (silk_copy_file(src_path, dest_path))
	{
		return silk_delete_file(src_path);
	}

	return silk_false;
}

SILK_INTERNAL silk_bool
silk_try_move_file_to_dir(const char* file, const char* directory)
{
	silk_bool result = silk_false;
	silk_strv filename = { 0 };
	const char* destination_file = NULL;

	silk_size index = silk_tmp_save();
	if (silk_path_exists(file))
	{
		filename = silk_path_filename_str(file);
		destination_file = silk_tmp_sprintf("%s%.*s", directory, filename.size, filename.data);

		result = silk_move_file(file, destination_file);
	}
	
	silk_tmp_restore(index);

	return result;
}

SILK_INTERNAL silk_bool
silk_move_file_to_dir(const char* file, const char* directory)
{
	if (!silk_try_move_file_to_dir(file, directory))
	{
		silk_log_error("Could not move file '%s' to '%s'", file, directory);
		return silk_false;
	}
	return silk_true;
}

SILK_API void
silk_init(void)
{
	silk_context_init(&default_ctx);
	current_ctx = &default_ctx;
#ifdef _WIN32
	silk_bool exit_on_exception = silk_true; /* @TODO make this configurable */
	if (exit_on_exception)
	{
		SetUnhandledExceptionFilter(exit_on_exception_handler);
	}
#endif
}

SILK_API void
silk_destroy(void)
{
	silk_context_destroy(silk_current_context());
	silk_tmp_reset();
}

SILK_API void
silk_clear(void)
{
	silk_context_clear(silk_current_context());
	silk_tmp_reset();
}

SILK_API silk_project_t*
silk_project(const char* name)
{
	silk_project_t* project;

	if (!silk_try_find_project_by_name_str(name, &project))
	{
		project = silk_create_project(name);
	}

	silk_current_context()->current_project = project;
	return project;
}

SILK_API silk_project_t*
silk_project_f(const char* format, ...)
{
	silk_project_t* p;
	va_list args;
	va_start(args, format);

	p = silk_project(silk_tmp_vsprintf(format, args));

	va_end(args);

	return p;
}

SILK_API void
silk_add_many_core(silk_strv key, silk_strv values[], silk_size count)
{
	silk_size i;
	silk_strv value = { 0 };
	/* Check that the ptr is contains in the tmp buffer */
	SILK_ASSERT(silk_tmp_contains(key.data));

	for (i = 0; i < count; ++i)
	{
		value = values[i];
		/* Check that the ptr is contains in the tmp buffer */
		SILK_ASSERT(silk_tmp_contains(value.data));
		
		silk_mmap_insert(
			&silk_current_project()->mmap,
			silk_kv_make_with_strv(key, value));
	}
}

SILK_INTERNAL void
silk_add_many(const char* key, const char* values[], silk_size count)
{
	silk_size i;
	silk_strv value = { 0 };

	for (i = 0; i < count; ++i)
	{
		value = silk_tmp_str_to_strv(values[i]);
		silk_add_many_core(silk_tmp_str_to_strv(key), &value, 1);
	}
}

SILK_API void
silk_add_many_vnull(const char* key, ...)
{
	silk_strv value;
	va_list args;
	const char* current = NULL;
	va_start(args, key);

	current = va_arg(args, const char*);
	SILK_ASSERT(current);
	while (current)
	{
		value = silk_tmp_str_to_strv(current);
		silk_add_many_core(silk_tmp_str_to_strv(key), &value, 1);
		current = va_arg(args, const char*);
	}
	va_end(args);
}

SILK_API void
silk_add(const char* key, const char* value)
{
	silk_strv value_copy = silk_tmp_str_to_strv(value);
	silk_add_many_core(silk_tmp_str_to_strv(key), &value_copy, 1);
}

SILK_API void
silk_add_f(const char* key, const char* format, ...)
{
	silk_strv value;
	va_list args;
	va_start(args, format);

	value = silk_tmp_strv_vprintf(format, args);
	silk_add_many_core(silk_tmp_str_to_strv(key), &value, 1);

	va_end(args);
}

SILK_API void
silk_set(const char* key, const char* value)
{
	/* @OPT this can easily be optimized, but we don't care about that right now. */
	silk_remove_all(key);
	silk_add(key, value);
}

SILK_API void
silk_set_f(const char* key, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	silk_set(key, silk_tmp_vsprintf(format, args));

	va_end(args);
}

SILK_API silk_size
silk_remove_all(const char* key)
{
	silk_kv kv = silk_kv_make_with_str(silk_strv_make_str(key), "");
	silk_project_t* p = silk_current_project();
	return silk_mmap_remove(&p->mmap, kv);
}

SILK_API silk_size
silk_remove_all_f(const char* format, ...)
{
	va_list args;
	silk_size count = 0;
	va_start(args, format);

	count = silk_remove_all(silk_tmp_vsprintf(format, args));

	va_end(args);
	return count;
}

SILK_API silk_bool
silk_contains(const char* key, const char* value)
{
	silk_project_t* p;
	silk_kv_range range;
	silk_kv kv = silk_kv_make_with_str(silk_strv_make_str(key), value);
	p = silk_current_project();
	range = silk_mmap_get_range(&p->mmap, kv.key);

	while (range.begin < range.end)
	{
		if (silk_strv_equals_strv((*range.begin).u.strv, kv.u.strv))
		{
			return silk_true;
		}
		range.begin++;
	}
	return silk_false;
}

SILK_API silk_bool
silk_remove_one(const char* key, const char* value)
{
	silk_kv kv = silk_kv_make_with_str(silk_strv_make_str(key), value);
	silk_project_t* p = silk_current_project();
	silk_kv_range range = silk_mmap_get_range(&p->mmap, kv.key);

	while (range.begin < range.end)
	{
		if (silk_strv_equals_strv((*range.begin).u.strv, kv.u.strv))
		{
			silk_size index = p->mmap.darr.data - range.begin;
			silk_darrT_remove(&p->mmap, index);
			return silk_true;
		}
		range.begin++;
	}
	return silk_false;
}

SILK_API silk_bool
silk_remove_one_f(const char* key, const char* format, ...)
{
	va_list args;
	silk_bool was_removed = silk_false;
	va_start(args, format);

	was_removed = silk_remove_one(key, silk_tmp_vsprintf(format, args));

	va_end(args);
	return was_removed;
}

SILK_API void
silk_add_file(const char* file)
{
	silk_add(silk_FILES, file);
}

/* Properties are just (strv) values from the map of a project. */
SILK_INTERNAL silk_bool
try_get_property(silk_project_t* project, const char* key, silk_kv* result)
{
	if (silk_mmap_try_get_first(&project->mmap, silk_strv_make_str(key), result))
	{
		return silk_true;
	}
	return silk_false;
}

SILK_INTERNAL silk_bool
try_get_property_strv(silk_project_t* project, const char* key, silk_strv* result)
{
	silk_kv kv_result;
	if (silk_mmap_try_get_first(&project->mmap, silk_strv_make_str(key), &kv_result))
	{
		*result = kv_result.u.strv;
		return silk_true;
	}
	return silk_false;
}

SILK_INTERNAL silk_bool
silk_property_equals(silk_project_t* project, const char* key, const char* comparison_value)
{
	silk_strv result;
	return try_get_property_strv(project, key, &result)
		&& silk_strv_equals_str(result, comparison_value);
}

SILK_API const char*
silk_bake_project_with(silk_toolchain toolchain, const char* project_name)
{
	const char* result = toolchain.bake(&toolchain, project_name);
	silk_log_important("%s", result);
	return result;
}

SILK_API const char*
silk_bake_project(const char* project_name)
{
	return silk_bake_project_with(silk_toolchain_default(), project_name);
}

SILK_API const char*
silk_bake(void)
{
	silk_project_t* p = silk_current_project();
	return silk_bake_project(p->name.data);
}

SILK_INTERNAL const char*
silk_get_output_directory(silk_project_t* project, const silk_toolchain* tc)
{
	silk_strv out_dir;
	if (try_get_property_strv(project, silk_OUTPUT_DIR, &out_dir))
	{
		return silk_path_get_absolute_dir(out_dir.data);
	}
	else
	{
		/* Get default output directory */
		return silk_path_get_absolute_dir(silk_path_combine(tc->default_directory_base, project->name.data));
	}
}

SILK_API void
silk_debug(silk_bool value)
{
	silk_debug_enabled = value;
}

struct silk_process_handle {
	const char* cmd;
	const char* starting_directory;
	silk_bool stdout_to_string; /* If stdout needs to be copied to a string. */
	silk_bool stderr_to_string; /* If stderr needs to be copied to a string. */
	silk_dstr stdout_string;    /* If stdout_to_string has been set to true. */
	silk_dstr stderr_string;    /* If stderr_to_string has been set to true. */
	int exit_code;
};

SILK_INTERNAL silk_process_handle* silk_process_core(silk_process_handle* handle);

SILK_INTERNAL silk_process_handle*
silk_create_process_handle(const char* cmd, const char* starting_directory)
{
	silk_process_handle* handle = (silk_process_handle*)silk_tmp_alloc(sizeof(silk_process_handle));
	memset(handle, 0, sizeof(silk_process_handle));

	handle->cmd = cmd;
	handle->starting_directory = starting_directory;
	handle->exit_code = -1;

	silk_dstr_init(&handle->stdout_string);
	silk_dstr_init(&handle->stderr_string);

	return handle;
}

SILK_API int
silk_process(const char* cmd)
{
	return silk_process_in_directory(cmd, NULL);
}

SILK_API int
silk_process_in_directory(const char* cmd, const char* starting_directory)
{
	silk_process_handle* handle = silk_create_process_handle(cmd, starting_directory);
	handle = silk_process_core(handle);
	return silk_process_end(handle);
}

SILK_API silk_process_handle*
silk_process_to_string(const char* cmd, const char* starting_directory, silk_bool also_get_stderr)
{
	silk_process_handle* handle = silk_create_process_handle(cmd, starting_directory);

	handle->stdout_to_string = silk_true;
	handle->stderr_to_string = also_get_stderr;

	return silk_process_core(handle);
}

SILK_API int
silk_run(const char* executable_path)
{
	return silk_process_in_directory(silk_tmp_sprintf("\"%s\"", executable_path), NULL);
}

SILK_API const char*
silk_process_stdout_string(silk_process_handle* handle)
{
	return handle->stdout_string.data;
}

SILK_API const char*
silk_process_stderr_string(silk_process_handle* handle)
{
	return handle->stderr_string.data;
}

SILK_API int
silk_process_end(silk_process_handle* handle)
{
	silk_dstr_destroy(&handle->stdout_string);
	silk_dstr_destroy(&handle->stderr_string);

	return handle->exit_code;
}

#if _WIN32

/* #process */

SILK_INTERNAL silk_process_handle*
silk_process_core(silk_process_handle* handle)
{
	HANDLE process_stdout_write = NULL; /* Pipe (write) for stdout of the child process */
	HANDLE process_stdout_read = NULL;  /* Pipe (read) for stdout of the child process */
	HANDLE process_stderr_write = NULL; /* Pipe (write) for stderr of the child process */
	HANDLE process_stderr_read = NULL;  /* Pipe (read) for stderr of the child process */
	SECURITY_ATTRIBUTES saAttr;         /* To create the pipes. */

	DWORD exit_code = (DWORD) -1;
	DWORD wait_result = 0;
	DWORD byte_read_from_buffer = 0;
	char process_output_buffer[256] = { 0 };
	BOOL handles_inheritance = 0;

	wchar_t* cmd_w = silk_utf8_to_utf16(handle->cmd);
	wchar_t* starting_directory_w = NULL;

	silk_log_debug("Running process '%s'", handle->cmd);
	if (handle->starting_directory && handle->starting_directory[0])
	{
		starting_directory_w = silk_utf8_to_utf16(handle->starting_directory);
		silk_log_debug("Subprocess started in directory '%s'", handle->starting_directory);
	}

	/* Ensure that everything is written into the outputs before creating a new process that will also write in those outputs */
	fflush(stdout);
	fflush(stderr);

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.silk = sizeof(si);

	if (handle->stdout_to_string
		|| handle->stderr_to_string)
	{
		handles_inheritance = TRUE;

		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

		si.dwFlags |= STARTF_USESTDHANDLES;
			
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		if (handle->stdout_to_string)
		{
			/* Create a pipe for the child process's stdout. */
			if (!CreatePipe(&process_stdout_read, &process_stdout_write, &saAttr, 0)
				|| !SetHandleInformation(process_stdout_read, HANDLE_FLAG_INHERIT, 0))
			{
				silk_log_error("Could not create pipe for stdout.");
				exit(1);
			}

			si.hStdOutput = process_stdout_write;
		}

		if (handle->stderr_to_string)
		{
			/* Create a pipe for the child process's stderr. */
			if (!CreatePipe(&process_stderr_read, &process_stderr_write, &saAttr, 0)
				|| !SetHandleInformation(process_stderr_read, HANDLE_FLAG_INHERIT, 0))
			{
				silk_log_error("Could not create pipe for stderr.");
				exit(1);
			}

			si.hStdError = process_stderr_write;
		}
	}

	ZeroMemory(&pi, sizeof(pi));

	/* Start the child process.
	*  NOTE: we use CreateProcessW because CreateProcessA (with /utf-8) cannot create path with unicode.
	*/
	if (!CreateProcessW(
		NULL,                 /* No module name (use command line) */
		cmd_w,                /* Command line */
		NULL,                 /* Process handle not inheritable */
		NULL,                 /* Thread handle not inheritable */ 
		handles_inheritance,  /* Set handle inheritance */
		0,                    /* No creation flags */
		NULL,                 /* Use parent's environment block */
		starting_directory_w, /* Use parent's starting directory */
		&si,                  /* Pointer to STARTUPINFO structure */
		&pi)                  /* Pointer to PROCESS_INFORMATION structure */
		)
	{
		silk_log_error("CreateProcessW failed: %d", GetLastError());
		/* No need to close handles since the process creation failed */
		return handle;
	}

	/* Close the write ends of the pipes since they will not be used in the parent process. */
	
	if (handle->stdout_to_string)
		CloseHandle(process_stdout_write);
	if (handle->stderr_to_string)
		CloseHandle(process_stderr_write);

	wait_result = WaitForSingleObject(
		pi.hProcess, /* HANDLE hHandle, */
		INFINITE     /* DWORD  dwMilliseconds */
	);

	if (wait_result == WAIT_FAILED)
	{
		silk_log_error("Could not wait on child process: %lu", GetLastError());
	}
	else
	{
		if (GetExitCodeProcess(pi.hProcess, &exit_code))
		{
			if (exit_code != 0)
			{
				silk_log_error("Command exited with exit code %lu", exit_code);
			}
		}
		else
		{
			silk_log_error("Could not get process exit code: %lu", GetLastError());
		}
	}

	if (handle->stdout_to_string)
	{
		/* Read output from the child process's pipe for stdout. Stop when there is no more data. */
		while (ReadFile(process_stdout_read, process_output_buffer, sizeof(process_output_buffer), &byte_read_from_buffer, NULL)
			&& byte_read_from_buffer != 0)
		{
			silk_dstr_append_f(&handle->stdout_string, "%.*s", (int)byte_read_from_buffer, process_output_buffer);
		}
		CloseHandle(process_stdout_read);
	}

	if (handle->stderr_to_string)
	{
		while (ReadFile(process_stderr_read, process_output_buffer, sizeof(process_output_buffer), &byte_read_from_buffer, NULL)
			&& byte_read_from_buffer != 0)
		{
			silk_dstr_append_f(&handle->stderr_string, "%.*s", (int)byte_read_from_buffer, process_output_buffer);
		}

		CloseHandle(process_stderr_read);
	}

	/* Close process and thread handles. */
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	handle->exit_code = exit_code;
	return handle;
}
#else

/* space or tab */
SILK_INTERNAL silk_bool silk_is_space(char c) { return c == ' ' || c == '\t'; }

SILK_INTERNAL silk_bool
silk_is_end_of_quote(const char* str, char quote_type)
{
	return *str != '\0'
		&& *str == quote_type
		&& (str[1] == '\0' || silk_is_space(str[1]));

}
/* Parse arguments, space is a separator and everything between <space or begining of string><quote> and <quote><space or end of string>
* Quotes can be double quotes or simple quotes.
a b c   => 3 arguments a b c
abc     => 1 argument "abc"
a "b" c => 3 arguments a b c;
a"b"c   => 1 argument a"b"c
"a"b"c" => 1 argument a"b"c
*/
SILK_INTERNAL const char*
silk_get_next_arg(const char* str, silk_strv* sv)
{
	sv->data = str;
	sv->size = 0;
	if (str == NULL || *str == '\0')
		return NULL;
		
	while (*str != '\0')
	{
		/* Skip spaces */
		while (*str != '\0' && silk_is_space(*str))
			str += 1;

		/* Return early if end of string */
		if (*str == '\0') 
			return sv->size > 0 ? str: NULL;

		/* Handle quotes */
		if (*str == '\'' || *str == '\"')
		{
			const char* quote = str;
			str += 1; /* skip quote */
			
			/* Return early if end of string */
			if (*str == '\0')
				return sv->size > 0 ? str : NULL;

			/* Quote next the previous one so it's an empty content, we look for another item */
			if (silk_is_end_of_quote(str, *quote))
			{
				str += 1; /* Skip quote */
				continue;
			}

			sv->data = str; /* The next argument will begin right after the quote */
			/* Skip everything until the next unescaped quote */
			while (*str != '\0' && !silk_is_end_of_quote(str, *quote))
				str += 1;

			/* Either it's the end of the quoted string, either we reached the null terminating string */
			sv->size = (str - quote) - 1;

			/* Eat trailing quote so that next argument can start with space */
			while (*str != '\0' && *str == *quote)
				str += 1;

			return str;
		}
		else /* is char */
		{
			const char* ch = str;
			while (*str != '\0' && !silk_is_space(*str))
				str += 1;

			sv->data = ch; /* remove quote */
			sv->size = str - ch;
			return str;
		}
	}
	return NULL;
}

#define SILK_INVALID_PROCESS (-1)

SILK_INTERNAL pid_t
silk_fork_process(char* args[], silk_process_handle* handle, int stdout_pfd[2], int stderr_pfd[2])
{
	pid_t pid = fork();

	switch (pid)
	{
	case -1: /* Error */
		silk_log_error("Could not fork child process: %s", strerror(errno));
		return SILK_INVALID_PROCESS;
	case 0:  /* Child process */
	{
		if (handle->stdout_to_string)
		{
			dup2(stdout_pfd[1], STDOUT_FILENO);
		}
		if (handle->stderr_to_string)
		{
			dup2(stderr_pfd[1], STDERR_FILENO);
		}

		/* Change directory in the fork */
		if (handle->starting_directory
			&& handle->starting_directory[0]
			&& chdir(handle->starting_directory) < 0) {
			silk_log_error("Could not change directory to '%s': %s", handle->starting_directory, strerror(errno));
			return SILK_INVALID_PROCESS;
		}
		if (execvp(args[0], args) == SILK_INVALID_PROCESS) {
			silk_log_error("Could not exec child process: %s", strerror(errno));
			return SILK_INVALID_PROCESS;
		}
		SILK_ASSERT(0 && "unreachable");
		break;
	}
	default:
		/* Do nothing for parent process */
		break;
	}

	return pid;
}

SILK_INTERNAL silk_process_handle*
silk_process_core(silk_process_handle* handle)
{
	silk_darrT(const char*) args;
	silk_strv arg; /* Current argument */
	const char* cmd_cursor = handle->cmd; /* Current position in the string command */
	pid_t pid = SILK_INVALID_PROCESS;
	int wstatus = 0; /* pid wait status */
	int exit_status = -1;

	ssize_t bytes_read = 0;   /* Byte read count when we retrieve the output of the child process. */
	char buffer[256] = { 0 }; /* Buffer to get the output of the child process. */
	int stdout_pfd[2] = { 0 };     /* Pipe file descriptor for stdout. */
	int stderr_pfd[2] = { 0 };     /* Pipe file descriptor for stderr. */

	silk_darrT_init(&args);

	silk_log_debug("Running process '%s'", handle->cmd);
	if (handle->starting_directory && handle->starting_directory[0])
	{
		silk_log_debug("Subprocess started in directory '%s'", handle->starting_directory);
	}

	/* Ensure that everything is written into the outputs before creating a new process that will also write in those outputs */
	fflush(stdout);
	fflush(stderr);

	/* Split args from the command line and add it to the array. */
	while ((cmd_cursor = silk_get_next_arg(cmd_cursor, &arg)) != NULL)
	{
		silk_darrT_push_back(&args, silk_tmp_strv_to_str(arg));
	}

	/* Last value of the args should be a null value, only append if necessary */
	if (args.darr.size > 0)
	{
		silk_darrT_push_back(&args, NULL);
	}

	if (handle->stdout_to_string)
	{
		if (pipe(stdout_pfd) == -1)
		{
			silk_log_error("Pipe creation failed for stdout_pfd.");
			silk_set_and_goto(exit_status, -1, cleanup);
		}
	}
	if (handle->stderr_to_string)
	{
		if (pipe(stderr_pfd) == -1)
		{
			silk_log_error("Pipe creation failed for stderr_pfd.");
			silk_set_and_goto(exit_status, -1, cleanup);
		}
	}

	pid = silk_fork_process((char**)args.darr.data, handle, stdout_pfd, stderr_pfd);

	if (pid == SILK_INVALID_PROCESS)
	{
		silk_set_and_goto(exit_status, -1, cleanup);
	}

	/* wait for process to be done */
	for (;;) {
		if (waitpid(pid, &wstatus, 0) < 0) {
			silk_log_error("Could not wait on command (pid %d): '%s'", pid, strerror(errno));
			silk_set_and_goto(exit_status, -1, cleanup);
		}

		if (WIFEXITED(wstatus)) {
			exit_status = WEXITSTATUS(wstatus);
			if (exit_status != 0) {
				silk_log_error("Command exited with exit code '%d'", exit_status);
				silk_set_and_goto(exit_status, -1, cleanup);
			}

			break; /* Get out of the loop since the process exited. */
		}

		if (WIFSIGNALED(wstatus)) {
			silk_log_error("Command process was terminated by '%s'", strsignal(WTERMSIG(wstatus)));
			silk_set_and_goto(exit_status, -1, cleanup);
		}
	}

	if (handle->stdout_to_string)
	{
		/* Close the write pipe since the child program has been exited. */
		close(stdout_pfd[1]);

		/* Read output of the child process from the read file description of the pipe. */
		while ((bytes_read = read(stdout_pfd[0], buffer, sizeof(buffer))) > 0)
		{
			silk_dstr_append_f(&handle->stdout_string, "%.*s", (int)bytes_read, buffer);
		}
		/* Close the read pipe since we read all the information from it. */
		close(stdout_pfd[0]);
	}

	if (handle->stderr_to_string)
	{
		/* Close the write pipe since the child program has been exited. */
		close(stderr_pfd[1]);

		if (handle->stderr_to_string)
		{
			/* Read output of the child process from the read file description of the pipe. */
			while ((bytes_read = read(stderr_pfd[0], buffer, sizeof(buffer))) > 0)
			{
				silk_dstr_append_f(&handle->stderr_string, "%.*s", (int)bytes_read, buffer);
			}
			/* Close the read pipe since we read all the information from it. */
			close(stderr_pfd[0]);
		}
	}

cleanup:
	silk_darrT_destroy(&args);
	handle->exit_code = exit_status;
	return handle;
}

#endif

#ifdef _WIN32

/* ================================================================ */
/* toolchain MSVC */
/* ================================================================ */

/* #msvc #toolchain */

SILK_API const char*
silk_toolchain_msvc_bake(silk_toolchain* tc, const char* project_name)
{
	silk_dstr str; /* cl.exe command */
	/* @FIXME use an array of string instead to make it straigthforward to follow */
	silk_dstr str_obj; /* to keep track of the .obj generated and copy them.*/
	const char* output_dir;       /* Output directory. Contains the directory path of the binary being created. */
	
	silk_kv_range range = { 0 };
	silk_kv_range lflag_range = { 0 };
	silk_kv current = { 0 };       /* Temporary kv to store results. */
	silk_kv current_lflag = { 0 }; /* Temporary kv to linker flags. */
	silk_strv basename = { 0 };
	const char* artefact = NULL; /* Resulting artefact path */
	const char* ext = "";        /* Resulting artefact extension */
	const char* _ = "  ";        /* Space to separate command arguments. */
	const char* tmp = "";
	
	silk_strv linked_project_name = { 0 };
	silk_project_t* linked_project = NULL;
	const char* linked_output_dir; /* to keep track of the .obj generated */
	const char* path_prefix = NULL;
	silk_project_t* project = NULL;
	silk_size tmp_index;
	project = silk_find_project_by_name_str(project_name);

	if (!project)
	{
		return NULL;
	}
	
	silk_dstr_init(&str);
	silk_dstr_init(&str_obj);

	/* Get and format output directory */
	output_dir = silk_get_output_directory(project, tc);

	/* Create output directory if it does not exist yet. */
	silk_create_directories(output_dir, strlen(output_dir));

	/* Use /utf-8 by default since it's retrocompatible with utf-8 */
	/* Use /nologo to avoid undesirable messages in the command line. */
	silk_dstr_append_str(&str, "cl.exe /utf-8 /nologo ");

	/* Handle binary type */

	silk_bool is_exe = silk_property_equals(project, silk_BINARY_TYPE, silk_EXE);
	silk_bool is_shared_library = silk_property_equals(project, silk_BINARY_TYPE, silk_SHARED_LIBRARY);
	silk_bool is_static_library = silk_property_equals(project, silk_BINARY_TYPE, silk_STATIC_LIBRARY);

	if (!is_exe && !is_shared_library && !is_static_library)
	{
		silk_log_error("Unknown binary type");
		silk_set_and_goto(artefact, NULL, exit);
	}

	if (is_static_library) {
		silk_dstr_append_str(&str, "/c ");
	}
	else if (is_shared_library) {
		silk_dstr_append_str(&str, "/LD ");
	}

	/* Add output directory to cl.exe command */
	/* /Fo"output/directory/" */
	silk_dstr_append_f(&str, "/Fo\"%s\" ", output_dir);

	ext = is_exe ? ".exe" : ext;
	ext = is_static_library ? ".lib" : ext;
	ext = is_shared_library ? ".dll" : ext;

	artefact = silk_tmp_sprintf("%s%s%s", output_dir, project_name, ext);

	/* Define binary output for executable and shared library. Static library is set with the link.exe command*/
	if (is_exe || is_shared_library)
	{
		/* /Fe"output/directory/bin.ext" */
		silk_dstr_append_f(&str, "/Fe\"%s\" ", artefact);
	}

	/* Append compiler flags */
	{
		range = silk_mmap_get_range_str(&project->mmap, silk_CXFLAGS);
		while (silk_mmap_range_get_next(&range, &current))
		{
			silk_dstr_append_f(&str,"%s ", current.u.strv.data);
		}
	}

	/* Append include directories */
	{
		range = silk_mmap_get_range_str(&project->mmap, silk_INCLUDE_DIRECTORIES);
		while (silk_mmap_range_get_next(&range, &current))
		{
			/* Absolute file is created using the tmp buffer allocator but we don't need it once it's inserted into the dynamic string */
			tmp_index = silk_tmp_save();
			silk_dstr_append_f(&str, "/I\"%s\" ", silk_path_get_absolute_dir(current.u.strv.data));
			silk_tmp_restore(tmp_index);
		}
	}
	
	/* Append preprocessor definition */
	{
		range = silk_mmap_get_range_str(&project->mmap, silk_DEFINES);
		while (silk_mmap_range_get_next(&range, &current))
		{
			silk_dstr_append_f(&str, "/D\"%s\" ", current.u.strv.data);
		}
	}

	/* Append files and .obj */
	{
		range = silk_mmap_get_range_str(&project->mmap, silk_FILES);
		while (silk_mmap_range_get_next(&range, &current))
		{
			/* Absolute file is created using the tmp buffer allocator but we don't need it once it's inserted into the dynamic string */
			tmp_index = silk_tmp_save();

			silk_dstr_append_f(&str, "\"%s\" ", silk_path_get_absolute_file(current.u.strv.data));

			silk_tmp_restore(tmp_index);

			basename = silk_path_basename(current.u.strv);

			silk_dstr_append_f(&str_obj, "\"%.*s.obj\" ", basename.size, basename.data);
		}
	}

	/* Append libraries */
	{
		range = silk_mmap_get_range_str(&project->mmap, silk_LIBRARIES);
		while (silk_mmap_range_get_next(&range, &current))
		{
			silk_dstr_append_f(&str, "\"%s.lib\" ", current.u.strv.data);
		}
	}

	/* For each linked project we add the link information to the cl.exe command */
	range = silk_mmap_get_range_str(&project->mmap, silk_LINK_PROJECTS);
	if (range.count > 0)
	{
		silk_dstr_append_str(&str, "/link ");
		/* Add linker flags */
		{
			lflag_range = silk_mmap_get_range_str(&project->mmap, silk_LFLAGS);
			while (silk_mmap_range_get_next(&lflag_range, &current_lflag))
			{
				silk_dstr_append_strv(&str, current_lflag.u.strv);
				silk_dstr_append_str(&str, _);
			}
		}

		/* iterate all the linked projects */
		while (silk_mmap_range_get_next(&range, &current))
		{
			linked_project_name = current.u.strv;

			linked_project = silk_find_project_by_name(linked_project_name);
			if (!project)
			{
				silk_set_and_goto(artefact, NULL, exit);
			}

			linked_output_dir = silk_get_output_directory(linked_project, tc);

			/* is shared or static library */
			if (silk_property_equals(linked_project, silk_BINARY_TYPE, silk_SHARED_LIBRARY)
				|| silk_property_equals(linked_project, silk_BINARY_TYPE, silk_STATIC_LIBRARY))
			{
				/* /LIBPATH:"output/dir/" "mlib.lib" */
				silk_dstr_append_f(&str, "/LIBPATH:\"%s\" \"%.*s.lib\" ", linked_output_dir, linked_project_name.size, linked_project_name.data);
			}

			/* is shared library */
			if (silk_property_equals(linked_project, silk_BINARY_TYPE, silk_SHARED_LIBRARY))
			{
				path_prefix = silk_tmp_sprintf("%s%.*s", linked_output_dir, linked_project_name.size, linked_project_name.data);
				/* .dll */
				tmp = silk_tmp_sprintf("%s.dll", path_prefix);
				if (!silk_copy_file_to_dir(tmp, output_dir))
				{
					silk_set_and_goto(artefact, NULL, exit);
				}

				/* .exp */
				tmp = silk_tmp_sprintf("%s.exp", path_prefix);
				if (!silk_copy_file_to_dir(tmp, output_dir))
				{
					silk_log_warning("Missing .exp. Shared libraries usually need to have some symbol exported with '__declspec(dllexport)'");
					silk_set_and_goto(artefact, NULL, exit);
				}

				/* .lib */
				tmp = silk_tmp_sprintf("%s.lib", path_prefix);
				if (!silk_copy_file_to_dir(tmp, output_dir))
				{
					silk_log_warning("Missing .lib file. Shared libraries must create a .lib file for other program to be linked with at compile time.");
					silk_set_and_goto(artefact, NULL, exit);
				}

				/* Copy .pdb if there is any. */
				tmp = silk_tmp_sprintf("%s.pdb", path_prefix);
				silk_try_copy_file_to_dir(tmp, output_dir);
			}
		}
	}

	/* execute cl.exe */
	if (silk_process_in_directory(str.data, output_dir) != 0)
	{
		silk_set_and_goto(artefact, NULL, exit);
	}
	
	if (is_static_library)
	{
		/* lib.exe /NOLOGO /OUT:"output/dir/my_lib.lib" /LIBPATH:"output/dir/" a.obj b.obj c.obj ... */
		tmp = silk_tmp_sprintf("lib.exe /NOLOGO /OUT:\"%s\"  /LIBPATH:\"%s\" %s ", artefact, output_dir, str_obj.data);
		if (silk_process_in_directory(tmp, output_dir) != 0)
		{
			silk_log_error("Could not execute command to build static library\n");
			silk_set_and_goto(artefact, NULL, exit);
		}
	}

exit:
	silk_dstr_destroy(&str);
	silk_dstr_destroy(&str_obj);

	return artefact;
}

SILK_API silk_toolchain
silk_toolchain_msvc(void)
{
	silk_toolchain tc;
	tc.bake = silk_toolchain_msvc_bake;
	tc.name = "msvc";
	tc.default_directory_base = ".build/msvc";
	return tc;
}

#else

/* ================================================================ */
/* toolchain GCC */
/* ================================================================ */

/* #gcc #toolchain */

SILK_INTERNAL silk_bool
silk_strv_ends_with(silk_strv sv, silk_strv rhs)
{
	silk_strv sub = { 0 };
	if (sv.size < rhs.size)
	{
		return silk_false;
	}

	sub = silk_strv_make(sv.data + (sv.size - rhs.size), rhs.size);
	return silk_strv_equals_strv(sub, rhs);
}

SILK_API const char*
silk_toolchain_gcc_bake(silk_toolchain* tc, const char* project_name)
{
	silk_dstr str;
	silk_dstr str_obj; /* to keep track of the .o generated */
	const char* output_dir;        /* Output directory. Contains the directory path of the binary being created. */
	silk_darrT(const char*) objects; /* Contains the path of all .o objects */

	silk_bool is_exe = silk_false;
	silk_bool is_static_library = silk_false;
	silk_bool is_shared_library = silk_false;

	const char* ext = "";
	silk_kv_range range = { 0 };
	silk_kv current = { 0 };       /* Temporary kv to store results. */
	silk_kv_range lflag_range = { 0 };
	silk_kv current_lflag = { 0 };
	 
	silk_strv basename = { 0 };

	const char* linked_output_dir;
	silk_strv linked_project_name = { 0 };
	silk_project_t* linked_project = NULL;
	const char* tmp; /* Temp string */
	const char* current_object = NULL;
	silk_size i = 0;
	const char* _ = "  ";        /* Space to separate command arguments */
	silk_size tmp_index = 0;           /* to save temporary allocation index */
	const char* artefact = NULL; /* Resulting artifact path */

	silk_project_t* project = NULL;
	
	project = silk_find_project_by_name_str(project_name);
	
	if (!project)
	{
		return NULL;
	}

	/* gcc command */
	
	silk_dstr_init(&str);

	silk_dstr_init(&str_obj);

	silk_darrT_init(&objects);

	/* Get and format output directory */
	output_dir = silk_get_output_directory(project, tc);

	/* Create output directory if it does not exist yet. */
	silk_create_directories(output_dir, strlen(output_dir));

	/* Start command */
	silk_dstr_append_str(&str, "cc ");

	/* Handle binary type */
	is_exe = silk_property_equals(project, silk_BINARY_TYPE, silk_EXE);
	is_shared_library = silk_property_equals(project, silk_BINARY_TYPE, silk_SHARED_LIBRARY);
	is_static_library = silk_property_equals(project, silk_BINARY_TYPE, silk_STATIC_LIBRARY);

	if (!is_exe && !is_shared_library && !is_static_library)
	{
		silk_log_error("Unknown binary type");
		silk_set_and_goto(artefact, NULL, exit);
	}

	ext = is_exe ? "" : ext; /* do not provide extension to executables on linux */
	ext = is_static_library ? ".a" : ext;
	ext = is_shared_library ? ".so" : ext;

	/* Append compiler flags */
	{
		range = silk_mmap_get_range_str(&project->mmap, silk_CXFLAGS);
		while (silk_mmap_range_get_next(&range, &current))
		{
			silk_dstr_append_strv(&str, current.u.strv);
			silk_dstr_append_str(&str, _);
		}
	}

	/* Append include directories */
	{
		range = silk_mmap_get_range_str(&project->mmap, silk_INCLUDE_DIRECTORIES);
		while (silk_mmap_range_get_next(&range, &current))
		{
			tmp_index = silk_tmp_save();
			silk_dstr_append_f(&str, "-I \"%s\" ", silk_path_get_absolute_dir(current.u.strv.data));
			silk_tmp_restore(tmp_index);
		}
	}

	/* Append preprocessor definition */
	{
		range = silk_mmap_get_range_str(&project->mmap, silk_DEFINES);
		while (silk_mmap_range_get_next(&range, &current))
		{
			silk_dstr_append_f(&str, "-D%.*s ", current.u.strv);
		}
	}

	if (is_static_library)
	{
		artefact = silk_tmp_sprintf("%slib%s%s", output_dir, project_name, ext);
		silk_dstr_append_str(&str, "-c ");
	}

	if (is_shared_library)
	{
		silk_dstr_append_str(&str, "-shared ");
		artefact = silk_tmp_sprintf("%slib%s%s", output_dir, project_name, ext);
		silk_dstr_append_f(&str, "-o \"%s\" ", artefact);
	}

	if (is_exe)
	{
		artefact = silk_tmp_sprintf("%s%s", output_dir, project_name);
		silk_dstr_append_f(&str, "-o \"%s\" ", artefact);
	}

	/* Append .c files and .obj */
	{
		range = silk_mmap_get_range_str(&project->mmap, silk_FILES);
		while (silk_mmap_range_get_next(&range, &current))
		{
			/* Absolute file is created using the tmp buffer allocator but we don't need it once it's inserted into the dynamic string */
			tmp_index = silk_tmp_save();
			/* add .c files */
			silk_dstr_append_f(&str, "\"%s\" ", silk_path_get_absolute_file(current.u.strv.data));
			silk_tmp_restore(tmp_index);
			
			basename = silk_path_basename(current.u.strv);

			if (is_exe || is_static_library)
			{
				/* output/dir/my_object.o */
				silk_dstr_append_f(&str_obj, "\"%s%.*s.o\" ", output_dir, basename.size, basename.data);

				/* my_object.o */
				silk_darrT_push_back(&objects, silk_tmp_sprintf("%.*s.o", basename.size, basename.data));
			}
		}
	}

	/* Append libraries */
	{
		range = silk_mmap_get_range_str(&project->mmap, silk_LIBRARIES);
		while (silk_mmap_range_get_next(&range, &current))
		{
			silk_dstr_append_f(&str, "-l \"%s\" ", current.u.strv.data);
		}
	}

	/* For each linked project we add the link information to the gcc command */
	range = silk_mmap_get_range_str(&project->mmap, silk_LINK_PROJECTS);
	if (range.count > 0)
	{
		/* Add linker flags */
		{
			lflag_range = silk_mmap_get_range_str(&project->mmap, silk_LFLAGS);
			while (silk_mmap_range_get_next(&lflag_range, &current_lflag))
			{
				silk_dstr_append_strv(&str, current_lflag.u.strv);
				silk_dstr_append_str(&str, _);
			}
		}

		/* Give some parameters to the linker to  look for the shared library next to the binary being built */
		silk_dstr_append_str(&str, " -Wl,-rpath,$ORIGIN ");

		while (silk_mmap_range_get_next(&range, &current))
		{
			linked_project_name = current.u.strv;

			linked_project = silk_find_project_by_name(linked_project_name);
			if (!project)
			{
				silk_set_and_goto(artefact, NULL, exit);
			}

			linked_output_dir = silk_get_output_directory(linked_project, tc);

			/* Is static lib or shared lib */
			if (silk_property_equals(linked_project, silk_BINARY_TYPE, silk_STATIC_LIBRARY)
				|| silk_property_equals(linked_project, silk_BINARY_TYPE, silk_SHARED_LIBRARY))
			{
				/* -L "my/path/" -l "my_proj" */ 
				silk_dstr_append_f(&str, "-L \"%s\" -l \"%.*s\" ", linked_output_dir, linked_project_name.size, linked_project_name.data);
			}

			/* Is shared library */
			if (silk_property_equals(linked_project, silk_BINARY_TYPE, silk_SHARED_LIBRARY))
			{
				/* libmy_project.so*/
				tmp = silk_tmp_sprintf("%slib%.*s.so", linked_output_dir, linked_project_name.size, linked_project_name.data);

				if (!silk_copy_file_to_dir(tmp, output_dir))
				{
					silk_set_and_goto(artefact, NULL, exit);
				}
			}
		}
	}

	/* Example: gcc <includes> -c  <c source files> */
	if (silk_process_in_directory(str.data, output_dir) != 0)
	{
		silk_set_and_goto(artefact, NULL, exit);
	}

	if (is_static_library || is_shared_library)
	{
		if (is_static_library)
		{
			/* Create libXXX.a in the output directory */
			/* Example: ar -crs libMyLib.a MyObjectAo MyObjectB.o */
			tmp = silk_tmp_sprintf("ar -crs \"%s\" %s ", artefact, str_obj.data);
			if (silk_process_in_directory(tmp, output_dir) != 0)
			{
				silk_set_and_goto(artefact, NULL, exit);
			}
		}
	}

exit:
	silk_dstr_destroy(&str);
	silk_dstr_destroy(&str_obj);

	return artefact;
}

SILK_API silk_toolchain
silk_toolchain_gcc()
{
	silk_toolchain tc;
	tc.bake = silk_toolchain_gcc_bake;
	tc.name = "gcc";
	tc.default_directory_base = ".build/gcc";
	return tc;
}

#endif /* #else of _WIN32 */

SILK_API silk_toolchain
silk_toolchain_default(void)
{
#ifdef _WIN32
	return silk_toolchain_msvc();
#else
	return silk_toolchain_gcc();
#endif
}

#endif /* SILK_IMPLEMENTATION_CPP  */

#endif /* SILK_IMPLEMENTATION */
