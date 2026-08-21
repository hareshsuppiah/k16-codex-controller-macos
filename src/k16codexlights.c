#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>

#include <errno.h>
#include <glob.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifndef K16_VENDOR_ID
#define K16_VENDOR_ID 13998
#endif

#ifndef K16_PRODUCT_ID
#define K16_PRODUCT_ID 9333
#endif

#ifndef K16_USAGE_PAGE
#define K16_USAGE_PAGE 0xff00
#endif

#ifndef K16_USAGE
#define K16_USAGE 2
#endif

enum { K16_REPORT_BYTES = 64 };

typedef enum {
    STATUS_IDLE,
    STATUS_THINKING,
    STATUS_COMPLETE,
    STATUS_NEEDS_INPUT,
    STATUS_ERROR,
} CodexStatus;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGB;

typedef struct {
    uint8_t hue;
    uint8_t saturation;
    uint8_t value;
} HSV;

static volatile sig_atomic_t keep_running = 1;

static void diagnostic(const char *format, ...) {
    FILE *log = fopen("/tmp/k16-codex-lights.log", "a");
    if (!log) {
        return;
    }

    time_t now = time(NULL);
    struct tm local_time;
    localtime_r(&now, &local_time);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time);
    fprintf(log, "%s ", timestamp);

    va_list arguments;
    va_start(arguments, format);
    vfprintf(log, format, arguments);
    va_end(arguments);
    fputc('\n', log);
    fclose(log);
}

static void handle_signal(int signal_number) {
    (void)signal_number;
    keep_running = 0;
}

static const char *status_name(CodexStatus status) {
    switch (status) {
        case STATUS_IDLE: return "idle";
        case STATUS_THINKING: return "thinking";
        case STATUS_COMPLETE: return "complete";
        case STATUS_NEEDS_INPUT: return "needs_input";
        case STATUS_ERROR: return "error";
    }
    return "unknown";
}

static RGB status_color(CodexStatus status) {
    switch (status) {
        case STATUS_IDLE: return (RGB){0xe8, 0xee, 0xf7};
        case STATUS_THINKING: return (RGB){0x4a, 0x90, 0xff};
        case STATUS_COMPLETE: return (RGB){0x54, 0xd6, 0x8c};
        case STATUS_NEEDS_INPUT: return (RGB){0xff, 0xd3, 0x4e};
        case STATUS_ERROR: return (RGB){0xff, 0x5c, 0x8a};
    }
    return (RGB){0xe8, 0xee, 0xf7};
}

static HSV rgb_to_hsv(RGB color) {
    int red = color.red;
    int green = color.green;
    int blue = color.blue;
    int maximum = red;
    if (green > maximum) maximum = green;
    if (blue > maximum) maximum = blue;
    int minimum = red;
    if (green < minimum) minimum = green;
    if (blue < minimum) minimum = blue;
    int delta = maximum - minimum;

    int hue = 0;
    if (delta != 0) {
        if (maximum == red) {
            hue = 43 * (green - blue) / delta;
        } else if (maximum == green) {
            hue = 85 + 43 * (blue - red) / delta;
        } else {
            hue = 171 + 43 * (red - green) / delta;
        }
        while (hue < 0) hue += 256;
        while (hue > 255) hue -= 256;
    }

    int saturation = maximum == 0 ? 0 : (delta * 255) / maximum;
    return (HSV){(uint8_t)hue, (uint8_t)saturation, (uint8_t)maximum};
}

static void dictionary_set_int(CFMutableDictionaryRef dictionary, CFStringRef key, int value) {
    CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &value);
    CFDictionarySetValue(dictionary, key, number);
    CFRelease(number);
}

static int property_int(IOHIDDeviceRef device, CFStringRef key) {
    CFTypeRef property = IOHIDDeviceGetProperty(device, key);
    int value = 0;
    if (property && CFGetTypeID(property) == CFNumberGetTypeID()) {
        CFNumberGetValue((CFNumberRef)property, kCFNumberIntType, &value);
    }
    return value;
}

