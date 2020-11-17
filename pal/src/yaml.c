#include <assert.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "yaml.h"

#define ALEN(_a) (sizeof(_a)/sizeof(*_a))
#define ERROR_MAX (ALEN(((pal_yaml_subdoc_t*)0)->errors))

#define TRY_ERRNO(_op) do{ \
    if(!(_op)) \
        fatal("%s: %s\n", #_op, strerror(errno)); \
}while(0)

static void error_stack_free(struct pal_yaml_error_stack **topp)
{
    assert(topp != NULL);

    struct pal_yaml_error_stack *top = *topp;
    while(top) {
        struct pal_yaml_error_stack *temp = top;
        top = top->next;
        free(temp);
    }

    *topp = NULL;
}

static void error_stack_push(struct pal_yaml_error_stack **top,
        const char *fmt, va_list ap)
{
    assert(top != NULL);
    assert(fmt != NULL);

    struct pal_yaml_error_stack *new_top = *top;
    TRY_ERRNO(new_top = malloc(sizeof *new_top));

    vsnprintf(new_top->err, sizeof new_top->err, fmt, ap);
    new_top->next = *top;
    *top = new_top;
}

static void error_stack_append(struct pal_yaml_error_stack **dst,
        struct pal_yaml_error_stack *src1,
        struct pal_yaml_error_stack *src2)
{
    assert(dst != NULL);

    while(src1) {
        TRY_ERRNO(*dst = malloc(sizeof **dst));

        memcpy((*dst)->err, src1->err, sizeof src1->err);
        (*dst)->next = NULL;

        dst = &(*dst)->next;
        src1 = src1->next;
    }
    while(src2) {
        TRY_ERRNO(*dst = malloc(sizeof **dst));

        memcpy((*dst)->err, src2->err, sizeof src2->err);
        (*dst)->next = NULL;

        dst = &(*dst)->next;
        src2 = src2->next;
    }
}

size_t pal_yaml_subdoc_error_count(pal_yaml_subdoc_t *sd)
{
    assert(sd != NULL);
    return sd->error_count;
}

void pal_yaml_subdoc_error_context_push(pal_yaml_subdoc_t *sd,
        const char *fmt, ...)
{
    assert(sd != NULL);
    assert(fmt != NULL);

    if(sd->error_count >= ERROR_MAX)
        return;

    va_list ap;
    va_start(ap, fmt);

    error_stack_push(&sd->errors[sd->error_count], fmt, ap);

    va_end(ap);
}

void pal_yaml_subdoc_error_push(pal_yaml_subdoc_t *sd,
        const char *fmt, ...)
{
    assert(sd != NULL);
    assert(fmt != NULL);

    if(sd->error_count >= ERROR_MAX) {
        return;
    } else {
        va_list ap;
        va_start(ap, fmt);

        error_stack_push(&sd->errors[sd->error_count], fmt, ap);

        va_end(ap);
    }

    ++sd->error_count;
}

void pal_yaml_subdoc_error_pop(pal_yaml_subdoc_t *sd)
{
    assert(sd != NULL);

    if(sd->error_count > 0)
        error_stack_free(&sd->errors[--sd->error_count]);
}

void pal_yaml_subdoc_error_context_clear(pal_yaml_subdoc_t *sd)
{
    assert(sd != NULL);

    if(sd->error_count >= ERROR_MAX)
        return;

    error_stack_free(&sd->errors[sd->error_count]);
}

void pal_yaml_subdoc_log_errors(pal_yaml_subdoc_t *sd)
{
    assert(sd);

    char buf[8192], *bufp = buf;
    size_t bufsz = sizeof buf;

    size_t i;
    for(i = 0; i < sd->error_count && i < ERROR_MAX; ++i) {
        struct pal_yaml_error_stack *e;
        for(e = sd->errors[i]; e; e = e->next) {
            size_t s = snprintf(bufp, bufsz, "%s%s\n",
                    e == sd->errors[i] ? "" : "\t\t", e->err);
            bufsz -= s;
            bufp += s;
        }
        for(e = sd->context; e; e = e ->next) {
            size_t s = snprintf(bufp, bufsz, "\t\t%s\n", e->err);
            bufsz -= s;
            bufp += s;
        }
    }
    if(sd->error_count >= ERROR_MAX)
        snprintf(bufp, bufsz, "Too many errors\n");

    error(buf);
}

