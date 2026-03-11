# LORA Test ESP32-S2
## for the SX1276 chip

This project tests the recv/transmit capability of the LORA chip.
The terminal command line can be used to configure both the
transmit/receive side.

    DECL_COMMANDx("v",          "Set verbosity", &set_verbose, &verb_args);
    DECL_COMMANDx("r",          "reboot / restart device",  &reboot_dev, NULL);
    DECL_COMMANDx("help",       "Help",  &help, NULL);
    DECL_COMMANDx("h",          "Help",  &help, NULL);
    DECL_COMMANDx("?",          "Help",  &help, NULL);
    DECL_COMMANDx("sf",         "Set spread", &set_spread, &spread_args);
    DECL_COMMANDx("s",          "Set spread", &set_spread, &spread_args);
    DECL_COMMANDx("bw",         "Set bandwidth", &set_bw, &bw_args);
    DECL_COMMANDx("b",          "Set bandwidth", &set_bw, &bw_args);
    DECL_COMMANDx("fr",         "Set frequency", &set_fr, &fr_args);
    DECL_COMMANDx("f",          "", &set_fr, &fr_args);
    DECL_COMMANDx("de",         "reset defaults", &set_de, &de_args);
    DECL_COMMANDx("d",          "", &set_fr, &fr_args);
    DECL_COMMANDx("t",         "transmit", &set_str, &str_args);
    DECL_COMMANDx("tr",         "transmit", &set_str, &str_args);

## Command help (brief)

     v (verbose) [1-10] h (help) r (reboot) s (sf) (spread) [2-12]\n");
     b (bw) bandwidth [1-10] f (fr) frequency [410-500]\n");
     d (de) defaults [f=433.175MHz sf=12 bw=30k] \n");
     t (tr) transmit [hello ] \n");
        printf("use: command ? for help on command");

