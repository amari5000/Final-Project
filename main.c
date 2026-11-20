#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#ifdef USE_RPI_GPIO
#include <lgpio.h>
#endif

#define MAX_READINGS 1000
#define READINGS_FILE "readings.csv"
#define SETTINGS_FILE "settings.cfg"
#define LED_GPIO 17

typedef struct {
    time_t timestamp;
    double temperature;
    int warning;
} Reading;

static Reading readings[MAX_READINGS];
static size_t reading_count = 0;

static double lower_limit = 68.0;
static double upper_limit = 76.0;

/* =========================================
   LED CONTROL
   ========================================= */

#ifdef USE_RPI_GPIO

static int gpio_handle = -1;
static int led_ready = 0;

void led_init(void) {
    int result;

    gpio_handle = lgGpiochipOpen(0);

    if (gpio_handle < 0) {
        printf("WARNING: Could not open GPIO chip.\n");
        return;
    }

    result = lgGpioClaimOutput(gpio_handle, 0, LED_GPIO, 0);

    if (result < 0) {
        printf("WARNING: Could not claim GPIO pin.\n");
        lgGpiochipClose(gpio_handle);
        gpio_handle = -1;
        return;
    }

    led_ready = 1;

    printf("GPIO LED initialized successfully.\n");
}

void led_set(int on) {
    if (led_ready) {
        lgGpioWrite(gpio_handle, LED_GPIO, on ? 1 : 0);
    }
}

void led_cleanup(void) {
    if (led_ready) {
        lgGpioWrite(gpio_handle, LED_GPIO, 0);
        lgGpioFree(gpio_handle, LED_GPIO);
        led_ready = 0;
    }

    if (gpio_handle >= 0) {
        lgGpiochipClose(gpio_handle);
        gpio_handle = -1;
    }
}

#else

/* Mac testing mode */

void led_init(void) {
    printf("LED simulation enabled.\n");
}

void led_set(int on) {
    printf("[LED SIMULATION] %s\n",
           on ? "LED ON - WARNING" : "LED OFF - NORMAL");
}

void led_cleanup(void) {
    /* No physical LED connected in Mac mode. */
}

#endif

/* =========================================
   INPUT FUNCTIONS
   ========================================= */

int read_line(const char *prompt, char *buffer, size_t size) {
    size_t length;

    printf("%s", prompt);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    length = strlen(buffer);

    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else {
        int ch;

        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }

    return 1;
}

int parse_double_value(const char *text, double *value) {
    char *end;
    double parsed;

    errno = 0;
    parsed = strtod(text, &end);

    if (text == end || errno == ERANGE || !isfinite(parsed)) {
        return 0;
    }

    while (isspace((unsigned char)*end)) {
        end++;
    }

    if (*end != '\0') {
        return 0;
    }

    *value = parsed;
    return 1;
}

double get_double_input(const char *prompt) {
    char line[100];
    double value;

    while (1) {
        if (!read_line(prompt, line, sizeof(line))) {
            return NAN;
        }

        if (parse_double_value(line, &value)) {
            return value;
        }

        printf("Please enter a valid number.\n");
    }
}

int get_int_input(const char *prompt) {
    char line[100];
    char *end;
    long value;

    while (1) {
        if (!read_line(prompt, line, sizeof(line))) {
            return -1;
        }

        errno = 0;
        value = strtol(line, &end, 10);

        if (line == end || errno == ERANGE) {
            printf("Please enter a valid menu number.\n");
            continue;
        }

        while (isspace((unsigned char)*end)) {
            end++;
        }

        if (*end == '\0' && value >= 0 && value <= 100) {
            return (int)value;
        }

        printf("Please enter a valid menu number.\n");
    }
}

/* =========================================
   SETTINGS FILE
   ========================================= */

void load_settings(void) {
    FILE *file;
    double low, high;

    file = fopen(SETTINGS_FILE, "r");

    if (file == NULL) {
        return;
    }

    if (fscanf(file, "%lf %lf", &low, &high) == 2) {
        if (isfinite(low) && isfinite(high) &&
            low >= -40.0 && high <= 150.0 && low < high) {

            lower_limit = low;
            upper_limit = high;
        }
    }

    fclose(file);
}

int save_settings(void) {
    FILE *file;

    file = fopen(SETTINGS_FILE, "w");

    if (file == NULL) {
        perror("Could not save settings");
        return 0;
    }

    fprintf(file, "%.2f %.2f\n", lower_limit, upper_limit);

    fclose(file);
    return 1;
}

/* =========================================
   READING FILE
   ========================================= */

void load_readings(void) {
    FILE *file;
    char line[200];

    file = fopen(READINGS_FILE, "r");

    if (file == NULL) {
        return;
    }

    /* Skip CSV header */

    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file) != NULL &&
           reading_count < MAX_READINGS) {

        long long epoch;
        double temperature;
        char status[20];

        if (sscanf(line, "%lld,%lf,%19[^\r\n]",
                   &epoch, &temperature, status) == 3 &&
            isfinite(temperature)) {

            readings[reading_count].timestamp = (time_t)epoch;
            readings[reading_count].temperature = temperature;

            readings[reading_count].warning =
                (strcmp(status, "WARNING") == 0);

            reading_count++;
        }
    }

    fclose(file);
}