pal_yaml_result_t pal_yaml_subdoc_open(pal_yaml_subdoc_t *sd,
        const char *fname)
{
    assert(sd != NULL);
    assert(fname != NULL);

    FILE *fp;
    yaml_parser_t parser;

    strcpy(sd->filename, fname);
    memset(sd->errors, 0, sizeof sd->errors);
    sd->error_count = 0;
    sd->node = NULL;
    sd->context = NULL;

    if(!(fp = fopen(fname, "rb"))) {
        pal_yaml_subdoc_error_push(sd, "Error opening file %s: %s",
                fname, strerror(errno));
        return PAL_YAML_NOT_FOUND;
    }

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, fp);
    int success = yaml_parser_load(&parser, &sd->doc);

    if(!success)
        pal_yaml_subdoc_error_push(sd,
                "Error parsing %s: %s at %ld:%ld in %s at %ld:%ld",
                fname, parser.problem, parser.problem_mark.line,
                parser.problem_mark.column, parser.context,
                parser.context_mark.line,
                parser.context_mark.column);

    yaml_parser_delete(&parser);
    fclose(fp);

    if(success &&
            !(sd->node = yaml_document_get_root_node(&sd->doc))) {
        pal_yaml_subdoc_error_push(sd, "Error: %s is empty", fname);
        success = false;
    }

    if(!success) {
        yaml_document_delete(&sd->doc);
        return PAL_YAML_PARSER_ERROR;
    }

    sd->doc_ref_count = malloc(sizeof *sd->doc_ref_count);
    *sd->doc_ref_count = 1;

    return PAL_YAML_OK;
}

void pal_yaml_subdoc_clear_errors(pal_yaml_subdoc_t *sd)
{
    assert(sd != NULL);

    pal_yaml_subdoc_error_context_clear(sd);

    size_t i;
    while(sd->error_count)
        pal_yaml_subdoc_error_pop(sd);
}

void pal_yaml_subdoc_close(pal_yaml_subdoc_t *sd)
{
    if(sd) {
        if(--*sd->doc_ref_count == 0) {
            yaml_document_delete(&sd->doc);
            free(sd->doc_ref_count);
        }

        pal_yaml_subdoc_clear_errors(sd);
        error_stack_free(&sd->context);

        sd->node = NULL;
        sd->error_count = 0;
    }
}

static const char *error_strings[] = {
    [PAL_YAML_OK]           = "Success",
    [PAL_YAML_NOT_FOUND]    = "File not found",
    [PAL_YAML_PARSER_ERROR] = "Parser error",
    [PAL_YAML_TYPE_ERROR]   = "Type error",
};

const char *pal_yaml_strerror(pal_yaml_result_t res)
{
    assert(0 <= res && res <= ALEN(error_strings));

    return error_strings[res];
}

static const char *node_type_strings[] = {
    [YAML_NO_NODE]       = "empty node",
    [YAML_SCALAR_NODE]   = "scalar",
    [YAML_MAPPING_NODE]  = "mapping",
    [YAML_SEQUENCE_NODE] = "sequence",
};

static const char *elem_string(struct pal_yaml_elem *elem)
{
    assert(elem != NULL);

    static char buf[1024];

    switch(elem->type) {
        case PAL_YAML_ELEM_MAP:
            snprintf(buf, sizeof buf,
                    "mapping field `%s'", elem->map_elem);
            break;
        case PAL_YAML_ELEM_SEQ:
            snprintf(buf, sizeof buf,
                    "sequence index %lu", elem->seq_elem);
            break;
    }

    return buf;
}

static yaml_node_t *lookup_in_mapping(yaml_document_t *doc,
        yaml_node_t *node, const char *key)
{
    assert(node != NULL);
    assert(node->type == YAML_MAPPING_NODE);
    assert(key != NULL);

    yaml_node_pair_t *pair,
                     *start = node->data.mapping.pairs.start,
                     *top = node->data.mapping.pairs.top;

    for(pair = start; pair < top; ++pair) {
        yaml_node_t *key_node;

        key_node = yaml_document_get_node(doc, pair->key);
        assert(key_node != NULL);
        assert(key_node->type == YAML_SCALAR_NODE);

        if(!strcmp((const char *)key_node->data.scalar.value, key))
            return yaml_document_get_node(doc, pair->value);
    }

    return NULL;
}

yaml_node_t *lookup_in_sequence(yaml_document_t *doc,
        yaml_node_t *node, size_t idx)
{
    assert(node != NULL);
    assert(node->type == YAML_SEQUENCE_NODE);

    if(node->data.sequence.items.start + idx
            >= node->data.sequence.items.top)
        return NULL;

    return yaml_document_get_node(doc,
            node->data.sequence.items.start[idx]);
}

static bool node_is_null(yaml_node_t *node)
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

