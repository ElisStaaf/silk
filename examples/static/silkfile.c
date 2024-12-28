#define SILK_IMPLEMENTATION
#include <silk.h>
#include <silk/assert.h>

int main(void)
{
    silk_init();

    silk_project("my project");
    silk_set(silk_BINARY_TYPE, silk_STATIC_LIBRARY);

    silk_add(silk_FILES, "str.c");
    silk_add(silk_FILES, "int.c")

    silk_assert_file_exists(
        silk_bake()
    );

    silk_destroy();

    return 0;
}
