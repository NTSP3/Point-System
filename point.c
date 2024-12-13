#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This file is a program to keep track of my stuff so I complete my work
int rows, cols;

#define MAX_LINE 256
#define MAX_VARS 100

typedef struct {
    char key[MAX_LINE];
    char value[MAX_LINE];
} IniVar;

int is_file_integer(FILE *file) {
    rewind(file); // Ensure we start reading from the beginning of the file

    int ch;
    int has_digits = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (!isdigit(ch) && ch != '\n') {
            return 0; // Not an integer
        }
        if (isdigit(ch)) {
            has_digits = 1;
        }
    }

    return has_digits; // Returns 1 if there are digits, 0 otherwise
}

void reset_file(FILE *file) {
    freopen(NULL, "w+", file); // Reopen the file in write mode, erasing all content
    if (file == NULL) {
        perror("Error resetting file");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "0\n"); // Write '0' to the file
    fflush(file);         // Ensure content is written to disk
}

int parse_ini_section(const char *filename, const char *section, IniVar vars[], int max_vars) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    char line[MAX_LINE];
    int inside_section = 0;
    int var_count = 0;

    while (fgets(line, sizeof(line), file)) {
        // Trim newline characters
        line[strcspn(line, "\r\n")] = 0;

        // Check for section header
        if (line[0] == '[' && strstr(line, section) == line + 1 && line[strlen(line) - 1] == ']') {
            inside_section = 1;
            continue;
        }

        // If we encounter another section, exit the current section
        if (inside_section && line[0] == '[') {
            break;
        }

        // Parse key-value pairs inside the section
        if (inside_section) {
            char *eq = strchr(line, '=');
            if (eq && var_count < max_vars) {
                *eq = 0; // Split key and value
                strncpy(vars[var_count].key, line, MAX_LINE);
                strncpy(vars[var_count].value, eq + 1, MAX_LINE);

                // Remove quotes around key and value if they exist
                char *key = vars[var_count].key;
                char *value = vars[var_count].value;

                if (*key == '"') memmove(key, key + 1, strlen(key)); // Remove leading quote
                if (key[strlen(key) - 1] == '"') key[strlen(key) - 1] = 0; // Remove trailing quote

                if (*value == '"') memmove(value, value + 1, strlen(value));
                if (value[strlen(value) - 1] == '"') value[strlen(value) - 1] = 0;

                var_count++;
            }
        }
    }

    fclose(file);
    return var_count;
}

int dialog(const char *message) {
    int win_height = 3;
    int win_width = strlen(message) + 2;

    // Calculate window's top-left corner position to center it
    int start_row = 0;
    int start_col = cols - win_width;

    // Create the window
    WINDOW *win = newwin(win_height, win_width, start_row, start_col);

    // Add border to the window
    box(win, 0, 0);

    // Print the message in the center of the window
    mvwprintw(win, 1, 1, "%s", message);

    // Refresh the window to show changes
    wrefresh(win);

    // Wait for user input
    int key;
    while (1) {
        key = tolower(getch()); // Read input and convert to lowercase

        if (key == 10) { // ENTER key
            // Close the window
            wclear(win);
            wrefresh(win);
            delwin(win);
            return EXIT_SUCCESS;
        } else if (key == 27) { // ESC key
            // Close the window
            wclear(win);
            wrefresh(win);
            delwin(win);
            return EXIT_FAILURE;
        }
    }
}

