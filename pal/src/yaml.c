#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "yaml.h"

#define ALEN(_a) (sizeof(_a)/sizeof(*_a))

#define TRY_ERRNO(_op) do{ \
    if(!(_op)) \
        fatal("%s: %s\n", #_op, strerror(errno)); \
}while(0)

size_t pal_error_count(pal_context_t *sd)
{
    assert(sd != NULL);
    return sd->error_count;
}

void pal_error(pal_context_t *ctx, const pal_config_node_t* node, const char *fmt, ...)
{
    assert(ctx != NULL);
    assert(fmt != NULL);


    const size_t size = 4092;
    char* buf = malloc(size);
    if (!buf) {
        const char* e = "INTERNAL ERROR: Out of memory.\n";
        write(STDERR_FILENO, e, strlen(e));
    }

    // Copy ERROR to start of buffer.
    const yaml_node_t* yaml = &node->yaml;
    const yaml_mark_t* start = &yaml->start_mark;
    const yaml_mark_t* end   = &yaml->end_mark;
    int cnt = snprintf(buf, size - 2, "Config error (%zu:%zu - %zu:%zu):\t", start->line + 1, start->column + 1, end->line + 1, end->column + 1);
    if (cnt < 0) {
        const char* e = "INTERNAL ERROR: Cannot format error message.\n";
        write(STDERR_FILENO, e, strlen(e));
        free(buf);
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    int res = vsnprintf(buf + cnt, sizeof size - cnt - 2, fmt, ap);
    va_end(ap);
    if (res < 0) {
        const char* e = "INTERNAL ERROR: Cannot format error message.\n";
        write(STDERR_FILENO, e, strlen(e));
        free(buf);
        return;
    }
    cnt += res;
    memcpy(buf + cnt, "\n", 2);
    write(STDERR_FILENO, buf, cnt + 2);
    free(buf);
    ++ctx->error_count;
}

static
void errprintf(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char* buf;
    int cnt = vasprintf(&buf, fmt, ap);
    if (cnt < 0) {
        const char* e = "INTERNAL ERROR: Cannot write error message.\n";
        write(STDERR_FILENO, e, strlen(e));
    } else {
        write(STDERR_FILENO, buf, cnt);
        free(buf);
    }
    va_end(ap);
}

static
pal_yaml_result_t pal_yaml_subdoc_open(yaml_document_t* doc, const pal_config_node_t** root, const char *fname)
{
    assert(fname != NULL);
    *root = NULL;

    // Open file.
    FILE *fp;
    if (!(fp = fopen(fname, "rb"))) {
        errprintf("Error opening file %s: %s\n", fname, strerror(errno));
        return PAL_YAML_NOT_FOUND;
    }

    // Parse YAML
    yaml_parser_t parser;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, fp);
    int success = yaml_parser_load(&parser, doc);
    fclose(fp);

    if (!success) {
        errprintf("Error parsing %s: %s at %ld:%ld in %s at %ld:%ld",
            fname, parser.problem,
            parser.problem_mark.line, parser.problem_mark.column,
            parser.context,
            parser.context_mark.line, parser.context_mark.column);
    }

    yaml_parser_delete(&parser);

    if (success && !(*root = mk_node(yaml_document_get_root_node(doc)))) {
        errprintf("Error: %s is empty", fname);
        success = false;
    }

    if (!success) {
        yaml_document_delete(doc);
        return PAL_YAML_PARSER_ERROR;
    }

    return PAL_YAML_OK;
}

void pal_context_flush_errors(pal_context_t *sd)
{
    sd->error_count = 0;
}

static const char *node_type_strings[] = {
    [YAML_NO_NODE]       = "empty node",
    [YAML_SCALAR_NODE]   = "scalar",
    [YAML_MAPPING_NODE]  = "mapping",
    [YAML_SEQUENCE_NODE] = "sequence",
};