static pal_yaml_result_t find_node(yaml_node_t **res,
        pal_yaml_subdoc_t *sd, size_t depth, va_list ap)
{
    assert(res != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    yaml_node_t *node = sd->node;

    pal_yaml_subdoc_error_context_push(sd, "In %s at %s:%lu:%lu",
            node_type_strings[node->type], sd->filename,
            node->start_mark.line, node->start_mark.column);

    *res = NULL;

    size_t d;
    for(d = 0; d < depth; ++d) {
        pal_yaml_elem_t elem = va_arg(ap, pal_yaml_elem_t);

        switch(elem.type) {
            case PAL_YAML_ELEM_MAP:
                if(node->type != YAML_MAPPING_NODE) {
                    pal_yaml_subdoc_error_push(sd,
                            "Expected mapping but got %s",
                            node_type_strings[node->type]);
                    return PAL_YAML_TYPE_ERROR;
                }
                node = lookup_in_mapping(&sd->doc, node,
                        elem.map_elem);
                break;
            case PAL_YAML_ELEM_SEQ:
                if(node->type != YAML_SEQUENCE_NODE) {
                    pal_yaml_subdoc_error_push(sd,
                            "Expected sequence but got %s",
                            node_type_strings[node->type]);
                    return PAL_YAML_TYPE_ERROR;
                }
                node = lookup_in_sequence(&sd->doc, node,
                        elem.seq_elem);
                break;
        }

        if(!node || node_is_null(node)) {
            pal_yaml_subdoc_error_push(sd,
                    "No such %s", elem_string(&elem));
            return PAL_YAML_NOT_FOUND;
        }

        pal_yaml_subdoc_error_context_push(sd, "In %s at %s:%lu:%lu",
                elem_string(&elem), sd->filename,
                node->start_mark.line, node->start_mark.column);
    }

    *res = node;
    return PAL_YAML_OK;
}

pal_yaml_result_t pal_yaml_subdoc_find_string(char **str,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(str != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    va_list ap;
    va_start(ap, depth);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    va_end(ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        if(node->type != YAML_SCALAR_NODE) {
            pal_yaml_subdoc_error_push(sd,
                    "Expected string but got %s",
                    node_type_strings[node->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        TRY_ERRNO(*str = strdup((char*)node->data.scalar.value));
        pal_yaml_subdoc_error_context_clear(sd);
        return PAL_YAML_OK;
    } else {
        return res;
    }
}

pal_yaml_result_t pal_yaml_subdoc_find_static_string(char *str,
        size_t sz, pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(str != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    va_list ap;
    va_start(ap, depth);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    va_end(ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        if(node->type != YAML_SCALAR_NODE) {
            pal_yaml_subdoc_error_push(sd,
                    "Expected string but got %s",
                    node_type_strings[node->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        snprintf(str, sz, "%s", (char*)node->data.scalar.value);
        pal_yaml_subdoc_error_context_clear(sd);
        return PAL_YAML_OK;
    } else {
        return res;
    }
}

static pal_yaml_result_t find_int(int64_t *val, size_t vsize,
        pal_yaml_subdoc_t *sd, size_t depth, va_list ap)
{
    assert(val != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        if(node->type != YAML_SCALAR_NODE) {
            pal_yaml_subdoc_error_push(sd,
                    "Expected signed integer but got %s",
                    node_type_strings[node->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        char *endp;
        errno = 0;
        *val = strtoll((char *)node->data.scalar.value, &endp, 10);
        if(!errno && !*endp) { // TODO: Check bounds based on vsize
            pal_yaml_subdoc_error_context_clear(sd);
            return PAL_YAML_OK;
        } else {
            pal_yaml_subdoc_error_push(sd,
                    "`%s' could not be parsed as a signed integer",
                    (char *)node->data.scalar.value,
                    errno ? strerror(errno)
                          : "Contains non-numeric characters");
            return PAL_YAML_TYPE_ERROR;
        }
    } else {
        return res;
    }
}

pal_yaml_result_t pal_yaml_subdoc_find_int64(int64_t *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(val != NULL);

    va_list ap;
    va_start(ap, depth);

    int64_t val64;
    pal_yaml_result_t ret = find_int(&val64, sizeof *val,
            sd, depth, ap);

    va_end(ap);

    if(ret == PAL_YAML_OK)
        *val = val64;
    return ret;
}

pal_yaml_result_t pal_yaml_subdoc_find_int32(int32_t *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(val != NULL);

    va_list ap;
    va_start(ap, depth);

    int64_t val64;
    pal_yaml_result_t ret = find_int(&val64, sizeof *val,
            sd, depth, ap);

    va_end(ap);

    if(ret == PAL_YAML_OK)
        *val = val64;
    return ret;
}

pal_yaml_result_t pal_yaml_subdoc_find_int16(int16_t *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(val != NULL);

    va_list ap;
    va_start(ap, depth);

    int64_t val64;
    pal_yaml_result_t ret = find_int(&val64, sizeof *val,
            sd, depth, ap);

    va_end(ap);

    if(ret == PAL_YAML_OK)
        *val = val64;
    return ret;
}

pal_yaml_result_t pal_yaml_subdoc_find_int8(int8_t *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(val != NULL);

    va_list ap;
    va_start(ap, depth);

    int64_t val64;
    pal_yaml_result_t ret = find_int(&val64, sizeof *val,
            sd, depth, ap);

    va_end(ap);

    if(ret == PAL_YAML_OK)
        *val = val64;
    return ret;
}

static pal_yaml_result_t find_uint(uint64_t *val, size_t vsize,
        pal_yaml_subdoc_t *sd, size_t depth, va_list ap)
{
    assert(val != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        if(node->type != YAML_SCALAR_NODE) {
            pal_yaml_subdoc_error_push(sd,
                    "Expected unsigned integer but got %s",
                    node_type_strings[node->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        char *endp;
        errno = 0;
        *val = strtoull((char *)node->data.scalar.value, &endp, 10);
        if(!errno && !*endp) { // TODO: Check bounds based on vsize
            pal_yaml_subdoc_error_context_clear(sd);
            return PAL_YAML_OK;
        } else {
            pal_yaml_subdoc_error_push(sd,
                    "`%s' could not be parsed as an "
                    "unsigned integer: %s",
                    (char *)node->data.scalar.value,
                    errno ? strerror(errno)
                          : "Contains non-numeric characters");
            return PAL_YAML_TYPE_ERROR;
        }
    } else {
        return res;
    }
}

pal_yaml_result_t pal_yaml_subdoc_find_uint64(uint64_t *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(val != NULL);

    va_list ap;
    va_start(ap, depth);

    uint64_t val64;
    pal_yaml_result_t ret = find_uint(&val64, sizeof *val,
            sd, depth, ap);

    va_end(ap);

    if(ret == PAL_YAML_OK)
        *val = val64;
    return ret;
}

pal_yaml_result_t pal_yaml_subdoc_find_uint32(uint32_t *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(val != NULL);

    va_list ap;
    va_start(ap, depth);

    uint64_t val64;
    pal_yaml_result_t ret = find_uint(&val64, sizeof *val,
            sd, depth, ap);

    va_end(ap);

    if(ret == PAL_YAML_OK)
        *val = val64;
    return ret;
}

pal_yaml_result_t pal_yaml_subdoc_find_uint16(uint16_t *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(val != NULL);

    va_list ap;
    va_start(ap, depth);

    uint64_t val64;
    pal_yaml_result_t ret = find_uint(&val64, sizeof *val,
            sd, depth, ap);

    va_end(ap);

    if(ret == PAL_YAML_OK)
        *val = val64;
    return ret;
}

pal_yaml_result_t pal_yaml_subdoc_find_uint8(uint8_t *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(val != NULL);

    va_list ap;
    va_start(ap, depth);

    uint64_t val64;
    pal_yaml_result_t ret = find_uint(&val64, sizeof *val,
            sd, depth, ap);

    va_end(ap);

    if(ret == PAL_YAML_OK)
        *val = val64;
    return ret;
}

pal_yaml_result_t pal_yaml_subdoc_find_double(double *valp,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(valp != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    va_list ap;
    va_start(ap, depth);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    va_end(ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        if(node->type != YAML_SCALAR_NODE) {
            pal_yaml_subdoc_error_push(sd,
                    "Expected decimal number but got %s",
                    node_type_strings[node->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        char *endp;
        errno = 0;
        double val = strtod((char *)node->data.scalar.value, &endp);
        if(!errno && !*endp) {
            pal_yaml_subdoc_error_context_clear(sd);
            *valp = val;
            return PAL_YAML_OK;
        } else {
            pal_yaml_subdoc_error_push(sd,
                    "`%s' could not be parsed as a decimal number",
                    (char *)node->data.scalar.value,
                    errno ? strerror(errno)
                          : "Contains non-numeric characters");
            return PAL_YAML_TYPE_ERROR;
        }
    } else {
        return res;
    }
}

pal_yaml_result_t pal_yaml_subdoc_find_bool(bool *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(val != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    va_list ap;
    va_start(ap, depth);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    va_end(ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        if(node->type != YAML_SCALAR_NODE) {
            pal_yaml_subdoc_error_push(sd,
                    "Expected boolean but got %s",
                    node_type_strings[node->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        if(!strcasecmp((const char *)node->data.scalar.value,
                    "true")) {
            pal_yaml_subdoc_error_context_clear(sd);
            *val = true;
            return PAL_YAML_OK;
        } else if(!strcasecmp((const char *)node->data.scalar.value,
                    "false")) {
            pal_yaml_subdoc_error_context_clear(sd);
            *val = false;
            return PAL_YAML_OK;
        } else {
            pal_yaml_subdoc_error_push(sd,
                    "`%s' could not be parsed as a boolean",
                    (char *)node->data.scalar.value);
            return PAL_YAML_TYPE_ERROR;
        }
    } else {
        return res;
    }
}

pal_yaml_result_t pal_yaml_subdoc_find_enum(int *val,
        pal_yaml_enum_schema_t *enums, pal_yaml_subdoc_t *sd,
        size_t depth, ...)
{
    assert(val != NULL);
    assert(enums != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    va_list ap;
    va_start(ap, depth);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    va_end(ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        if(node->type != YAML_SCALAR_NODE) {
            pal_yaml_subdoc_error_push(sd,
                    "Expected enumeration value but got %s",
                    node_type_strings[node->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        for(; enums->name; ++enums) {
            if(!strcmp((const char *)node->data.scalar.value,
                        enums->name)) {
                pal_yaml_subdoc_error_context_clear(sd);
                *val = enums->value;
                return PAL_YAML_OK;
            }
        }

        pal_yaml_subdoc_error_push(sd,
                "No such enumeration value `%s'",
                (char *)node->data.scalar.value);
        return PAL_YAML_TYPE_ERROR;
    } else {
        return res;
    }
}

pal_yaml_result_t pal_yaml_subdoc_find_flags(int *val,
        pal_yaml_enum_schema_t *enums, pal_yaml_subdoc_t *sd,
        size_t depth, ...)
{
    assert(val != NULL);
    assert(enums != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    va_list ap;
    va_start(ap, depth);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    va_end(ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        if(node->type != YAML_SEQUENCE_NODE) {
            pal_yaml_subdoc_error_push(sd,
                    "Expected flags sequence but got %s",
                    node_type_strings[node->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        int val_temp = 0;
        yaml_node_item_t *item;
        for(item = node->data.sequence.items.start;
                item < node->data.sequence.items.top; ++item) {
            assert(item != NULL);

            yaml_node_t *elem = yaml_document_get_node(&sd->doc,
                    *item);
            assert(elem != NULL);

            if(elem->type != YAML_SCALAR_NODE) {
                pal_yaml_subdoc_error_context_push(sd,
                        "In %s at %s:%lu:%lu",
                        node_type_strings[elem->type],
                        elem->start_mark.line,
                        elem->start_mark.column);
                pal_yaml_subdoc_error_push(sd,
                        "Expected flags enumeration value but got "
                        "%s", node_type_strings[elem->type]);
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
                pal_yaml_subdoc_error_push(sd,
                        "No such enumeration value `%s'",
                        (char *)elem->data.scalar.value);
                return PAL_YAML_TYPE_ERROR;
            }
        }

        pal_yaml_subdoc_error_context_clear(sd);
        *val = val_temp;
        return PAL_YAML_OK;
    } else {
        return res;
    }
}

static void subdoc_from_node(pal_yaml_subdoc_t *dst,
        yaml_node_t *node, pal_yaml_subdoc_t *sd)
{
    assert(dst != NULL);
    assert(node != NULL);
    assert(sd != NULL);

    dst->doc = sd->doc;
    dst->doc_ref_count = sd->doc_ref_count;
    ++*dst->doc_ref_count;

    dst->node = node;
    strcpy(dst->filename, sd->filename);
    memset(dst->errors, 0, sizeof dst->errors);
    dst->error_count = 0;
    dst->context = NULL;

    error_stack_append(&dst->context,
            sd->errors[sd->error_count], sd->context);
}

pal_yaml_result_t pal_yaml_subdoc_find_sequence(
        pal_yaml_subdoc_t *seq, size_t *len, pal_yaml_subdoc_t *sd,
        size_t depth, ...)
{
    assert(seq != NULL);
    assert(len != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    va_list ap;
    va_start(ap, depth);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    va_end(ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        if(node->type != YAML_SEQUENCE_NODE) {
            pal_yaml_subdoc_error_push(sd,
                    "Expected sequence but got %s",
                    node_type_strings[node->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        subdoc_from_node(seq, node, sd);
        *len = node->data.sequence.items.top
                - node->data.sequence.items.start;
        pal_yaml_subdoc_error_context_clear(sd);
        return PAL_YAML_OK;
    } else {
        return res;
    }
}

pal_yaml_result_t pal_yaml_subdoc_find_string_sequence(
        char ***strseqp, size_t *lenp, pal_yaml_subdoc_t *sd,
        size_t depth, ...)
{
    assert(strseqp != NULL);
    assert(lenp != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    va_list ap;
    va_start(ap, depth);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    va_end(ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        if(node->type != YAML_SEQUENCE_NODE) {
            pal_yaml_subdoc_error_push(sd,
                    "Expected sequence but got %s",
                    node_type_strings[node->type]);
            return PAL_YAML_TYPE_ERROR;
        }

        size_t len = node->data.sequence.items.top
                - node->data.sequence.items.start;
        char **strseq;
        pal_yaml_subdoc_t seq;

        subdoc_from_node(&seq, node, sd);
        TRY_ERRNO(strseq = calloc(len, sizeof *strseq));

        size_t i;
        for(i = 0; i < len; ++i)
            pal_yaml_subdoc_find_string(&strseq[i], &seq,
                    1, PAL_SEQ_IDX(i));

        if(seq.error_count > 0) {
            size_t i;
            for(i = 0; i < len; ++i)
                free(strseq[i]);
            free(strseq);

            for(i = 0; i < seq.error_count
                    && sd->error_count < ERROR_MAX; ++i)
                error_stack_append(&sd->errors[sd->error_count++],
                        seq.errors[i], NULL);

            return PAL_YAML_TYPE_ERROR;
        }

        pal_yaml_subdoc_close(&seq);
        pal_yaml_subdoc_error_context_clear(sd);
        *lenp = len;
        *strseqp = strseq;
        return PAL_YAML_OK;
    } else {
        return res;
    }
}

pal_yaml_result_t pal_yaml_subdoc_find_subdoc(pal_yaml_subdoc_t *dst,
        pal_yaml_subdoc_t *sd, size_t depth, ...)
{
    assert(dst != NULL);
    assert(sd != NULL);
    assert(sd->node != NULL);

    va_list ap;
    va_start(ap, depth);

    yaml_node_t *node;
    pal_yaml_result_t res = find_node(&node, sd, depth, ap);

    va_end(ap);

    if(res == PAL_YAML_OK) {
        assert(node != NULL);

        subdoc_from_node(dst, node, sd);
        pal_yaml_subdoc_error_context_clear(sd);
        return PAL_YAML_OK;
    } else {
        return res;
    }
}

/* FIXME: Integrate this into individual resource plugins.
 */
static void load_resource_contents(struct rsc_contents *c,
        pal_yaml_subdoc_t *sd)
{
    assert(c != NULL);

    /* pirate_ and fd_channel
     */
    {
        pal_yaml_enum_schema_t channel_type_schema[] = {
            {"device",      DEVICE},
            {"pipe",        PIPE},
            {"unix_socket", UNIX_SOCKET},
            {"tcp_socket",  TCP_SOCKET},
            {"udp_socket",  UDP_SOCKET},
            {"shmem",       SHMEM},
            {"udp_shmem",   UDP_SHMEM},
            {"uio",         UIO_DEVICE},
            {"serial",      SERIAL},
            {"mercury",     MERCURY},
            {"ge_eth",      GE_ETH},
        };
        if(pal_yaml_subdoc_find_enum((int*)&c->cc_channel_type,
                    channel_type_schema, sd,
                    1, PAL_MAP_FIELD("channel_type"))
                == PAL_YAML_NOT_FOUND)
            pal_yaml_subdoc_error_pop(sd);
    }
    if(pal_yaml_subdoc_find_string(&c->cc_path, sd,
                1, PAL_MAP_FIELD("path"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    if(pal_yaml_subdoc_find_unsigned(&c->cc_min_tx_size, sd,
                1, PAL_MAP_FIELD("min_tx_size"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    if(pal_yaml_subdoc_find_unsigned(&c->cc_mtu, sd,
                1, PAL_MAP_FIELD("mtu"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    if(pal_yaml_subdoc_find_unsigned(&c->cc_buffer_size, sd,
                1, PAL_MAP_FIELD("buffer_size"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    if(pal_yaml_subdoc_find_string(&c->cc_host, sd,
                1, PAL_MAP_FIELD("host"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    if(pal_yaml_subdoc_find_unsigned(&c->cc_max_tx_size, sd,
                1, PAL_MAP_FIELD("max_tx_size"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    if(pal_yaml_subdoc_find_unsigned(&c->cc_packet_size, sd,
                1, PAL_MAP_FIELD("packet_size"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    if(pal_yaml_subdoc_find_unsigned(&c->cc_packet_count, sd,
                1, PAL_MAP_FIELD("packet_count"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    if(pal_yaml_subdoc_find_unsigned(&c->cc_region, sd,
                1, PAL_MAP_FIELD("region"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    if(pal_yaml_subdoc_find_unsigned(&c->cc_baud, sd,
                1, PAL_MAP_FIELD("baud"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    if(pal_yaml_subdoc_find_unsigned(&c->cc_message_id, sd,
                1, PAL_MAP_FIELD("message_id"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    {
        pal_yaml_subdoc_t sess;
        if(pal_yaml_subdoc_find_subdoc(&sess, sd,
                    1, PAL_MAP_FIELD("session"))) {
            pal_yaml_subdoc_error_pop(sd);
        } else {
            struct session *s;
            TRY_ERRNO(s = calloc(1, sizeof *c->cc_session));

            if(pal_yaml_subdoc_find_unsigned(&s->sess_level, &sess,
                        1, PAL_MAP_FIELD("level"))
                    == PAL_YAML_NOT_FOUND)
                pal_yaml_subdoc_error_pop(sd);
            if(pal_yaml_subdoc_find_unsigned(&s->sess_src_id, &sess,
                        1, PAL_MAP_FIELD("src_id"))
                    == PAL_YAML_NOT_FOUND)
                pal_yaml_subdoc_error_pop(sd);
            if(pal_yaml_subdoc_find_unsigned(&s->sess_dst_id, &sess,
                        1, PAL_MAP_FIELD("dst_id"))
                    == PAL_YAML_NOT_FOUND)
                pal_yaml_subdoc_error_pop(sd);
            if(pal_yaml_subdoc_find_unsigned_sequence(
                        &s->sess_messages,
                        &s->sess_messages_count, &sess,
                        1, PAL_MAP_FIELD("messages"))
                    == PAL_YAML_NOT_FOUND)
                pal_yaml_subdoc_error_pop(sd);
            if(pal_yaml_subdoc_find_unsigned(
                        &s->sess_id, &sess,
                        1, PAL_MAP_FIELD("sess_id"))
                    == PAL_YAML_NOT_FOUND)
                pal_yaml_subdoc_error_pop(sd);

            c->cc_session = s;
            pal_yaml_subdoc_close(&sess);
        }
    }

    /* Trivial resources
     */
    if(pal_yaml_subdoc_find_string(&c->cc_string_value, sd,
                1, PAL_MAP_FIELD("string_value"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    {
        int64_t int_val;
        if(pal_yaml_subdoc_find_signed(&int_val, sd,
                    1, PAL_MAP_FIELD("integer_value"))
                == PAL_YAML_NOT_FOUND) {
            pal_yaml_subdoc_error_pop(sd);
        } else {
            TRY_ERRNO(c->cc_integer_value
                    = malloc(sizeof *c->cc_integer_value));
            *c->cc_integer_value = int_val;
        }
    }
    {
        bool bool_val;
        if(pal_yaml_subdoc_find_boolean(&bool_val, sd,
                    1, PAL_MAP_FIELD("boolean_value"))
                == PAL_YAML_NOT_FOUND) {
            pal_yaml_subdoc_error_pop(sd);
        } else {
            TRY_ERRNO(c->cc_boolean_value
                    = malloc(sizeof *c->cc_boolean_value));
            *c->cc_boolean_value = bool_val;
        }
    }

    /* File resource
     */
    if(pal_yaml_subdoc_find_string(&c->cc_file_path, sd,
                1, PAL_MAP_FIELD("file_path"))
            == PAL_YAML_NOT_FOUND)
        pal_yaml_subdoc_error_pop(sd);
    {
        pal_yaml_enum_schema_t fflags_schema[] = {
            {"O_RDONLY",    O_RDONLY},
            {"O_WRONLY",    O_WRONLY},
            {"O_RDWR",      O_RDWR},

            {"O_APPEND",    O_APPEND},
            {"O_ASYNC",     O_ASYNC},
            {"O_CLOEXEC",   O_CLOEXEC},
            {"O_CREAT",     O_CREAT},
            {"O_DIRECTORY", O_DIRECTORY},
            {"O_DSYNC",     O_DSYNC},
            {"O_EXCL",      O_EXCL},
            {"O_NOCTTY",    O_NOCTTY},
            {"O_NOFOLLOW",  O_NOFOLLOW},
            {"O_NONBLOCK",  O_NONBLOCK},
            {"O_SYNC",      O_SYNC},
            {"O_TRUNC",     O_TRUNC},
        };
        int fflags;
        if(pal_yaml_subdoc_find_flags(&fflags, fflags_schema, sd,
                    1, PAL_MAP_FIELD("file_flags"))
                == PAL_YAML_NOT_FOUND) {
            pal_yaml_subdoc_error_pop(sd);
        } else {
            TRY_ERRNO(c->cc_file_flags
                    = malloc(sizeof *c->cc_file_flags));
            *c->cc_file_flags = fflags;
        }
    }
}

struct top_level *load_yaml(const char *fname)
{
    assert(fname != NULL);

    struct top_level *tlp = calloc(1, sizeof *tlp);

    pal_yaml_subdoc_t sd;
    if(pal_yaml_subdoc_open(&sd, fname) != PAL_YAML_OK) {
        pal_yaml_subdoc_log_errors(&sd);
        free(tlp);
        return NULL;
    }

    {
        pal_yaml_subdoc_t encs;
        pal_yaml_subdoc_find_sequence(
                &encs, &tlp->tl_encs_count, &sd,
                1, PAL_MAP_FIELD("enclaves"));
        if(tlp->tl_encs_count > 0)
            tlp->tl_encs = calloc(tlp->tl_encs_count,
                    sizeof(*tlp->tl_encs));

        size_t i;
        for(i = 0; i < tlp->tl_encs_count; ++i) {
            struct enclave *e = &tlp->tl_encs[i];

            pal_yaml_subdoc_find_string(&e->enc_name, &encs,
                    2, PAL_SEQ_IDX(i), PAL_MAP_FIELD("name"));
            if(pal_yaml_subdoc_find_string(&e->enc_path, &encs,
                        2, PAL_SEQ_IDX(i), PAL_MAP_FIELD("path"))
                    == PAL_YAML_NOT_FOUND)
                pal_yaml_subdoc_error_pop(&encs);
            if(pal_yaml_subdoc_find_string_sequence(
                        &e->enc_args, &e->enc_args_count, &encs,
                        2, PAL_SEQ_IDX(i), PAL_MAP_FIELD("args"))
                    == PAL_YAML_NOT_FOUND)
                pal_yaml_subdoc_error_pop(&encs);
            if(pal_yaml_subdoc_find_string_sequence(
                        &e->enc_env, &e->enc_env_count, &encs,
                        2, PAL_SEQ_IDX(i), PAL_MAP_FIELD("env"))
                    == PAL_YAML_NOT_FOUND)
                pal_yaml_subdoc_error_pop(&encs);
        }

        pal_yaml_subdoc_close(&encs);
    }

    {
        pal_yaml_subdoc_t rscs;
        pal_yaml_subdoc_find_sequence(
                &rscs, &tlp->tl_rscs_count, &sd,
                1, PAL_MAP_FIELD("resources"));
        if(tlp->tl_rscs_count > 0)
            tlp->tl_rscs = calloc(tlp->tl_rscs_count,
                    sizeof(*tlp->tl_rscs));

        size_t i;
        for(i = 0; i < tlp->tl_rscs_count; ++i) {
            struct resource *r = &tlp->tl_rscs[i];
            pal_yaml_subdoc_find_string(&r->r_name, &rscs,
                    2, PAL_SEQ_IDX(i), PAL_MAP_FIELD("name"));
            pal_yaml_subdoc_find_string(&r->r_type, &rscs,
                    2, PAL_SEQ_IDX(i), PAL_MAP_FIELD("type"));
            pal_yaml_subdoc_find_string_sequence(
                    &r->r_ids, &r->r_ids_count, &rscs,
                    2, PAL_SEQ_IDX(i), PAL_MAP_FIELD("ids"));
            pal_yaml_subdoc_find_subdoc(&r->r_yaml, &rscs,
                    2, PAL_SEQ_IDX(i), PAL_MAP_FIELD("contents"));
        }

        pal_yaml_subdoc_close(&rscs);
    }

    {
        pal_yaml_enum_schema_t log_level_schema[] = {
            { "default", LOGLVL_DEFAULT },
            { "info",    LOGLVL_INFO },
            { "debug",   LOGLVL_DEBUG }
        };
        if(pal_yaml_subdoc_find_enum((int*)&tlp->tl_cfg.cfg_loglvl,
                    log_level_schema, &sd,
                    2, PAL_MAP_FIELD("config"),
                       PAL_MAP_FIELD("log_level"))
                == PAL_YAML_NOT_FOUND)
            pal_yaml_subdoc_error_pop(&sd);
    }

    if(pal_yaml_subdoc_error_count(&sd) > 0) {
        pal_yaml_subdoc_log_errors(&sd);
        free_yaml(tlp);
        tlp = NULL;
    }

    pal_yaml_subdoc_close(&sd);
    return tlp;
}

void free_yaml(struct top_level *tlp)
{
    for(size_t i = 0; i < tlp->tl_encs_count; ++i) {
        struct enclave *e = &tlp->tl_encs[i];
        free(e->enc_name);
        free(e->enc_path);
        for(size_t i = 0; i < e->enc_args_count; ++i)
            free(e->enc_args[i]);
        free(e->enc_args);
        for(size_t i = 0; i < e->enc_env_count; ++i)
            free(e->enc_env[i]);
        free(e->enc_env);
    }
    free(tlp->tl_encs);
    for(size_t i = 0; i < tlp->tl_rscs_count; ++i) {
        struct resource *r = &tlp->tl_rscs[i];
        free(r->r_name);
        free(r->r_type);
        for(size_t i = 0; i < r->r_ids_count; ++i)
            free(r->r_ids[i]);
        free(r->r_ids);
        pal_yaml_subdoc_close(&r->r_yaml);
    }
    free(tlp->tl_rscs);
    free(tlp);
}