int save_point(int score) {
    FILE *file = fopen("score.txt", "w");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Write new content to the file
    fprintf(file, "%d", score);

    // Close the file
    fclose(file);

    // Print new score
    move(0, 0);
    printw("Point balance: %d    ", score);
    refresh();
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    // Init variables
    char key;
    int score;

    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    getmaxyx(stdscr, rows, cols);

    // Check if the window is large enough
    if (rows < 14 || cols < 30) {
        printw("\nWindow too small! Make sure it is at least 14x30:\n");
        printw("Current columns: %d ; Current rows: %d\n\n", cols, rows);
        printw("Continue anyway? [Y / other key]");
        refresh();
        key = getch();
        if (key != 'Y' && key != 'y') {
            endwin();
            return EXIT_FAILURE;
        }
    }

    // Create a subwindow
    clear();
    int start_row = 3;
    int start_col = 6;
    int width = cols - (start_col * 2);
    int height = rows - (start_row * 2);
    WINDOW *win = newwin(height, width, start_row, start_col);
    refresh();

    // Open and check the score file
    FILE *file = fopen("score.txt", "a+");
    if (file == NULL) {
        endwin();
        perror("Error opening score file");
        return EXIT_FAILURE;
    }

    if (!is_file_integer(file)) {
        reset_file(file);
        score = 0;
    } else {
        rewind(file); // Go back to the start to read the integer
        fscanf(file, "%d", &score);
    }

    // Display the score
    printw("Point balance: %d\n", score);
    printw("Press 'Q' to quit.");
    refresh();

    // Parse the INI file
    IniVar item[MAX_VARS];
    IniVar store[MAX_VARS];
    int total_items = parse_ini_section("items.ini", "item", item, MAX_VARS);
    int total_stores = parse_ini_section("items.ini", "store", store, MAX_VARS);

    if (total_items < 0 || total_stores < 0) {
        endwin();
        return EXIT_FAILURE; // Error handling
    }

    // Display items in the window
    wprintw(win, "\n\n");
    for (int i = 0; i < total_items; i++) {
        wprintw(win, "   [EARN] (%c) %s :: +%s Points\n", 'a' + i, item[i].key, item[i].value + strspn(item[i].value, " "));
    }

    wprintw(win, "\n");

    for (int i = 0; i < total_stores; i++) {
        const char *trimmed_value = store[i].value + strspn(store[i].value, " ");
        char *endptr;
        long cost = strtol(trimmed_value, &endptr, 10);
        if (*endptr != '\0') {
            wprintw(win, "    [BUY] (%c) %s :: INVALID COST: %s\n", 'a' + total_items + i, store[i].key, store[i].value);
        } else {
            wprintw(win, "    [BUY] (%c) %s :: -%ld Points\n", 'a' + total_items + i, store[i].key, cost);
        }
    }

    // Add border and title to the window
    box(win, 0, 0);
    wmove(win, 0, 1);
    wprintw(win, "Tasks");
    wrefresh(win);

    // Print the question
    move(rows - 2, 10);
    printw("What did you do? ");
    refresh();

    // Check for user input
    int total_options = total_items + total_stores;
    char msg[1024];
    while ((key = tolower(getch())) != 'q') {
        if (key >= 'a' && key < 'a' + total_options) {
            if (key < 'a' + total_items) {
                sprintf(msg, "Earn%s points from item '%s'?", item[key - 'a'].value, item[key - 'a'].key);
                if (dialog(msg) == EXIT_SUCCESS) {
                    char *endptr;
                    long points = strtol(item[key - 'a'].value, &endptr, 10); // Parse as integer
                    if (*endptr == '\0') { // Check for successful conversion
                        score += points;
                    } else {
                        wprintw(win, "Error: Invalid point value in item[%d]\n", key - 'a');
                    }
                    save_point(score);
                }
            } else {
                sprintf(msg, "Buy item '%s' for%s points?", store[key - 'a' - total_items].key, store[key - 'a' - total_items].value);
                if (dialog(msg) == EXIT_SUCCESS) {
                    char *endptr;
                    long cost = strtol(store[key - 'a' - total_items].value, &endptr, 10); // Parse as integer
                    if (*endptr == '\0') { // Check for successful conversion
                        if (score >= cost) {
                            score -= cost; // Subtract cost if enough points are available
                            save_point(score); // Save updated score
                        } else {
                            dialog("Not enough points."); // Show error dialog
                        }
                    } else {
                        wprintw(win, "Error: Invalid cost value in store[%d]\n", key - 'a' - total_items);
                    }
                }
            }
            refresh();
        }
    }

    // Quit program
    endwin();
    return EXIT_SUCCESS;
}
