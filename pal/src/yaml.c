#include <assert.h>
#include <errno.h>
#include <string.h>
#include <yaml.h>

#include "yaml.h"

#define TRY(_op) do{ \
    if(!(_op)) \
        fatal("%s: %s", #_op, strerror(errno)); \
}while(0)

#define LIBYAML_NOTOK (0)
#define LIBYAML_OK    (1)

/* YAML parser state
 */

enum state_type {
    START,
    IN_STREAM,
    IN_DOCUMENT,
    IN_MAPPING,
    IN_SEQUENCE,
    IN_KEY,
    ERROR,
};

struct state {
    enum state_type type;
    yaml_mark_t start_pos;
    yaml_mark_t end_pos;
    char *key;

    union {
        struct pal_yaml_value *document;

        struct {
            struct pal_yaml_mapping *head;
            struct pal_yaml_mapping *tail;
        } mapping;

        struct {
            struct pal_yaml_sequence *head;
            struct pal_yaml_sequence *tail;
        } sequence;

        struct {
            char *problem;
            yaml_mark_t problem_pos;

            char *context;
            yaml_mark_t context_pos;
        } error;
    } data;

    struct state *next;
};

#define START_STATE { \
    .type = START, \
    .start_pos = {0,0,0}, \
    .end_pos = {0,0,0}, \
    .key = NULL, \
    .data = { .document = NULL }, \
    .next = NULL, \
}

static void log_stack_trace(struct state *state)
{
    while(state && state->type != START) {
        char pos[64];
        snprintf(pos, sizeof pos, "%lu:%lu-%lu:%lu",
                state->start_pos.line, state->start_pos.column,
                state->end_pos.line, state->end_pos.column);

        switch(state->type) {
            case START: /* fallthru */
            case IN_STREAM:
                break;
            case IN_DOCUMENT:
                error("In document at %s", pos);
                break;
            case IN_MAPPING:
                error("In mapping %s at %s",
                        state->key ? state->key : "\b", pos);
                break;
            case IN_SEQUENCE:
                error("In sequence %s at %s",
                        state->key ? state->key : "\b", pos);
                break;
            case IN_KEY:
                error("In key %s at %s", state->key, pos);
                break;
            case ERROR:
                snprintf(pos, sizeof pos, "%lu:%lu",
                        state->data.error.problem_pos.line,
                        state->data.error.problem_pos.column);
                error("Error at %s: %s", pos, state->data.error.problem);
                snprintf(pos, sizeof pos, "%lu:%lu",
                        state->data.error.context_pos.line,
                        state->data.error.context_pos.column);
                error("In context at %s: %s", pos, state->data.error.problem);
                break;
        }

        state = state->next;
    }
}

#define ADD_TO_LL(_dst, _src) do{ \
    assert((_src) != NULL); \
    (_src)->next = NULL; \
    if((_dst).tail) { \
        assert((_dst).head != NULL); \
        (_dst).tail->next = (_src); \
        (_dst).tail = (_src); \
    } else { \
        assert((_dst).head == NULL); \
        (_dst).head = (_dst).tail = (_src); \
    } \
}while(0);

static void add_mapping_to_state(struct state *state,
        char *key, struct pal_yaml_mapping *map)
{
    assert(state != NULL);

    switch(state->type) {
        case IN_DOCUMENT:
            assert(key == NULL);
            assert(state->data.document == NULL);

            struct pal_yaml_value *new_val;
            TRY(new_val = malloc(sizeof *new_val));

            new_val->type = MAPPING;
            new_val->data.mapping = map;

            state->data.document = new_val;
            break;
        case IN_MAPPING:
            assert(key != NULL);

            struct pal_yaml_mapping *new_map;
            TRY(new_map = malloc(sizeof *new_map));

            new_map->key = key;
            new_map->value.type = MAPPING;
            new_map->value.data.mapping = map;

            ADD_TO_LL(state->data.mapping, new_map);
            break;
        case IN_SEQUENCE:
            assert(key == NULL);

            struct pal_yaml_sequence *new_seq;
            TRY(new_seq = malloc(sizeof *new_seq));

            new_seq->value.type = MAPPING;
            new_seq->value.data.mapping = map;

            ADD_TO_LL(state->data.sequence, new_seq);
            break;
        default:
            fatal("BUG: Adding mapping to state of invalid type: %d",
                    state->type);
    }
}