static yaml_node_t *lookup_in_mapping(yaml_document_t *doc, const yaml_node_t *node, const char *key)
{
    assert(node != NULL);
    assert(node->type == YAML_MAPPING_NODE);
    assert(key != NULL);

    yaml_node_pair_t *pair;
    yaml_node_pair_t *start = node->data.mapping.pairs.start;
    yaml_node_pair_t *top = node->data.mapping.pairs.top;

    for(pair = start; pair < top; ++pair) {
        yaml_node_t *key_node;

        key_node = yaml_document_get_node(doc, pair->key);
        assert(key_node != NULL);
        assert(key_node->type == YAML_SCALAR_NODE);

        if (!strcmp((const char *)key_node->data.scalar.value, key))
            return yaml_document_get_node(doc, pair->value);
    }

    return NULL;
}

yaml_node_t *lookup_in_sequence(yaml_document_t *doc, const yaml_node_t *node, size_t idx)
{
    assert(node != NULL);
    assert(node->type == YAML_SEQUENCE_NODE);

    if(node->data.sequence.items.start + idx
            >= node->data.sequence.items.top)
        return NULL;

    return yaml_document_get_node(doc,
            node->data.sequence.items.start[idx]);
}

static bool node_is_null(const yaml_node_t* node)
{
    assert(node != NULL);

    return node->type == YAML_SCALAR_NODE
            && node->data.scalar.style
                    != YAML_SINGLE_QUOTED_SCALAR_STYLE
            && node->data.scalar.style
                    != YAML_DOUBLE_QUOTED_SCALAR_STYLE
            && !strcasecmp((const char *)node->data.scalar.value,
                    "null");
}

static pal_yaml_result_t deref_object_field(pal_context_t *ctx, const pal_config_node_t** res, pal_bool required, const pal_config_node_t* node, const char* field)
{
    if (node->yaml.type != YAML_MAPPING_NODE) {
        pal_error(ctx, node,
                  "Expected object with field %s but got %s",
                  field,
                  node_type_strings[node->yaml.type]);
        return PAL_YAML_TYPE_ERROR;
    }
    yaml_node_t* r = lookup_in_mapping(ctx->doc, &node->yaml, field);
    if(!r || node_is_null(r)) {
        if (required)
            pal_error(ctx, node, "Expected field named '%s'.", field);
        return PAL_YAML_NOT_FOUND;
    }

    *res = mk_node(r);
    return PAL_YAML_OK;
}

static
pal_yaml_result_t
vpal_config_node(const pal_config_node_t **res,
          pal_context_t *sd, const pal_config_node_t* root,
          pal_bool required, size_t count, va_list ap)
{
    assert(res != NULL);
    assert(sd != NULL);

    *res = NULL;

    const pal_config_node_t* node = root;
    while (count) {
        const char* field = va_arg(ap, const char*);
        pal_yaml_result_t r = deref_object_field(sd, &node, required, node, field);
        if (r != PAL_YAML_OK) return r;
        --count;
    }

    *res = node;
    return PAL_YAML_OK;
}

static
pal_yaml_result_t pal_node_as_scalar(const unsigned char **str, const char* expected, pal_context_t *sd, const pal_config_node_t* node) {
    if(node->yaml.type != YAML_SCALAR_NODE) {
        pal_error(sd, node, "Expected %s but got %s.", expected, node_type_strings[node->yaml.type]);
        return PAL_YAML_TYPE_ERROR;
    }

    *str = node->yaml.data.scalar.value;
    return PAL_YAML_OK;
}

bool hasInvalidChars(pal_context_t *ctx, const pal_config_node_t* node, const unsigned char* v) {
    for (const unsigned char* p = v; *p; ++p) {
        if (*p >= 0x80) {
            pal_error(ctx, node, "String contains invalid character %x (%s)\n", *p, (const char*) v);
            return true;
        }
    }
    return false;
}

pal_yaml_result_t
pal_config_node(
    const pal_config_node_t** out,
    pal_context_t *ctx, const pal_config_node_t* root,
    pal_bool required, size_t count, ...) {

    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t r = vpal_config_node(out, ctx, root, required, count, ap);
    va_end(ap);
    return r;
}


pal_yaml_result_t pal_config_string(const char **str,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(str != NULL);
    assert(ctx != NULL);

    const pal_config_node_t* node;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t res = vpal_config_node(&node, ctx, root, required, count, ap);
    va_end(ap);
    if (res != PAL_YAML_OK) return res;

    const unsigned char* v;
    res = pal_node_as_scalar(&v, "string", ctx, node);
    if (res != PAL_YAML_OK) return res;

    if (hasInvalidChars(ctx, node, v))
        return PAL_YAML_PARSER_ERROR;

    *str = strdup((const char*) v);
    if (!*str) {
        pal_error(ctx, node, "Memory allocation failed.");
        return PAL_YAML_PARSER_ERROR;
    }
    return PAL_YAML_OK;
}

