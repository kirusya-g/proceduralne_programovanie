#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define YEAR 2026 // Current year for age calculation
#define MAX_LINE 256 // Max Line lenght

// Strips trailing newline characters from a string
static void newline(char *s){
    s[strcspn(s, "\r\n")] = '\0';
}

/* Parses birth date in format "DDMMYYYY"
Last 4 chars = year, previus 2 = month, rest = day
*/

static void parse_birth_date(const char *date, int *year, int *month, int *day){
    char buf[MAX_LINE];
    int len = (int)strlen(date);

    //Year: last 4 characters
    *year = (len >= 4) ? atoi(date + len - 4) : 0;
    // Month: 2 characters before year
    *month = 0;
    if (len >= 6){
        strncpy(buf, date + len - 6, 2);
        buf[2] = '\0';
        *month = atoi(buf);
    }
    //Day: everything before month
    *day = 0;
    if (len > 6){
        strncpy(buf, date, (size_t)(len - 6));
        buf[len - 6] = '\0';
        *day = atoi(buf);
    }
}

// Reads a single passenger record starting with '#' from the file
static int read_record(FILE *f, char *flight, char *first_name, char *last_name, 
    char *birth_date, char *seat, char *baggage){
        char line[MAX_LINE];

        //Skip lines until one starting with '#'
        do{
            if (fgets(line, sizeof(line), f) == NULL)
                return 0;
        } while (line[0] != '#');

        // Read the 6 data fields
        if (fgets(flight, MAX_LINE, f) == NULL) 
            return 0;
        if (fgets(first_name, MAX_LINE, f) == NULL) 
            return 0;
        if (fgets(last_name, MAX_LINE, f) == NULL) 
            return 0;
        if (fgets(birth_date, MAX_LINE, f) == NULL) 
            return 0;
        if (fgets(seat, MAX_LINE, f) == NULL) 
            return 0;
        if (fgets(baggage, MAX_LINE, f) == NULL) 
            return 0;

        // Remove newlines from each field
        newline(flight);
        newline(first_name);
        newline(last_name);
        newline(birth_date);
        newline(seat);
        newline(baggage);

        return 1;
    }

// creates a heap copy of a string
static char *copy_string(const char *s){
    char *copy = malloc(strlen(s) + 1);
    if (copy != NULL)
        strcpy(copy, s);
    return copy;
}

// Frees all dynamic passenger arrays and resets pointers to NULL
static void free_passengers(char ***flight, char ***first_name, char ***last_name, char ***birth_date, char ***seat, int **baggage, int *count){
    int i;

    if(*flight == NULL)
        return;

    for (i = 0; i < *count; i++){
        free((*flight)[i]);
        free((*first_name)[i]);
        free((*last_name)[i]);
        free((*birth_date)[i]);
        free((*seat)[i]);
    }
    free(*flight);
    free(*first_name);
    free(*last_name);
    free(*birth_date);
    free(*seat);
    free(*baggage);

    *flight = NULL;
    *first_name = NULL;
    *last_name = NULL;
    *birth_date = NULL;
    *seat = NULL;
    *baggage = NULL;
    *count = 0;
}

// free all check-in arrays and sets the pointers back to NULL
static void free_checkin(char ***flight, char ***seat, int **baggage, double ***weights, int *count){
    int i;

    if (*flight == NULL)
        return;
    
    for (i = 0; i < *count; i++){
        free((*flight)[i]);
        free((*seat)[i]);
        free((*weights)[i]);
    }

    free(*flight);
    free(*seat);
    free(*baggage);
    free(*weights);

    *flight = NULL;
    *seat = NULL;
    *baggage = NULL;
    *weights = NULL;
    *count = 0;
}

// finds the check-in record with the given flight abd seat
static int find_checkin(const char *flight, const char *seat, char **c_flight, char **c_seat, int c_count){
    int i;

    for (i = 0; i < c_count; i++)
        if(strcmp(c_flight[i], flight) == 0 && strcmp(c_seat[i], seat) == 0)
            return i;
    return -1;
    
}