static void add_sequence_to_state(struct state *state,
        char *key, struct pal_yaml_sequence *seq)
{
    assert(state != NULL);

    switch(state->type) {
        case IN_DOCUMENT:
            assert(key == NULL);
            assert(state->data.document == NULL);

            struct pal_yaml_value *new_val;
            TRY(new_val = malloc(sizeof *new_val));

            new_val->type = SEQUENCE;
            new_val->data.sequence = seq;

            state->data.document = new_val;
            break;
        case IN_MAPPING:
            assert(key != NULL);

            struct pal_yaml_mapping *new_map;
            TRY(new_map = malloc(sizeof *new_map));

            new_map->key = key;
            new_map->value.type = SEQUENCE;
            new_map->value.data.sequence = seq;

            ADD_TO_LL(state->data.mapping, new_map);
            break;
        case IN_SEQUENCE:
            assert(key == NULL);

            struct pal_yaml_sequence *new_seq;
            TRY(new_seq = malloc(sizeof *new_seq));

            new_seq->value.type = SEQUENCE;
            new_seq->value.data.sequence = seq;

            ADD_TO_LL(state->data.sequence, new_seq);
            break;
        default:
            fatal("BUG: Adding sequence to state of invalid type: %d",
                    state->type);
    }
}

static void add_scalar_to_state(struct state *state, char *key,
        const unsigned char *scalar)
{
    assert(state != NULL);

    char *new_scalar;
    TRY(new_scalar = strdup((const char *)scalar));

    switch(state->type) {
        case IN_MAPPING:
            assert(key != NULL);

            struct pal_yaml_mapping *new_map;
            TRY(new_map = malloc(sizeof *new_map));

            new_map->key = key;
            new_map->value.type = SCALAR;
            new_map->value.data.scalar = new_scalar;

            ADD_TO_LL(state->data.mapping, new_map);
            break;
        case IN_SEQUENCE:
            assert(key == NULL);

            struct pal_yaml_sequence *new_seq;
            TRY(new_seq = malloc(sizeof *new_seq));

            new_seq->value.type = SCALAR;
            new_seq->value.data.scalar = new_scalar;

            ADD_TO_LL(state->data.sequence, new_seq);
            break;
        default:
            fatal("BUG: Adding scalar to state of invalid type: %d",
                    state->type);
    }
}

static struct state *push_event(struct state *cur_state,
        const yaml_event_t *event)
{
    assert(cur_state != NULL);
    assert(event != NULL);

    struct state *new_state;

    if(cur_state->type == IN_KEY) {
        assert(event->type == YAML_MAPPING_START_EVENT
                || event->type == YAML_SEQUENCE_START_EVENT
                || event->type == YAML_ALIAS_EVENT);
        new_state = cur_state;
        cur_state = cur_state->next;
        assert(cur_state->type == IN_MAPPING);
    } else {
        TRY(new_state = malloc(sizeof *new_state));
        new_state->next = cur_state;
    }

    new_state->start_pos = event->start_mark;
    new_state->end_pos = event->end_mark;

    switch(event->type) {
        case YAML_STREAM_START_EVENT:
            new_state->type = IN_STREAM;
            new_state->data.document = NULL;
            break;
        case YAML_DOCUMENT_START_EVENT:
            new_state->type = IN_DOCUMENT;
            new_state->data.document = NULL;
            break;
        case YAML_MAPPING_START_EVENT:
            new_state->type = IN_MAPPING;
            new_state->data.mapping.head = NULL;
            new_state->data.mapping.tail = NULL;
            break;
        case YAML_SEQUENCE_START_EVENT:
            new_state->type = IN_SEQUENCE;
            new_state->data.sequence.head = NULL;
            new_state->data.sequence.tail = NULL;
            break;
        default:
            fatal("BUG: Got a start event (%d) in invalid state %d",
                    event->type, cur_state->type);
    }

    return new_state;
}

