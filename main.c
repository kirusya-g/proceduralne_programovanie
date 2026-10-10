#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define YEAR 2026
#define MAX_LINE 256

static void newline(char *s){
    s[strcspn(s, "\r\n")] = '\0';
}

static void parse_birth_date(const char *date, int *year, int *month, int *day){
    char buf[MAX_LINE];
    int len = (int)strlen(date);

    *year = (len >= 4) ? atoi(date + len - 4) : 0;

    *month = 0;
    if (len >= 6){
        strncpy(buf, date + len - 6, 2);
        buf[2] = '\0';
        *month = atoi(buf);
    }

    *day = 0;
    if (len > 6){
        strncpy(buf, date, (size_t)(len - 6));
        buf[len - 6] = '\0';
        *day = atoi(buf);
    }
}

static int read_record(FILE *f, char *flight, char *first_name, char *last_name, 
    char *birth_date, char *seat, char *baggage){
        char line[MAX_LINE];

        do{
            if (fgets(line, sizeof(line), f) == NULL)
                return 0;
        } while (line[0] != '#');

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

        newline(flight);
        newline(first_name);
        newline(last_name);
        newline(birth_date);
        newline(seat);
        newline(baggage);

        return 1;
    }

static void v1(FILE **passengers, FILE **checkin){
    char flight[MAX_LINE], first_name[MAX_LINE], last_name[MAX_LINE], birth_date[MAX_LINE], seat[MAX_LINE], baggage[MAX_LINE];
    int year, month, day;

    if (*passengers == NULL)
        *passengers = fopen("zoznamCK.txt", "r");
    if (*checkin == NULL)
        *checkin = fopen("checkin1.txt", "r");
    
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

    rewind(*passengers);
    while (read_record(*passengers, flight, first_name, last_name, birth_date, seat, baggage)){
        parse_birth_date(birth_date, &year, &month, &day);
        printf("Oznacenie letu: %s\n", flight);
        printf("Meno a Priezvisko [vek]: %s %s [%d]\n", first_name, last_name, YEAR - year);
        printf("Oznacenie sedadla: %s\n", seat);
        printf("\n");
    }
}

static void v_x(FILE **passengers, FILE **checkin){
    char option;

    if (scanf(" %c", &option) != 1)
        return;

    switch (option)
    {
    case '1':
        v(passengers, checkin);
        break;
    
    default:
        printf("V: Nespravna volba vypisu.\n");
        break;
    }
}