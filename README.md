# Silk Build System
Silk is the best build system ever. It originates from an idea that I believe originates from
TSoding's nobuild.h project. The idea is that you shouldn't need another language to build your
projects. In this case the only language we need is C. This is a minimalist's favourite build
system, but I can't imagine it's much more than that. Nobuild never became popular, so I can't
imagine this going mainstream. I personally might use it someday in my projects though ;)

## Install
```sh
sudo ./install.sh
```

## Example
```c
#define SILK_IMPLEMENTATION
#include <silk.h>
#include <silk/assert.h>

int main()
{
    silk_init();

    silk_project("my project");
    silk_set(silk_BINARY_TYPE, silk_STATIC_LIBRARY);

    silk_add(silk_FILES, "main.c"); /* Assuming that you
                                     * have a file called
                                     * "main.c" in $pwd. */

    silk_assert_file_exists(
        silk_bake()
    );

    silk_destroy();

    return 0;
}
```