static struct state *push_error(struct state *last_state,
        const yaml_parser_t *parser)
{
    assert(last_state != NULL);
    assert(parser != NULL);

    struct state *new_state;

    TRY(new_state = malloc(sizeof *new_state));

    new_state->type = ERROR;
    TRY(new_state->data.error.problem = strdup(parser->problem));
    new_state->data.error.problem_pos = parser->problem_mark;
    TRY(new_state->data.error.context = strdup(parser->context));
    new_state->data.error.context_pos = parser->context_mark;
    new_state->next = last_state;

    return new_state;
}

static struct state *pop_event(struct state *cur_state,
        const yaml_event_t *event)
{
    assert(cur_state != NULL);
    assert(event != NULL);

    struct state *new_state = cur_state->next;
    assert(new_state != NULL);

    switch(event->type) {
        case YAML_STREAM_END_EVENT:
            assert(cur_state->type == IN_STREAM);
            assert(new_state->type == START);
            assert(new_state->data.document == NULL);
            new_state->data.document = cur_state->data.document;
            break;
        case YAML_DOCUMENT_END_EVENT:
            assert(cur_state->type == IN_DOCUMENT);
            assert(new_state->type == IN_STREAM);
            if(new_state->data.document)
                fatal("Multiple documents in one stream are not supported");
            new_state->data.document = cur_state->data.document;
            break;
        case YAML_MAPPING_END_EVENT:
            assert(cur_state->type == IN_MAPPING);
            add_mapping_to_state(new_state, cur_state->key,
                    cur_state->data.mapping.head);
            break;
        case YAML_SEQUENCE_END_EVENT:
            assert(cur_state->type == IN_SEQUENCE);
            add_sequence_to_state(new_state, cur_state->key,
                    cur_state->data.sequence.head);
            break;
        default:
            fatal("BUG: Got an end event (%d) in invalid state %d",
                    event->type, cur_state->type);
    }

    free(cur_state);
    return new_state;
}

static struct state *handle_scalar(struct state *cur_state,
        const yaml_event_t *event)
{
    assert(cur_state != NULL);
    assert(event->data.scalar.value != NULL);

    char *key = NULL;
    struct state *temp;

    switch(cur_state->type) {
        case IN_KEY:
            key = cur_state->key;

            temp = cur_state;
            cur_state = cur_state->next;
            free(temp);

            assert(cur_state != NULL);
            assert(cur_state->type == IN_MAPPING);
            /* fallthru */
        case IN_SEQUENCE:
            add_scalar_to_state(cur_state, key, event->data.scalar.value);
            break;
        case IN_MAPPING:
            temp = cur_state;
            TRY(cur_state = malloc(sizeof *cur_state));
            cur_state->next = temp;

            cur_state->type = IN_KEY;
            TRY(cur_state->key = strdup((const char *)event->data.scalar.value));
            cur_state->start_pos = event->start_mark;
            cur_state->end_pos = event->end_mark;
            break;
        default:
            fatal("BUG: Got a scalar event in invalid state %d",
                    cur_state->type);
    }

    return cur_state;
}

static void free_pal_yaml_value(struct pal_yaml_value *valp);

static void free_pal_yaml_mapping(struct pal_yaml_mapping *mapp)
{
    while(mapp) {
        free(mapp->key);
        free_pal_yaml_value(&mapp->value);

        struct pal_yaml_mapping *temp = mapp;
        mapp = mapp->next;
        free(temp);
    }
}

static void free_pal_yaml_sequence(struct pal_yaml_sequence *seqp)
{
    while(seqp) {
        free_pal_yaml_value(&seqp->value);

        struct pal_yaml_sequence *temp = seqp;
        seqp = seqp->next;
        free(temp);
    }
}

static void free_pal_yaml_value(struct pal_yaml_value *valp)
{
    if(valp) {
        switch(valp->type) {
            case MAPPING:
                free_pal_yaml_mapping(valp->data.mapping);
                break;
            case SEQUENCE:
                free_pal_yaml_sequence(valp->data.sequence);
                break;
            case SCALAR:
                free(valp->data.scalar);
                break;
            case NIL:
                break;
        }
    }
}