// Inserts a string into a dynamic string array at the given position
static void insert_str(char ***arr, int count, int pos, const char *value){
    *arr = realloc(*arr, (size_t)(count + 1) * sizeof(char *));
    memmove(*arr + pos + 1, *arr + pos, (size_t)(count - pos) * sizeof(char *));
    (*arr)[pos] = copy_string(value);
}

// Inserts an integer into a dynamic int array at the given position
static void insert_int(int **arr, int count, int pos, int value){
    *arr = realloc(*arr, (size_t)(count + 1) * sizeof(int));
    memmove(*arr + pos + 1, *arr + pos, (size_t)(count - pos) * sizeof(int));
    (*arr)[pos] = value;
}

// Removes passengers matching the flight ID and shrinks the arrays
static int remove_passengers(const char *id, char ***flight, char ***first_name, char ***last_name, char ***birth_date, char ***seat, int **baggage, int *count){
    int i, j = 0;
    int removed;
    size_t size;

    for (i = 0; i < *count; i++){
        if(strcmp((*flight)[i], id) == 0){
            free((*flight)[i]);
            free((*first_name)[i]);
            free((*last_name)[i]);
            free((*birth_date)[i]);
            free((*seat)[i]);
        } else {
            (*flight)[j] = (*flight)[i];
            (*first_name)[j] = (*first_name)[i];
            (*last_name)[j] = (*last_name)[i];
            (*birth_date)[j] = (*birth_date)[i];
            (*seat)[j] = (*seat)[i];
            (*baggage)[j] = (*baggage)[i];
            j++;
        }
    }

    removed = *count - j;
    *count = j;

    size = (j > 0) ? (size_t)j : 1;
    *flight = realloc(*flight, size * sizeof(char *));
    *first_name = realloc(*first_name, size * sizeof(char *));
    *last_name = realloc(*last_name, size * sizeof(char *));
    *birth_date = realloc(*birth_date, size * sizeof(char *));
    *seat = realloc(*seat, size * sizeof(char *));
    *baggage = realloc(*baggage, size * sizeof(int));

    return removed;
}

// Removes check-in records matching the flight ID and shrinks the arrays
static void remove_checkin(const char *id, char ***flight, char ***seat, int **baggage, double ***weights, int *count){
    int i, j = 0;
    size_t size;

    for (i = 0; i < *count; i++){
        if(strcmp((*flight)[i], id) == 0){
            free((*flight)[i]);
            free((*seat)[i]);
            free((*weights)[i]);
        } else {
            (*flight)[j] = (*flight)[i];
            (*seat)[j] = (*seat)[i];
            (*baggage)[j] = (*baggage)[i];
            (*weights)[j] = (*weights)[i];
            j++;
        }
    }

    *count = j;

    size = (j > 0) ? (size_t)j : 1;
    *flight = realloc(*flight, size * sizeof(char *));
    *seat = realloc(*seat, size * sizeof(char *));
    *baggage = realloc(*baggage, size * sizeof(int));
    *weights = realloc(*weights, size * sizeof(double *));
}

// Prints all check-in records formatted with hash separators
static void print_checkin(char **c_flight, char **c_seat, int *c_bag, double **c_weights, int c_count){
    int i, b;

    for (i = 0; i < c_count; i++){
        printf("%s#%s#%d", c_flight[i], c_seat[i], c_bag[i]);
        for (b = 0; b < c_bag[i]; b++)
            printf("#%.2f", c_weights[i][b]);
        printf("\n");
    }
}

    /* prints all passangers from "zoznamCK.txt" with flight, name
    age and seat. Files are opened on first call and kept open
    */
static void v1(FILE **passengers, FILE **checkin){
    char flight[MAX_LINE], first_name[MAX_LINE], last_name[MAX_LINE], birth_date[MAX_LINE], seat[MAX_LINE], baggage[MAX_LINE];
    int year, month, day;

    //Open files if not already open
    if (*passengers == NULL)
        *passengers = fopen("zoznamCK.txt", "r");
    if (*checkin == NULL)
        *checkin = fopen("checkin1.txt", "r");
    
    // Handle open errors
    if (*passengers == NULL || *checkin == NULL){
        printf("V1: Neotvorene txt subory.\n");
        if (*passengers != NULL){
            fclose(*passengers);
            *passengers = NULL;
        }
        if (*checkin != NULL){
            fclose(*checkin);
            *checkin = NULL;
        }
        return;
    }

    // Rewind and read all records
    rewind(*passengers);
    while (read_record(*passengers, flight, first_name, last_name, birth_date, seat, baggage)){
        parse_birth_date(birth_date, &year, &month, &day);
        printf("Oznacenie letu: %s\n", flight);
        printf("Meno a Priezvisko [vek]: %s %s [%d]\n", first_name, last_name, YEAR - year);
        printf("Oznacenie sedadla: %s\n", seat);
        printf("\n");
    }
}