static IOHIDDeviceRef find_k16(void) {
    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager) {
        return NULL;
    }

    CFMutableDictionaryRef match = CFDictionaryCreateMutable(
        kCFAllocatorDefault,
        0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    dictionary_set_int(match, CFSTR(kIOHIDVendorIDKey), K16_VENDOR_ID);
    dictionary_set_int(match, CFSTR(kIOHIDProductIDKey), K16_PRODUCT_ID);
    // Match only the vendor-defined RGB interface. Matching the whole physical
    // keypad also includes its keyboard interfaces, which Karabiner legitimately
    // owns and causes IOHIDManagerOpen to return kIOReturnExclusiveAccess.
    dictionary_set_int(match, CFSTR(kIOHIDPrimaryUsagePageKey), K16_USAGE_PAGE);
    dictionary_set_int(match, CFSTR(kIOHIDPrimaryUsageKey), K16_USAGE);
    IOHIDManagerSetDeviceMatching(manager, match);
    CFRelease(match);

    IOReturn opened = IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
    if (opened != kIOReturnSuccess) {
        fprintf(stderr, "K16 Codex Lights: HID access failed (0x%08x). Grant Input Monitoring permission.\n", opened);
        diagnostic("IOHIDManagerOpen failed: 0x%08x; access=%d", opened, IOHIDCheckAccess(kIOHIDRequestTypeListenEvent));
        CFRelease(manager);
        return NULL;
    }

    CFSetRef devices = IOHIDManagerCopyDevices(manager);
    IOHIDDeviceRef selected = NULL;
    if (devices) {
        CFIndex count = CFSetGetCount(devices);
        const void **values = calloc((size_t)count, sizeof(void *));
        if (values) {
            CFSetGetValues(devices, values);
            for (CFIndex index = 0; index < count; index++) {
                IOHIDDeviceRef candidate = (IOHIDDeviceRef)values[index];
                int page = property_int(candidate, CFSTR(kIOHIDPrimaryUsagePageKey));
                int usage = property_int(candidate, CFSTR(kIOHIDPrimaryUsageKey));
                if (page == K16_USAGE_PAGE && usage == K16_USAGE) {
                    selected = candidate;
                    CFRetain(selected);
                    break;
                }
            }
            free(values);
        }
        CFRelease(devices);
    }

    IOHIDManagerClose(manager, kIOHIDOptionsTypeNone);
    CFRelease(manager);
    if (!selected) {
        diagnostic("HID manager opened, but the K16 vendor interface was not found");
    }
    return selected;
}

static bool set_all_keys(RGB color) {
    IOHIDDeviceRef device = find_k16();
    if (!device) {
        return false;
    }

    IOReturn opened = IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
    if (opened != kIOReturnSuccess) {
        fprintf(stderr, "K16 Codex Lights: unable to open keypad interface (0x%08x).\n", opened);
        diagnostic("IOHIDDeviceOpen failed: 0x%08x", opened);
        CFRelease(device);
        return false;
    }

    // The vendor configurator first selects the lighting effect with command
    // 22, then sends its complete configuration with command 11.  Using the
    // firmware's supported monochrome Static mode is sufficient because each
    // Codex state intentionally colours the entire keypad.
    uint8_t effect_report[K16_REPORT_BYTES] = {0};
    effect_report[0] = 6;
    effect_report[1] = 22;
    effect_report[5] = 1;  // lighting type
    effect_report[7] = 1;  // static mode

    IOReturn result = IOHIDDeviceSetReport(
        device,
        kIOHIDReportTypeOutput,
        0,
        effect_report,
        sizeof(effect_report)
    );
    if (result != kIOReturnSuccess) {
        fprintf(stderr, "K16 Codex Lights: keypad rejected static-effect report (0x%08x).\n", result);
        diagnostic("Static lighting effect report failed: 0x%08x", result);
        IOHIDDeviceClose(device, kIOHIDOptionsTypeNone);
        CFRelease(device);
        return false;
    }
    usleep(20000);

    HSV hsv = rgb_to_hsv(color);
    uint8_t mode_report[K16_REPORT_BYTES] = {0};
    mode_report[0] = 6;
    mode_report[1] = 11;
    mode_report[2] = 11;
    mode_report[5] = 1;  // lighting type
    mode_report[7] = 1;  // static mode
    mode_report[8] = 4;  // maximum brightness
    mode_report[9] = 3;  // vendor default speed (unused by static mode)
    mode_report[10] = 1; // direction (unused by static mode)
    mode_report[11] = 1; // monochrome enabled
    mode_report[13] = hsv.hue;
    mode_report[14] = hsv.saturation;
    mode_report[15] = hsv.value;

    result = IOHIDDeviceSetReport(
        device,
        kIOHIDReportTypeOutput,
        0,
        mode_report,
        sizeof(mode_report)
    );
    if (result != kIOReturnSuccess) {
        fprintf(stderr, "K16 Codex Lights: keypad rejected static-colour report (0x%08x).\n", result);
        diagnostic("Static lighting colour report failed: 0x%08x", result);
        IOHIDDeviceClose(device, kIOHIDOptionsTypeNone);
        CFRelease(device);
        return false;
    }
    IOHIDDeviceClose(device, kIOHIDOptionsTypeNone);
    CFRelease(device);

    diagnostic("Static RGB report accepted: %02x%02x%02x (HSV %u,%u,%u)",
               color.red, color.green, color.blue,
               hsv.hue, hsv.saturation, hsv.value);
    return true;
}