static void free_state(struct state *state)
{
    while(state && state->type != START) {
        if(state->key)
            free(state->key);

        switch(state->type) {
            case START:
                if(state->data.document)
                    free_pal_yaml_value(state->data.document);
                free(state->data.document);
                break;
            case IN_STREAM:
                if(state->data.document)
                    free_pal_yaml_value(state->data.document);
                free(state->data.document);
                break;
            case IN_DOCUMENT:
                if(state->data.document)
                    free_pal_yaml_value(state->data.document);
                free(state->data.document);
                break;
            case IN_MAPPING:
                if(state->data.mapping.head)
                    free_pal_yaml_mapping(state->data.mapping.head);
                break;
            case IN_SEQUENCE:
                if(state->data.sequence.head)
                    free_pal_yaml_sequence(state->data.sequence.head);
                break;
            default:
                break;
        }

        struct state *temp = state;
        state = state->next;
        free(temp);
    }
}

static struct pal_yaml_value *load_yaml_doc(const char *fname)
{
    assert(fname != NULL);

    FILE *fp;
    yaml_parser_t parser;

    if(yaml_parser_initialize(&parser) != LIBYAML_OK)
        fatal("Unable to initialize libyaml parser");

    if(!(fp = fopen(fname, "rb")))
        fatal("Unable to open %s: %s\n", fname, strerror(errno));

    yaml_parser_set_input_file(&parser, fp);

    bool done = false;
    struct state start = START_STATE, *state = &start;
    do {
        yaml_event_t event;

        if(yaml_parser_parse(&parser, &event) != LIBYAML_OK) {
            push_error(state, &parser);
            log_stack_trace(state);
            free_state(state);
            return NULL;
        }

        switch(event.type) {
            case YAML_STREAM_START_EVENT:
                plog(LOGLVL_DEBUG, "STREAM-START");
                state = push_event(state, &event);
                break;
            case YAML_STREAM_END_EVENT:
                plog(LOGLVL_DEBUG, "STREAM-END");
                state = pop_event(state, &event);
                assert(state->type == START);
                done = true;
                break;
            case YAML_DOCUMENT_START_EVENT:
                plog(LOGLVL_DEBUG, "DOCUMENT-START");
                state = push_event(state, &event);
                break;
            case YAML_DOCUMENT_END_EVENT:
                plog(LOGLVL_DEBUG, "DOCUMENT-END");
                state = pop_event(state, &event);
                break;
            case YAML_ALIAS_EVENT:
                plog(LOGLVL_DEBUG, "ALIAS: %s", event.data.alias.anchor);
                fatal("YAML aliases not supported");
                // TODO: Handle these
                break;
            case YAML_SCALAR_EVENT:
                plog(LOGLVL_DEBUG, "SCALAR: %s", event.data.scalar.value);
                state = handle_scalar(state, &event);
                break;
            case YAML_SEQUENCE_START_EVENT:
                plog(LOGLVL_DEBUG, "SEQUENCE-START");
                state = push_event(state, &event);
                break;
            case YAML_SEQUENCE_END_EVENT:
                plog(LOGLVL_DEBUG, "SEQUENCE-END");
                state = pop_event(state, &event);
                break;
            case YAML_MAPPING_START_EVENT:
                plog(LOGLVL_DEBUG, "MAPPING-START");
                state = push_event(state, &event);
                break;
            case YAML_MAPPING_END_EVENT:
                plog(LOGLVL_DEBUG, "MAPPING-END");
                state = pop_event(state, &event);
                break;
            default:
                plog(LOGLVL_DEBUG, "EMPTY-EVENT");
                // FIXME: Does this indicate the NIL value?
        }

        yaml_event_delete(&event);
    } while(!done);

    fclose(fp);

    return state->data.document;
}

struct top_level *load_yaml(const char *fname)
{
    struct top_level *tlp = NULL;

    struct pal_yaml_value __attribute__((unused)) *docp = load_yaml_doc(fname);

    return tlp;
}

void free_yaml(struct top_level *tlp)
{
    // TODO: Implement this
}
