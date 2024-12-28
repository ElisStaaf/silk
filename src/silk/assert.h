#ifndef SILK_ASSERT_H
#define SILK_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#define silk_assert_int_equals(expected, expression) \
	do { \
		int result = (expression); \
		if (result != expected) { \
			silk_log_error("Result '%d' is not equal to the expected value '%d' : %s %d", result, expected, __FILE__, __LINE__); \
			exit(1); \
		} \
	} while (0);

#define silk_assert_int_not_equals(expected, expression) \
	do { \
		int result = (expression); \
		if (result == expected) { \
			silk_log_error("Result '%d' is equal to the expected value '%d' : %s %d", result, expected, __FILE__, __LINE__); \
			exit(1); \
		} \
	} while (0);

#define silk_assert_run(command) \
	silk_assert_int_equals(0, silk_run(command))

#define silk_assert_true(expression) \
	do { \
		int result = (expression); \
		if (result == 0) { \
			silk_log_error("Expression is not true : %s %d", __FILE__, __LINE__); \
			exit(1); \
		} \
	} while (0);

#define silk_assert_false(expression) \
	do { \
		int result = (expression); \
		if (result != 0) { \
			silk_log_error("Expression is not false : %s %d", __FILE__, __LINE__); \
			exit(1); \
		} \
	} while (0);

SILK_API void silk_assert_file_exists(const char* filepath);
SILK_API void silk_assert_file_exists_f(const char* format, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SILK_ASSERT_H */

#ifdef SILK_IMPLEMENTATION

SILK_API void
silk_assert_file_exists(const char* filepath)
{
	if (!silk_path_exists(filepath))
	{
		silk_log_error("File does not exist: %s", filepath);
		exit(1);
	}
}

SILK_API void
silk_assert_file_exists_f(const char* format, ...)
{
	va_list args;
	const char* str = NULL;
	va_start(args, format);
	
	str = silk_tmp_vsprintf(format, args);
	silk_assert_file_exists(str);

	va_end(args);
}

#endif /* SILK_IMPLEMENTATION */