// Prints passenger details and check-in status from dynamic arrays
static void v2(char **p_flight, char **p_first, char **p_last, char **p_date, char **p_seat, int *p_bag, int p_count, char **c_flight, char **c_seat, int *c_bag, double **c_weights, int c_count){
    int i, b, idx, year, month, day;

    if(p_flight == NULL){
        printf("V2: Nenaplnene polia.\n");
        return;
    }

    for (i = 0; i < p_count; i++){
        parse_birth_date(p_date[i], &year, &month, &day);

        idx = find_checkin(p_flight[i], p_seat[i], c_flight, c_seat, c_count);

        printf("Oznacenie letu: %s\n", p_flight[i]);
        printf("Meno a Priezvisko [vek]: %s %s [%d]\n", p_first[i], p_last[i], YEAR - year);
        printf("Oznacenie sedadla: %c %s\n", p_seat[i][0], p_seat[i] + 1);
        printf("Status: %s\n", (idx >= 0) ? "true" : "false");

        printf("Pocet batoziny: %d / %d", p_bag[i], (idx >= 0) ? c_bag[idx] : 0);
        if (idx >= 0){
            for (b = 0; b < c_bag[idx]; b++){
                if (b == 0)
                    printf(" [%.2f]", c_weights[idx][b]);
                else
                    printf(",[%.2f]", c_weights[idx][b]);
            }
        }
        printf("\n\n");
    }
}

/* Reads next char and dispatches to the right funtion*/
static void v_x(FILE **passengers, FILE **checkin, char **p_flight, char **p_first, char **p_last, char **p_date, char **p_seat, int *p_bag, int p_count, char **c_flight, char **c_seat, int *c_bag, double **c_weights, int c_count){
    char option;

    if (scanf(" %c", &option) != 1)
        return;

    switch (option)
    {
    case '1':
        v1(passengers, checkin);
        break;
    case '2':
        v2(p_flight, p_first, p_last, p_date, p_seat, p_bag, p_count, c_flight, c_seat, c_bag, c_weights, c_count);
        break;

    default:
        printf("V: Nespravna volba vypisu.\n");
        break;
    }
}

/* character frequency histogram of passengers file*/
static void h(FILE *passengers){
    int count[256] = {0}; // Counters for all byte values
    int upper[26], digits[10]; // Arrays for sorting
    int i, j, tmp, c;

    if (passengers == NULL){
        printf("H: Neotvoreny subor.\n");
        return;
    }

    // Count each character in the file
    rewind(passengers);
    while((c = fgetc(passengers)) != EOF)
        count[(unsigned char)c]++; // Take the character c, reinterpret its byte value as an unsigned number (0–255), use it as an index into the count array, and increment that counter by one.
    
        // prepare character arrays for sorting
    for (i = 0; i < 26; i++)
        upper[i] = 'A' + i;
    for (i = 0; i < 10; i++)
        digits[i] = '0' + i;

    for(i = 1; i < 26; i++){
        tmp = upper[i];
        j = i - 1;
        while (j >= 0 && count[upper[j]] < count[tmp]) {
            upper[j + 1] = upper[j];
            j--;
        }
        upper[j + 1] = tmp;
    }

    for (i = 1; i < 10; i++){
        tmp = digits[i];
        j = i - 1;
        while (j >= 0 && count[digits[j]] < count[tmp]){
            digits[j + 1] = digits[j];
            j--;
        }
        digits[j + 1] = tmp;
    }

    for (i = 0; i < 26; i++)
        if (isupper(upper[i]) && count[upper[i]] > 0)
            printf("%c : %d\n", upper[i], count[upper[i]]);


    for (c = 'a'; c <= 'z'; c++){
        if (islower(c) && count[c] > 0){
            printf("%c : %d\n", c, count[c]);
        }
    }

    for (i = 0; i < 10; i++){
        if (isdigit(digits[i]) && count[digits[i]] > 0){
            printf("%c : %d\n", digits[i], count[digits[i]]);
        }
    }
}