pal_yaml_result_t pal_config_static_string(char *str,
        size_t sz, pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(str != NULL);
    assert(sd != NULL);

    const pal_config_node_t* node;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t res = vpal_config_node(&node, sd, root, required, count, ap);
    va_end(ap);
    if (res != PAL_YAML_OK) return res;

    const unsigned char* v;
    res = pal_node_as_scalar(&v, "string", sd, node);
    if (res != PAL_YAML_OK) return res;

    snprintf(str, sz, "%s", (const char*) v);
    return PAL_YAML_OK;
}

static pal_yaml_result_t vpal_config_int(const pal_config_node_t** node, int64_t *val,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, va_list ap)
{
    pal_yaml_result_t res = vpal_config_node(node, sd, root, required, count, ap);
    if (res != PAL_YAML_OK) return res;

    const unsigned char* str;
    res = pal_node_as_scalar(&str, "signed integer", sd, *node);
    if (res != PAL_YAML_OK) return res;

    char *endp;
    errno = 0;
    *val = strtoll((const char *)str, &endp, 10);
    if(!errno && !*endp) { // TODO: Check bounds based on vsize
        return PAL_YAML_OK;
    } else {
        pal_error(sd, *node,
                "`%s' could not be parsed as a signed integer",
                (const char *) str,
                errno ? strerror(errno)
                        : "Contains non-numeric characters");
        return PAL_YAML_TYPE_ERROR;
    }
}

pal_yaml_result_t pal_config_int64(int64_t *val,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(val != NULL);

    const pal_config_node_t* node;
    int64_t val64;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t ret = vpal_config_int(&node, &val64, ctx, root, required, count, ap);
    va_end(ap);
    if (ret != PAL_YAML_OK) return ret;

    *val = val64;
    return PAL_YAML_OK;
}

pal_yaml_result_t pal_config_int32(int32_t *val,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(val != NULL);

    const pal_config_node_t* node;
    int64_t val64;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t ret = vpal_config_int(&node, &val64, ctx, root, required, count, ap);
    va_end(ap);
    if (ret != PAL_YAML_OK) return ret;

    if ((int64_t) (int32_t) val64 != val64) {
        pal_error(ctx, node, "Expected 32bit int.");
        return PAL_YAML_TYPE_ERROR;
    }

    *val = val64;
    return PAL_YAML_OK;
}

pal_yaml_result_t pal_config_int16(int16_t *val,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(val != NULL);

    const pal_config_node_t* node;
    int64_t val64;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t ret = vpal_config_int(&node, &val64, sd, root, required, count, ap);
    va_end(ap);
    if (ret != PAL_YAML_OK) return ret;

    if ((int64_t) (int16_t) val64 != val64) {
        pal_error(sd, node, "Expected 16bit int.");
        return PAL_YAML_TYPE_ERROR;
    }

    *val = val64;
    return PAL_YAML_OK;
}

static pal_yaml_result_t
vpal_config_uint(const pal_config_node_t** node, uint64_t *val,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, va_list ap)
{
    assert(val != NULL);
    assert(sd != NULL);

    pal_yaml_result_t res = vpal_config_node(node, sd, root, required, count, ap);
    if (res != PAL_YAML_OK) return res;

    const unsigned char* str;
    res = pal_node_as_scalar(&str, "signed integer", sd, *node);
    if (res != PAL_YAML_OK) return res;

    char *endp;
    errno = 0;
    *val = strtoull((const char *) str, &endp, 10);
    if (!errno && !*endp) {
        return PAL_YAML_OK;
    } else {
        pal_error(sd, *node,
                "`%s' could not be parsed as an "
                "unsigned integer: %s",
                (const char *) str,
                errno ? strerror(errno)
                        : "Contains non-numeric characters");
        return PAL_YAML_TYPE_ERROR;
    }
}

