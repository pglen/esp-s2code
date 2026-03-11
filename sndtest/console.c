
/* =====[ esp_loratest.sess ]========================================================

   File Name:       console.c

   Description:     Functions for console.c

   Revisions:

      REV   DATE                BY              DESCRIPTION
      ----  -----------         ----------      --------------------------
      0.00  Sun 25.Jan.2026     Peter Glen      Initial version.

   ======================================================================= */

typedef struct {
    struct arg_str *arg1;
    struct arg_end *end;
} arg_args_t;

static arg_args_t  stop_args;
int stop_flag = 1;
static int set_stop(int argc, char **argv)

{
    int nerrors = arg_parse(argc, argv, (void **) &stop_args);
    if (nerrors != 0) {
        //arg_print_errors(stderr, verb_args.end, argv[0]);
        }
    stop_flag = ! stop_flag;
    printf("Stop toggled to %d\n", stop_flag);
    return 0;
}

static arg_args_t  help_args;
static int set_help(int argc, char **argv)

{
    int nerrors = arg_parse(argc, argv, (void **) &help_args);
    if (nerrors != 0) {
        //arg_print_errors(stderr, verb_args.end, argv[0]);
        }
    printf("Commands: [v]erbose [sp]ect [h]elp [p]refix [f]ilt\n");
    return 0;
}

static arg_args_t  spect_args;
int show_spect = 0;
static int set_spect(int argc, char **argv)

{
    int nerrors = arg_parse(argc, argv, (void **) &spect_args);
    if (nerrors != 0) {
        //arg_print_errors(stderr, verb_args.end, argv[0]);
        }
    show_spect = ! show_spect;
    printf("Spect display toggled to %d\n", show_spect);
    return 0;
}

static arg_args_t  filt_args;
int show_filt = 0;
static int set_filt(int argc, char **argv)

{
    int nerrors = arg_parse(argc, argv, (void **) &filt_args);
    if (nerrors != 0) {
        //arg_print_errors(stderr, verb_args.end, argv[0]);
        }
    show_filt = ! show_filt;
    printf("Spect filter toggled to %d\n", show_filt);
    return 0;
}

char prefix[32]  = "";
static arg_args_t  prefix_args;

static int set_prefix(int argc, char **argv)

{
    int nerrors = arg_parse(argc, argv, (void **) &prefix_args);

    if (nerrors != 0) {
        //arg_print_errors(stderr, verb_args.end, argv[0]);
        }
    if(strlen(prefix_args.arg1->sval[0]) == 0)
        {
        printf("Current prefix: %s\n", prefix);
        return 0;
        }
    strncpy(prefix, prefix_args.arg1->sval[0], sizeof(prefix));
    printf("Current set to: %s\n", prefix);
    return 0;
}

int verbose  = 0;
static arg_args_t  verb_args;

static int set_verbose(int argc, char **argv)

{
    //printf("verb_args called\n");
    int nerrors = arg_parse(argc, argv, (void **) &verb_args);

    if (nerrors != 0) {
        //arg_print_errors(stderr, verb_args.end, argv[0]);
        }
    if(strlen(verb_args.arg1->sval[0]) == 0)
        {
        printf("Current verbosity level: %d\n", verbose);
        return 0;
        }
    //printf("arg '%s'\n", verb_args.verb->sval[0]);
    int vvv = atoi(verb_args.arg1->sval[0]);
    if(vvv <  0)
        vvv = 0;
    if(vvv > 10)
        vvv = 10;
    verbose = vvv;
    printf("Verbose set to %d\n", verbose);
    verb_args.arg1->sval[0] = "";

    return 0;
}

#define INIT_STRUCT(sss)                                    \
    sss.arg1 = arg_str1(NULL, NULL, "", "");                \
    sss.end  = arg_end(1);

#define DECL_COMMANDx(namex, helpx, funcx, argx)            \
    {                                                       \
    const esp_console_cmd_t varx =  {                       \
        .command = namex,                                   \
        .help = helpx,                                      \
        .hint = NULL,                                       \
        .func = funcx,                                      \
        .argtable = argx,                                   \
        };                                                  \
    ESP_ERROR_CHECK( esp_console_cmd_register(& varx));     \
    }

void register_cmds(void)

{
    char *verb = "Set verbosity";
    INIT_STRUCT(verb_args)
    DECL_COMMANDx("verb",       verb, &set_verbose, &verb_args);
    DECL_COMMANDx("verbose",    verb, &set_verbose, &verb_args);
    DECL_COMMANDx("v",          verb, &set_verbose, &verb_args);

    INIT_STRUCT(prefix_args)
    DECL_COMMANDx("prefix",     "Set prefix",  &set_prefix, &prefix_args);
    DECL_COMMANDx("p",          "Set prefix",  &set_prefix, &prefix_args);

    INIT_STRUCT(stop_args)
    DECL_COMMANDx("stop",       "Stop output",  &set_stop, &stop_args);
    DECL_COMMANDx("start",      "Start output",  &set_stop, &stop_args);
    DECL_COMMANDx("s",          "Stop output",  &set_stop, &stop_args);

    INIT_STRUCT(spect_args)
    DECL_COMMANDx("spect",      "Show spect",  &set_spect, &spect_args);
    DECL_COMMANDx("sp",         "Set prefix",  &set_spect, &spect_args);

    INIT_STRUCT(filt_args)
    DECL_COMMANDx("filt",      "Show filt",   &set_filt, &filt_args);
    DECL_COMMANDx("f",         "Show filt",  &set_filt, &filt_args);

    INIT_STRUCT(help_args)
    DECL_COMMANDx("help",      "Show help",   &set_help, &help_args);
    DECL_COMMANDx("?",         "Show help",  &set_help, &help_args);
    DECL_COMMANDx("h",         "Show help",  &set_help, &help_args);
}

// -----------------------------------------------------------------------

void    start_console(char *prompt)

{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config =
                                ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config =
                                ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    repl_config.prompt = prompt;

    // init console REPL environment
    ESP_ERROR_CHECK(esp_console_new_repl_uart(
                                &uart_config, &repl_config, &repl));

    linenoiseSetDumbMode(1);
    //linenoiseSetDumbMode(0);

    /* Register commands */
    //register_system();
    register_cmds();

    vTaskDelay(pdMS_TO_TICKS(100));
    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

// EOF