// Counts and loads records from files into dynamic arrays
static void n(FILE *passengers_file, FILE *checkin_file, char ***p_flight, char ***p_first, char ***p_last, char ***p_date, char ***p_seat, int **p_bag, int *p_count, char ***c_flight, char ***c_seat, int **c_bag, double ***c_weights, int *c_count){
    char flight[MAX_LINE], first_name[MAX_LINE], last_name[MAX_LINE];
    char birth_date[MAX_LINE], seat[MAX_LINE], baggage[MAX_LINE];
    char line[MAX_LINE];

    char *token;
    int i, b;
    size_t size;

    if (passengers_file == NULL || checkin_file == NULL){
        printf("N: Neotvoreny subor\n");
        return;
    }

    free_passengers(p_flight, p_first, p_last, p_date, p_seat, p_bag, p_count);
    free_checkin(c_flight, c_seat, c_bag, c_weights, c_count);

    rewind(passengers_file);
    while(read_record(passengers_file, flight, first_name, last_name, birth_date, seat, baggage))
        (*p_count)++;

    size = (*p_count > 0) ? (size_t)*p_count : 1;
    *p_flight = malloc(size * sizeof(char *));
    *p_first = malloc(size * sizeof(char *));
    *p_last = malloc(size * sizeof(char *));
    *p_date = malloc(size * sizeof(char *));
    *p_seat = malloc(size * sizeof(char *));
    *p_bag = malloc(size * sizeof(int));

    rewind(passengers_file);
    i = 0;
    while(i < *p_count && read_record(passengers_file, flight, first_name, last_name, birth_date, seat, baggage)){
        (*p_flight)[i] = copy_string(flight);
        (*p_first)[i] = copy_string(first_name);
        (*p_last)[i] = copy_string(last_name);
        (*p_date)[i] = copy_string(birth_date);
        (*p_seat)[i] = copy_string(seat);
        (*p_bag)[i] = atoi(baggage);
        i++;
    }

    rewind(checkin_file);
    while(fgets(line, MAX_LINE, checkin_file) != NULL){
        newline(line);
        if (line[0] != '\0')
            (*c_count)++;
    }

    size = (*c_count > 0) ? (size_t)*c_count : 1;
    *c_flight = malloc(size * sizeof(char *));
    *c_seat = malloc(size * sizeof(char *));
    *c_bag = malloc(size * sizeof(int));
    *c_weights = malloc(size * sizeof(double *));

    rewind(checkin_file);
    i = 0;
    while (i < *c_count && fgets(line, MAX_LINE, checkin_file) != NULL){
        newline(line);
        if (line[0] == '\0')
            continue;

        token = strtok(line, "#");
        (*c_flight)[i] = copy_string(token);
        token = strtok(NULL, "#");
        (*c_seat)[i] = copy_string(token);
        token = strtok(NULL, "#");
        (*c_bag)[i] = atoi(token);

        size = ((*c_bag)[i] > 0) ? (size_t)(*c_bag)[i] : 1;
        (*c_weights)[i] = malloc(size * sizeof(double));
        for (b = 0; b < (*c_bag)[i]; b++){
            token = strtok(NULL, "#");
            (*c_weights)[i][b] = (token != NULL) ? atof(token) : 0.0;
        }
        i++;
    }
}