int save_reading_to_file(const Reading *reading) {
    FILE *file;
    long position;

    file = fopen(READINGS_FILE, "a+");

    if (file == NULL) {
        perror("Could not open readings file");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    position = ftell(file);

    if (position == 0) {
        fprintf(file, "timestamp,temperature_f,status\n");
    }

    fprintf(file, "%lld,%.2f,%s\n",
            (long long)reading->timestamp,
            reading->temperature,
            reading->warning ? "WARNING" : "NORMAL");

    if (fclose(file) != 0) {
        perror("Could not finish saving reading");
        return 0;
    }

    return 1;
}

/* =========================================
   TEMPERATURE FUNCTIONS
   ========================================= */

void add_reading(void) {
    Reading new_reading;
    double temperature;

    if (reading_count >= MAX_READINGS) {
        printf("Maximum reading limit reached.\n");
        return;
    }

    temperature = get_double_input(
        "Enter classroom temperature (F): ");

    if (!isfinite(temperature)) {
        printf("Input cancelled.\n");
        return;
    }

    if (temperature < -40.0 || temperature > 150.0) {
        printf("Temperature must be between -40 and 150 F.\n");
        return;
    }

    new_reading.timestamp = time(NULL);
    new_reading.temperature = temperature;

    new_reading.warning =
        (temperature < lower_limit ||
         temperature > upper_limit);

    if (!save_reading_to_file(&new_reading)) {
        printf("Reading was not saved.\n");
        return;
    }

    readings[reading_count] = new_reading;
    reading_count++;

    printf("\nTemperature: %.2f F\n", temperature);

    printf("Acceptable range: %.2f - %.2f F\n",
           lower_limit, upper_limit);

    if (new_reading.warning) {
        printf("Status: WARNING\n");
    } else {
        printf("Status: NORMAL\n");
    }

    led_set(new_reading.warning);
}

void show_summary(void) {
    size_t i;
    size_t warning_count = 0;
    double total = 0.0;

    printf("\n========== TEMPERATURE SUMMARY ==========\n");

    printf("Acceptable range: %.2f - %.2f F\n",
           lower_limit, upper_limit);

    printf("Total saved readings: %zu\n", reading_count);

    if (reading_count == 0) {
        printf("No readings recorded yet.\n");
        return;
    }

    for (i = 0; i < reading_count; i++) {
        total += readings[i].temperature;

        if (readings[i].temperature < lower_limit ||
            readings[i].temperature > upper_limit) {

            warning_count++;
        }
    }

    printf("Average temperature: %.2f F\n",
           total / (double)reading_count);

    printf("Normal readings: %zu\n",
           reading_count - warning_count);

    printf("Warning readings: %zu\n", warning_count);

    printf("=========================================\n");
}

void show_recent_readings(void) {
    size_t i;
    size_t start;
    char date_text[40];

    printf("\n========== RECENT READINGS ==========\n");

    if (reading_count == 0) {
        printf("No readings recorded yet.\n");
        return;
    }

    start = (reading_count > 10)
            ? reading_count - 10
            : 0;

    for (i = start; i < reading_count; i++) {
        struct tm *time_info;

        time_info = localtime(&readings[i].timestamp);

        if (time_info != NULL) {
            strftime(date_text, sizeof(date_text),
                     "%Y-%m-%d %H:%M:%S", time_info);
        } else {
            strcpy(date_text, "Unknown time");
        }

        printf("%s | %.2f F | %s\n",
               date_text,
               readings[i].temperature,
               readings[i].warning ? "WARNING" : "NORMAL");
    }

    printf("=====================================\n");
}

/* =========================================
   CONFIGURATION
   ========================================= */

void change_range(void) {
    double new_low;
    double new_high;

    printf("\nCurrent range: %.2f - %.2f F\n",
           lower_limit, upper_limit);

    new_low = get_double_input(
        "Enter new minimum temperature (F): ");

    if (!isfinite(new_low)) {
        printf("Change cancelled.\n");
        return;
    }

    new_high = get_double_input(
        "Enter new maximum temperature (F): ");

    if (!isfinite(new_high)) {
        printf("Change cancelled.\n");
        return;
    }

    if (new_low < -40.0 || new_high > 150.0 ||
        new_low >= new_high) {

        printf("Invalid temperature range.\n");
        return;
    }

    lower_limit = new_low;
    upper_limit = new_high;

    if (save_settings()) {
        printf("Temperature range updated successfully.\n");
        printf("New range: %.2f - %.2f F\n",
               lower_limit, upper_limit);
    }
}

/* =========================================
   MENU
   ========================================= */

void display_menu(void) {
    printf("\n========================================\n");
    printf("     SMART CLASSROOM COMFORT MONITOR\n");
    printf("========================================\n");

    printf("1. Enter a temperature reading\n");
    printf("2. View temperature summary\n");
    printf("3. View recent readings\n");
    printf("4. Change acceptable temperature range\n");
    printf("5. Exit\n");
}

/* =========================================
   MAIN PROGRAM
   ========================================= */

int main(void) {
    int choice;

    load_settings();
    load_readings();
    led_init();

    printf("\nSmart Classroom Comfort Monitor started.\n");

    printf("Loaded %zu previous reading(s).\n",
           reading_count);

    while (1) {
        display_menu();

        choice = get_int_input("Choose an option: ");

        switch (choice) {
            case 1:
                add_reading();
                break;

            case 2:
                show_summary();
                break;

            case 3:
                show_recent_readings();
                break;

            case 4:
                change_range();
                break;

            case 5:
                led_set(0);
                led_cleanup();

                printf("Program closed.\n");
                printf("Readings saved in %s.\n",
                       READINGS_FILE);

                return 0;

            default:
                printf("Please select an option from 1 to 5.\n");
        }
    }
}