static bool contains_case_insensitive(const char *text, const char *needle) {
    return text && needle && strcasestr(text, needle) != NULL;
}

static bool completion_needs_input(const char *line) {
    static const char *signals[] = {
        "please ",
        "reply ",
        "tell me when",
        "let me know when",
        "need you to",
        "waiting for you",
        "confirm ",
        "approve ",
        "when you are done",
        "when you're done",
    };
    for (size_t index = 0; index < sizeof(signals) / sizeof(signals[0]); index++) {
        if (contains_case_insensitive(line, signals[index])) {
            return true;
        }
    }
    return false;
}

static bool update_status_from_line(const char *line, CodexStatus *status, bool *task_active) {
    if (strstr(line, "\"type\":\"task_started\"")) {
        *task_active = true;
        *status = STATUS_THINKING;
        return true;
    }
    if (strstr(line, "\"type\":\"task_complete\"")) {
        *task_active = false;
        if (strstr(line, "\"error\":{")) {
            *status = STATUS_ERROR;
        } else if (completion_needs_input(line)) {
            *status = STATUS_NEEDS_INPUT;
        } else {
            *status = STATUS_COMPLETE;
        }
        return true;
    }
    if (strstr(line, "\"type\":\"turn_aborted\"")) {
        *task_active = false;
        *status = STATUS_ERROR;
        return true;
    }
    bool actual_user_input_tool = strstr(line, "\"name\":\"request_user_input\"") != NULL;
    if (strstr(line, "\"type\":\"custom_tool_call\"") &&
        actual_user_input_tool) {
        *status = STATUS_NEEDS_INPUT;
        return true;
    }
    if (*task_active &&
        (strstr(line, "\"type\":\"custom_tool_call_output\"") ||
         strstr(line, "\"type\":\"agent_reasoning\""))) {
        *status = STATUS_THINKING;
        return true;
    }
    return false;
}

static bool newest_session_file(char output[PATH_MAX], struct timespec *modified) {
    const char *home = getenv("HOME");
    if (!home) {
        return false;
    }

    char pattern[PATH_MAX];
    if (snprintf(pattern, sizeof(pattern), "%s/.codex/sessions/*/*/*/*.jsonl", home) >= (int)sizeof(pattern)) {
        return false;
    }

    glob_t matches = {0};
    if (glob(pattern, 0, NULL, &matches) != 0) {
        globfree(&matches);
        return false;
    }

    bool found = false;
    struct timespec newest = {0, 0};
    for (size_t index = 0; index < matches.gl_pathc; index++) {
        struct stat info;
        if (stat(matches.gl_pathv[index], &info) != 0) {
            continue;
        }
        if (!found || info.st_mtimespec.tv_sec > newest.tv_sec ||
            (info.st_mtimespec.tv_sec == newest.tv_sec && info.st_mtimespec.tv_nsec > newest.tv_nsec)) {
            strlcpy(output, matches.gl_pathv[index], PATH_MAX);
            newest = info.st_mtimespec;
            found = true;
        }
    }
    globfree(&matches);
    if (found && modified) {
        *modified = newest;
    }
    return found;
}

