#define SILK_IMPLEMENTATION
#include <silk.h>
#include <silk/assert.h>

int main(void)
{
    const char* path = NULL;

    silk_init();

    silk_project("exe");
    silk_set(silk_BINARY_TYPE, silk_EXE);

    silk_add(silk_FILES, "src/main.c");

    path = silk_bake();

    silk_assert_file_exists(path);
    
    silk_assert_run(path);

    silk_destroy();

    return 0;
}