pal_yaml_result_t pal_config_uint64(uint64_t *val,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(val != NULL);

    const pal_config_node_t* node;
    uint64_t val64;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t ret = vpal_config_uint(&node, &val64, sd, root, required, count, ap);
    va_end(ap);
    if (ret != PAL_YAML_OK) return ret;

    *val = val64;
    return PAL_YAML_OK;
}

pal_yaml_result_t pal_config_uint32(uint32_t *val,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(val != NULL);

    const pal_config_node_t* node;
    uint64_t val64;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t ret = vpal_config_uint(&node, &val64, sd, root, required, count, ap);
    va_end(ap);
    if (ret != PAL_YAML_OK) return ret;

    if ((uint64_t) (uint32_t) val64 != val64) {
        pal_error(sd, node, "Expected 32bit word.");
        return PAL_YAML_TYPE_ERROR;
    }
    *val = val64;
    return PAL_YAML_OK;
}

pal_yaml_result_t pal_config_uint16(uint16_t *val,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(val != NULL);

    const pal_config_node_t* node;
    uint64_t val64;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t ret = vpal_config_uint(&node, &val64, sd, root, required, count, ap);
    va_end(ap);
    if (ret != PAL_YAML_OK) return ret;

    if ((uint64_t) (uint16_t) val64 != val64) {
        pal_error(sd, node, "Expected 16bit word.");
        return PAL_YAML_TYPE_ERROR;
    }
    *val = val64;
    return PAL_YAML_OK;
}

pal_yaml_result_t pal_config_double(double *valp,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(valp != NULL);
    assert(sd != NULL);

    const pal_config_node_t* node;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t res = vpal_config_node(&node, sd, root, required, count, ap);
    va_end(ap);
    if (res != PAL_YAML_OK) return res;

    const unsigned char* str;
    res = pal_node_as_scalar(&str, "decimal number", sd, node);
    if (res != PAL_YAML_OK) return res;

    char *endp;
    errno = 0;
    double val = strtod((const char *) str, &endp);
    if(!errno && !*endp) {
        *valp = val;
        return PAL_YAML_OK;
    } else {
        pal_error(sd, node,
                "`%s' could not be parsed as a decimal number",
                (const char *) str,
                errno ? strerror(errno)
                        : "Contains non-numeric characters");
        return PAL_YAML_TYPE_ERROR;
    }
}

pal_yaml_result_t pal_config_bool(bool *val,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(val != NULL);
    assert(sd != NULL);

    const pal_config_node_t* node;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t res = vpal_config_node(&node, sd, root, required, count, ap);
    va_end(ap);
    if (res != PAL_YAML_OK) return res;

    const unsigned char* str;
    res = pal_node_as_scalar(&str, "boolean", sd, node);
    if (res != PAL_YAML_OK) return res;

    if(!strcasecmp((const char *) str, "true")) {
        *val = true;
        return PAL_YAML_OK;
    } else if (!strcasecmp((const char *) str, "false")) {
        *val = false;
        return PAL_YAML_OK;
    } else {
        pal_error(sd, node,
                "`%s' could not be parsed as a boolean",
                (const char *) str);
        return PAL_YAML_TYPE_ERROR;
    }
}

pal_yaml_result_t pal_config_enum(int *val,
        pal_yaml_enum_schema_t *enums,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(val != NULL);
    assert(enums != NULL);
    assert(sd != NULL);

    const pal_config_node_t* node;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t res = vpal_config_node(&node, sd, root, required, count, ap);
    va_end(ap);
    if(res != PAL_YAML_OK) return res;

    const unsigned char* str;
    res = pal_node_as_scalar(&str, "enumeration value", sd, node);
    if (res != PAL_YAML_OK) return res;

    for(; enums->name; ++enums) {
        if (!strcmp((const char *) str, enums->name)) {
            *val = enums->value;
            return PAL_YAML_OK;
        }
    }

    pal_error(sd, node,
            "No such enumeration value `%s'",
            (const char *) str);
    return PAL_YAML_TYPE_ERROR;
}