// Adds a new passenger to dynamic arrays at position Y
static void q(char ***p_flight, char ***p_first, char ***p_last, char ***p_date, char ***p_seat, int **p_bag, int *p_count){
    int y, pos, bag;
    char flight[MAX_LINE], first_name[MAX_LINE], last_name[MAX_LINE];
    char birth_date[MAX_LINE], seat[MAX_LINE];

    if(scanf(" %d", &y) != 1)
        return;
    if(scanf(" %255s %255s %255s %255s %255s %d", flight, first_name, last_name, birth_date, seat, &bag) != 6)
        return;

    if(*p_flight == NULL){
        printf("Q: Polia nie su vytvorena\n");
        return;
    }

    pos = y - 1;
    if (pos < 0)
        pos = 0;
    if (pos > *p_count)
        pos = *p_count;

    insert_str(p_flight, *p_count, pos, flight);
    insert_str(p_first, *p_count, pos, first_name);
    insert_str(p_last, *p_count, pos, last_name);
    insert_str(p_date, *p_count, pos, birth_date);
    insert_str(p_seat, *p_count, pos, seat);
    insert_int(p_bag, *p_count, pos, bag);
    (*p_count)++;
}

// Deletes all records for a given flight ID from dynamic arrays
static void w(char ***p_flight, char ***p_first, char ***p_last, char ***p_date, char ***p_seat, int **p_bag, int *p_count, char ***c_flight, char ***c_seat, int **c_bag, double ***c_weights, int *c_count){
    char id[MAX_LINE];
    int removed;

    if(scanf(" %255s", id) != 1)
        return;

    if (*p_flight == NULL){
        printf("W: Polia nie su vytvorene\n");
        return;
    }

    removed = remove_passengers(id, p_flight, p_first, p_last, p_date, p_seat, p_bag, p_count);
    remove_checkin(id, c_flight, c_seat, c_bag, c_weights, c_count);

    printf("W: Vymazalo sa: %d zaznamov!\n", removed);
}

// Updates baggage weights for a seat and prints before and after data
static double **e(char **c_flight, char **c_seat, int *c_bag, double **c_weights, int c_count){
    char seat[MAX_LINE];
    int i, b;

    if(scanf(" %255s", seat) != 1)
        return c_weights;
    
    if (c_flight == NULL){
        printf("E: Polia nie su vytvorene\n");
        return c_weights;
    }

    print_checkin(c_flight, c_seat, c_bag, c_weights, c_count);

    for (i = 0; i < c_count; i++){
        if(strcmp(c_seat[i], seat) == 0){
            for (b = 0; b < c_bag[i]; b++)
                scanf(" %lf", &c_weights[i][b]);
        }
    }

    print_checkin(c_flight, c_seat, c_bag, c_weights, c_count);

    return c_weights;
}

/*
    'v' - displays data (1: from files, 2: from arrays)
    'h' - character frequency histogram
    'n' - loads files into dynamic arrays
    'q' - inserts a passenger at position Y
    'w' - deletes passengers by flight ID
    'e' - updates baggage weights for a seat
 */
int main(void){
    FILE *passengers = NULL;
    FILE *checkin = NULL;
    char command;

    char **p_flight = NULL, **p_first = NULL, **p_last = NULL;
    char **p_date = NULL, **p_seat = NULL;
    int *p_bag = NULL;
    int p_count= 0;

    char **c_flight = NULL, **c_seat = NULL;
    int *c_bag = NULL;
    double **c_weights = NULL;
    int c_count = 0; 

    while (scanf(" %c", &command) == 1){
        switch (command)
        {
        case 'v':
            v_x(&passengers, &checkin, p_flight, p_first, p_last, p_date, p_seat, p_bag, p_count, c_flight, c_seat, c_bag, c_weights, c_count);
            break;
        case 'h':
            h(passengers);
            break;
        case 'n':
            n(passengers, checkin, &p_flight, &p_first, &p_last, &p_date, &p_seat, &p_bag, &p_count, &c_flight, &c_seat, &c_bag, &c_weights, &c_count);
            break;
        case 'q':
            q(&p_flight, &p_first, &p_last, &p_date, &p_seat, &p_bag, &p_count);
            break;
        case 'w':
            w(&p_flight, &p_first, &p_last, &p_date, &p_seat, &p_bag, &p_count, &c_flight, &c_seat, &c_bag, &c_weights, &c_count);
            break;
        case 'e':
            c_weights = e(c_flight, c_seat, c_bag, c_weights, c_count);
            break;
        
        default:
            break;
        }
    }

}
