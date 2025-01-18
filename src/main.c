#include "cpuapi.h"
#include "devices.h"
#include "display.h"
#include "drive.h"
#include "pc.h"
#include "platform.h"
#include "util.h"
#include "noSDL.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Halfix entry point

static struct pc_settings pc;
int parse_cfg(struct pc_settings* pc, char* data);

int realtime_option = 0;

struct option {
    const char *alias, *name;
    int flags, id;
    const char* help;
};

#define HASARG 1

enum {
    OPTION_HELP,
    OPTION_CONFIG,
    OPTION_REALTIME
};

static const struct option options[] = {
    { "h", "help", 0, OPTION_HELP, "Show available options" },
    { "c", "config", HASARG, OPTION_CONFIG, "Use custom config file [arg]" },
    { "r", "realtime", 0, OPTION_REALTIME, "Try to sync internal emulator clock with wall clock" },
    { NULL, NULL, 0, 0, NULL }
};

static void generic_help(const struct option* options)
{
    int i = 0;
    printf("Halfix x86 PC Emulator\n");
    for (;;) {
        const struct option* o = options + i++;
        if (!o->name)
            return;

        char line[100];
        int linelength = sprintf(line, " -%s", o->alias);
        if (o->alias)
            linelength += sprintf(line + linelength, " --%s", o->name);

        if (o->flags & HASARG)
            linelength += sprintf(line + linelength, " [arg]");

        while (linelength < 40)
            line[linelength++] = ' ';
        line[linelength] = 0;
        printf("%s%s\n", line, o->help);
    }
}

int main_halfix_unix(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    char* configfile = "default.conf";
    int filesz=0;
    FILE* f;
    char* buf;

    if (argc == 1)
        goto parse_config;
    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        int j = 0;
        for (;;) {
            const struct option* o = options + j++;
            int long_ver = arg[1] == '-'; // XXX what if string is only 1 byte long?

            if (!o->name)
                break;
            if (!strcmp(long_ver ? o->name : o->alias, arg + (long_ver + 1))) {
                char* data;
                if (o->flags & HASARG) {
                    if (!(data = argv[++i])) {
                        fprintf(stderr, "Expected argument to option %s\n", arg);
                        return -1;
                    }
                } else
                    data = NULL;

                switch (o->id) {
                case OPTION_HELP:
                    generic_help(options);
                    return 0;
                case OPTION_CONFIG:
                    configfile = data;
                    continue;
                case OPTION_REALTIME:
                    realtime_option = -1;
                    continue;
                }
                break;
            }
        }
    }

parse_config:
    f = fopen(configfile, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open configuration file %s\n", configfile);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    buf = malloc((filesz = ftell(f)) + 1);
    fseek(f, 0, SEEK_SET);
    if (fread(buf, filesz, 1, f) != 1) {
        perror("fread");
        fprintf(stderr, "Failed to read configuration file\n");
        return -1;
    }

    buf[filesz] = 0;

    fclose(f);

    int result = parse_cfg(&pc, buf);
    free(buf);
    if (result < 0)
        return -1;

    if (pc.memory_size < (1 << 20)) {
        fprintf(stderr, "Memory size (0x%x) too small\n", pc.memory_size);
        return -1;
    }
    if (pc.vga_memory_size < (256 << 10)) {
        fprintf(stderr, "VGA memory size (0x%x) too small\n", pc.vga_memory_size);
        return -1;
    }
    if (pc_init(&pc) == -1) {
        fprintf(stderr, "Unable to initialize PC\n");
        return -1;
    }

    // all ok
    return 0;
}

void mainloop_processor_debug()
{
    // Good for debugging the CPU alone
    while(1) {
        pc_execute(0);
    }
}

void mainloop_single_core()
{
    // Single loop that does everything with some frameskip and adaptive execution
    // good for real-world stuff
    int frames = 10;
    int vgaupd = 0;
    while (1) {

        unsigned b, c, d, e;
        noSDL_wrapStartTimer();
        noSDL_wrapCheckTimerMs();

        int ms_to_sleep = pc_execute(frames);

        b = noSDL_wrapCheckTimerMs();

        if (b > 100 && frames > 0)
            frames--;
        if (b < 100 && frames < 10)
            frames++;

        vgaupd++;
        vgaupd %= 10;

        // Update our screen/devices here
        if (vgaupd == 0)
            vga_update();

        c = noSDL_wrapCheckTimerMs() - (b);

        display_handle_events();
        ms_to_sleep &= realtime_option;

        d = noSDL_wrapCheckTimerMs() - (b + c);

        // ms_to_sleep is always zero here, this updates the USB status
        display_sleep(ms_to_sleep * 5);

        e = noSDL_wrapCheckTimerMs() - (b + c + d);

        // sprintf(deb, "FR: %02d - Exe:%03u - vga:%03u - eve:%03u - slp:%03u  -  Tot:%04u", frames, b, c, d, e, (b + c + d + e));
        // noSDL_wrapScreenLogAt(deb, 20, 740);
    }
}

unsigned time_exe=0, time_rest=0, time_vga=0;
int frames = 10;

// no VGA update
void mainloop_multi_core_zero()
{
    // Multiple loop types, good for real-world stuff
    while (1)
    {
        noSDL_wrapStartTimer();
        noSDL_wrapCheckTimerMs();

        int ms_to_sleep = pc_execute(frames);

        time_exe = noSDL_wrapCheckTimerMs();

        if (time_exe > 100 && frames > 0)
            frames--;
        if (time_exe < 100 && frames < 10)
            frames++;

        display_handle_events();
        ms_to_sleep &= realtime_option;

        // ms_to_sleep is always zero here, this updates the USB status
        display_sleep(ms_to_sleep * 5);

        time_rest = noSDL_wrapCheckTimerMs() - time_exe;
    }
}

static char deb[200]="";

void mainloop_multi_core_two()
{
    while (1)
    {
        sprintf(deb, "FR: %02d - Exe:%03u - Rest:%03u - Tot:%03u", frames, time_exe, time_rest, (time_exe+time_rest));
        noSDL_wrapScreenLogAt(deb, 20, 740);

        sprintf(deb, "VGA:%03u", time_vga);
        noSDL_wrapScreenLogAt(deb, 20, 756);

        SDL_Delay(100);
    }
}

void mainloop_multi_core_one()
{
    // VGA update loop
    while (1)
    {
        noSDL_wrapStartTimer();
        noSDL_wrapCheckTimerMs();

        vga_update();

        time_vga = noSDL_wrapCheckTimerMs();
    }
}