pal_yaml_result_t pal_config_flags(int *val,
        pal_yaml_enum_schema_t *enums,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(val != NULL);
    assert(enums != NULL);
    assert(sd != NULL);

    const pal_config_node_t* node;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t res = vpal_config_node(&node, sd, root, required, count, ap);
    va_end(ap);
    if (res != PAL_YAML_OK) return res;

    assert(node != NULL);

    if(node->yaml.type != YAML_SEQUENCE_NODE) {
        pal_error(sd, node,
                "Expected flags sequence but got %s",
                node_type_strings[node->yaml.type]);
        return PAL_YAML_TYPE_ERROR;
    }

    int val_temp = 0;
    yaml_node_item_t *item;
    for(item = node->yaml.data.sequence.items.start;
            item < node->yaml.data.sequence.items.top; ++item) {
        assert(item != NULL);

        yaml_node_t *elem = yaml_document_get_node(sd->doc, *item);
        assert(elem != NULL);

        if(elem->type != YAML_SCALAR_NODE) {
            pal_error(sd, node,
                "Expected flags enumeration value but got %s.",
                node_type_strings[elem->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        pal_yaml_enum_schema_t *e = enums;
        bool found = false;
        for(e = enums; e->name; ++e) {
            if(!strcmp((char *)elem->data.scalar.value,
                        e->name)) {
                val_temp |= e->value;
                found = true;
            }
        }

        if(!found) {
            pal_error(sd, node,
                "No such enumeration value `%s'.",
                (char *)elem->data.scalar.value);
            return PAL_YAML_TYPE_ERROR;
        }
    }

    *val = val_temp;
    return PAL_YAML_OK;
}

pal_yaml_result_t pal_config_sequence(
        const pal_config_node_t*** seqPtr, size_t *lenPtr,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...) {
    const pal_config_node_t* node;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t res = vpal_config_node(&node, sd, root, required, count, ap);
    va_end(ap);

    if (res != PAL_YAML_OK) return res;
    assert(node != NULL);

    if(node->yaml.type != YAML_SEQUENCE_NODE) {
        pal_error(sd, node,
                "Expected sequence but got %s",
                node_type_strings[node->yaml.type]);
        return PAL_YAML_TYPE_ERROR;
    }
    size_t  len = node->yaml.data.sequence.items.top
                - node->yaml.data.sequence.items.start;

    const pal_config_node_t** array;
    TRY_ERRNO(array = calloc(len, sizeof(const pal_config_node_t*)));
    for (size_t i = 0; i != len; ++i) {
        const yaml_node_t* r = yaml_document_get_node(sd->doc, node->yaml.data.sequence.items.start[i]);
        if (!r || node_is_null(r)) {
            pal_error(sd, node, "No such sequence index %lu", i);
            free(array);
            return PAL_YAML_PARSER_ERROR;
        }
        array[i] = mk_node(r);
    }

    *seqPtr = array;
    *lenPtr = len;
    return PAL_YAML_OK;
}

pal_yaml_result_t pal_config_string_sequence(
        const char ***seqPtr, size_t *lenPtr,
        pal_context_t *sd, const pal_config_node_t* root,
        pal_bool required, size_t count, ...)
{
    assert(sd != NULL);
    *seqPtr = 0;
    *lenPtr = 0;

    const pal_config_node_t* node;
    va_list ap;
    va_start(ap, count);
    pal_yaml_result_t res = vpal_config_node(&node, sd, root, required, count, ap);
    va_end(ap);
    if (res != PAL_YAML_OK) return res;

    if (node->yaml.type != YAML_SEQUENCE_NODE) {
        pal_error(sd, node,
                "Expected sequence but got %s",
                node_type_strings[node->yaml.type]);
        return PAL_YAML_TYPE_ERROR;
    }

    size_t len = node->yaml.data.sequence.items.top
               - node->yaml.data.sequence.items.start;

    const char **strseq;
    TRY_ERRNO(strseq = calloc(len, sizeof *strseq));

    size_t i = 0;
    res = PAL_YAML_OK;
    while (i < len) {
        // Get node for ith element.
        const yaml_node_t* r = yaml_document_get_node(sd->doc, node->yaml.data.sequence.items.start[i]);
        if (!r || node_is_null(r)) {
            pal_error(sd, node, "No such sequence index %lu", i);
            res = PAL_YAML_NOT_FOUND;
            break;
        }
        // Extract string.
        const unsigned char* v;
        res = pal_node_as_scalar(&v, "string", sd, mk_node(r));
        if (res != PAL_YAML_OK) break;

        if (hasInvalidChars(sd, node, v)) {
            res = PAL_YAML_PARSER_ERROR;
            break;
        }
        strseq[i] = (const char*) v;
        ++i;
    }
    // Clean up if an error occured.
    if (res != PAL_YAML_OK) {
        free(strseq);
        return res;
    }
    // Finally, assign result and return.
    *lenPtr = len;
    *seqPtr = strseq;
    return PAL_YAML_OK;
}

struct top_level *load_yaml(yaml_document_t* doc, pal_context_t* ctx, const char *fname)
{
    assert(fname != NULL);

    const pal_config_node_t* root;
    if (pal_yaml_subdoc_open(doc, &root, fname) != PAL_YAML_OK) {
        return NULL;
    }

    // Initialize context
    ctx->doc = doc;
    ctx->error_count = 0;

    struct top_level *tlp = calloc(1, sizeof *tlp);
    {
        const pal_config_node_t** encs = 0;
        pal_config_sequence(&encs, &tlp->tl_encs_count, ctx, root, false, 1, "enclaves");
        if (tlp->tl_encs_count > 0)
            tlp->tl_encs = calloc(tlp->tl_encs_count, sizeof(*tlp->tl_encs));

        for (size_t i = 0; i < tlp->tl_encs_count; ++i) {
            struct enclave *e = &tlp->tl_encs[i];
            const pal_config_node_t* child = encs[i];
            pal_config_string(&e->enc_name, ctx, child, true, 1, "name");
            pal_config_string(&e->enc_path, ctx, child, false, 1, "path");
            pal_config_string_sequence(&e->enc_args, &e->enc_args_count, ctx, child, false, 1, "args");
            pal_config_string_sequence(&e->enc_env,  &e->enc_env_count,  ctx, child, false, 1, "env");
        }
        free(encs);
    }

    {
        const pal_config_node_t** rscs = 0;
        pal_config_sequence(&rscs, &tlp->tl_rscs_count, ctx, root, false, 1, "resources");
        if(tlp->tl_rscs_count > 0)
            tlp->tl_rscs = calloc(tlp->tl_rscs_count, sizeof(*tlp->tl_rscs));

        size_t i;

        for(i = 0; i < tlp->tl_rscs_count; ++i) {
            struct resource *r = &tlp->tl_rscs[i];
            const pal_config_node_t* child = rscs[i];
            pal_config_string(&r->r_name, ctx, child, true, 1, "name");
            pal_config_string(&r->r_type, ctx, child, true, 1, "type");
            pal_config_string_sequence(&r->r_ids, &r->r_ids_count, ctx, child, true, 1, "ids");
            pal_yaml_result_t res = deref_object_field(ctx, &r->r_config, true, child, "value");
            if (res != PAL_YAML_OK) break;
        }
        free(rscs);
    }

    {
        pal_yaml_enum_schema_t log_level_schema[] = {
            { "default", LOGLVL_DEFAULT },
            { "info",    LOGLVL_INFO },
            { "debug",   LOGLVL_DEBUG }
        };
        pal_config_enum((int*)&tlp->tl_cfg.cfg_loglvl, log_level_schema, ctx, root, false, 2, "config", "log_level");
    }
    pal_config_string(&tlp->tl_cfg.cfg_plugin_dir, ctx, root, false, 2, "config", "plugin_directory");

    if (pal_error_count(ctx) > 0) {
        pal_context_flush_errors(ctx);
        free_yaml(tlp);
        tlp = NULL;
    }

    return tlp;
}

void free_yaml(struct top_level *tlp)
{
    for(size_t i = 0; i < tlp->tl_encs_count; ++i) {
        struct enclave *e = &tlp->tl_encs[i];
        free((void*) e->enc_name);
        free((void*) e->enc_path);
        free(e->enc_args);
        free(e->enc_env);
    }
    free(tlp->tl_encs);
    for(size_t i = 0; i < tlp->tl_rscs_count; ++i) {
        struct resource *r = &tlp->tl_rscs[i];
        free((void*) r->r_name);
        free((void*) r->r_type);
        free(r->r_ids);
    }
    free(tlp->tl_rscs);
    free((void*) tlp->tl_cfg.cfg_plugin_dir);
    free(tlp);
}
