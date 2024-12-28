#ifndef SILK_DUMP_H
#define SILK_DUMP_H

/*
   Display all keys and values of all projects.
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Display properties of all project into a string. */
SILK_API const char* silk_dump_to_str(void);

/* Display properties of all project into a file. */
SILK_API void silk_dump_to_file(FILE* file);

/* Same as silk_dump_to_file using stdout as file. */
SILK_API void silk_dump(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SILK_DUMP_H */

#ifdef SILK_IMPLEMENTATION

SILK_API const char*
silk_dump_to_str(void)
{
    silk_dstr str;
    const char* result;
    silk_kv_range projects_range;
    silk_kv current_project;
    silk_kv_range properties_range;
    silk_kv current_property;

    silk_project_t* p = NULL;
    silk_context* ctx = silk_current_context();

    silk_dstr_init(&str);

    projects_range = silk_mmap_get_range_all(&ctx->projects);
    while (silk_mmap_range_get_next(&projects_range, &current_project))
    {
        p = (silk_project_t*)current_project.u.ptr;

        silk_dstr_append_f(&str, "Project '%s'\n", p->name.data);
        properties_range = silk_mmap_get_range_all(&p->mmap);
        while (silk_mmap_range_get_next(&properties_range, &current_property))
        {
            silk_dstr_append_f(&str, "%s : %s \n", current_property.key.data, current_property.u.strv.data);
        }
    }
    result = silk_tmp_str(str.data);

    silk_dstr_destroy(&str);

    return result;
}

SILK_API void
silk_dump_to_file(FILE* file)
{
    fprintf(file, "%s", silk_dump_to_str());
}

SILK_API void
silk_dump(void)
{
    fprintf(stdout, "%s", silk_dump_to_str());
}

#endif /* SILK_IMPLEMENTATION */