static void read_new_lines(
    const char *path,
    off_t *offset,
    CodexStatus *status,
    bool *task_active,
    bool *status_changed
) {
    FILE *file = fopen(path, "r");
    if (!file) {
        return;
    }

    if (*offset == 0) {
        struct stat info;
        if (fstat(fileno(file), &info) == 0 && info.st_size > 4 * 1024 * 1024) {
            *offset = info.st_size - 4 * 1024 * 1024;
            fseeko(file, *offset, SEEK_SET);
            char *discard = NULL;
            size_t discard_size = 0;
            getline(&discard, &discard_size, file);
            free(discard);
        }
    } else {
        fseeko(file, *offset, SEEK_SET);
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length = 0;
    while ((length = getline(&line, &capacity, file)) >= 0) {
        (void)length;
        CodexStatus previous = *status;
        if (update_status_from_line(line, status, task_active) && previous != *status) {
            *status_changed = true;
        }
    }
    free(line);
    *offset = ftello(file);
    fclose(file);
}

static bool parse_status_name(const char *name, CodexStatus *status) {
    for (int candidate = STATUS_IDLE; candidate <= STATUS_ERROR; candidate++) {
        if (strcmp(name, status_name((CodexStatus)candidate)) == 0) {
            *status = (CodexStatus)candidate;
            return true;
        }
    }
    return false;
}

static int replay_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Unable to open replay file: %s\n", strerror(errno));
        return 2;
    }
    CodexStatus status = STATUS_IDLE;
    bool task_active = false;
    char *line = NULL;
    size_t capacity = 0;
    while (getline(&line, &capacity, file) >= 0) {
        CodexStatus previous = status;
        if (update_status_from_line(line, &status, &task_active) && previous != status) {
            fprintf(stdout, "%s\n", status_name(status));
        }
    }
    free(line);
    fclose(file);
    return 0;
}

static int run_monitor(bool dry_run) {
    char current_path[PATH_MAX] = {0};
    off_t offset = 0;
    CodexStatus desired = STATUS_IDLE;
    CodexStatus applied = (CodexStatus)-1;
    bool task_active = false;
    time_t last_attempt = 0;

    while (keep_running) {
        char newest_path[PATH_MAX] = {0};
        struct timespec modified = {0, 0};
        if (newest_session_file(newest_path, &modified)) {
            (void)modified;
            if (strcmp(current_path, newest_path) != 0) {
                strlcpy(current_path, newest_path, sizeof(current_path));
                offset = 0;
                desired = STATUS_IDLE;
                task_active = false;
            }

            bool changed = false;
            read_new_lines(current_path, &offset, &desired, &task_active, &changed);
            if (changed || desired != applied) {
                fprintf(stdout, "K16 Codex Lights: %s\n", status_name(desired));
                fflush(stdout);
            }
        }

        time_t now = time(NULL);
        if (dry_run) {
            applied = desired;
        } else if (desired != applied && (last_attempt == 0 || now - last_attempt >= 5)) {
            last_attempt = now;
            if (set_all_keys(status_color(desired))) {
                applied = desired;
            }
        }
        usleep(500000);
    }
    return 0;
}

static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s [--dry-run]\n", program);
    fprintf(stderr, "       %s --test idle|thinking|complete|needs_input|error\n", program);
    fprintf(stderr, "       %s --request-access\n", program);
    fprintf(stderr, "       %s --replay SESSION.jsonl\n", program);
}

int main(int argc, char **argv) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (argc == 2 && strcmp(argv[1], "--request-access") == 0) {
        bool granted = IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);
        fprintf(stdout, "Input Monitoring access: %s\n", granted ? "granted" : "not granted yet");
        return granted ? 0 : 7;
    }

    if (argc == 3 && strcmp(argv[1], "--test") == 0) {
        CodexStatus status;
        if (!parse_status_name(argv[2], &status)) {
            print_usage(argv[0]);
            return 2;
        }
        if (IOHIDCheckAccess(kIOHIDRequestTypeListenEvent) != kIOHIDAccessTypeGranted) {
            fprintf(stderr, "Input Monitoring permission is required. Run --request-access first.\n");
            return 7;
        }
        return set_all_keys(status_color(status)) ? 0 : 5;
    }

    if (argc == 3 && strcmp(argv[1], "--replay") == 0) {
        return replay_file(argv[2]);
    }

    bool dry_run = argc == 2 && strcmp(argv[1], "--dry-run") == 0;
    if (argc > 2 || (argc == 2 && !dry_run)) {
        print_usage(argv[0]);
        return 2;
    }
    if (!dry_run && IOHIDCheckAccess(kIOHIDRequestTypeListenEvent) != kIOHIDAccessTypeGranted) {
        (void)IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);
        fprintf(stderr, "Input Monitoring permission is required. Enable K16 Codex Lights in System Settings; the service will keep retrying.\n");
        diagnostic("Waiting for Input Monitoring permission");
    }
    if (!dry_run) {
        diagnostic("K16 Codex Lights started; HID access granted");
    }
    return run_monitor(dry_run);
}
