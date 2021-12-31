#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "timedate.h"

static uint8_t days_in_month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

int8_t add_days_to_date(struct date_yyyymmdd_t *date, struct date_yyyymmdd_t *output, uint16_t add_days) {
    if (date == NULL) {
        printf("error: date is missing\n");
        return 0;
    }
    
    output->year = 1970;
    output->month = 1;
    output->day = 1;
    
    uint8_t done = 0;
    
    int64_t timestamp = get_timestamp(date);

    uint16_t days = add_days + (timestamp / (24*60*60));
    
    uint16_t days_in_a_year = 0;    
    
    while (!done) {
        days_in_a_year = (365 + is_leap_year(output->year));
        if (days >= days_in_a_year) {
            days -= days_in_a_year;
            output->year++;
        } else if (days >= days_in_month[output->month - 1]) {
            if ((output->month == 2) && (days_in_a_year == 366)) {
                if (days >= 29) {
                    output->month++;
                    days -= 29;
                } else {
                    done = 1;
                    output->day += days;
                }
            } else {
                days -= days_in_month[output->month - 1];                           
                output->month++;
            }
            
        } else {
            done = 1;
            output->day += days;
        }
    }
    
#ifdef DEBUG            
            printf("date: %04d-%02d-%02d\t%15lld\n", output->year, output->month, output->day, get_timestamp(output));
#endif  
    
    return 1;
    
}

uint8_t is_leap_year(int year) {
    if ((year % 4) > 0) {
        return 0;
    } else if ((year % 100) > 0) {
        return 1;
    } else if ((year % 400) == 0) {
        return 1;
    }
    
    return 0;

}

uint8_t is_valid_date(struct date_yyyymmdd_t *date) {
    uint8_t date_is_in_the_future = 0;
    
    if ((date->month == 2) && (date->day == 29)) {
        if (is_leap_year(date->year)) {
            return 1;
        } else {
            printf("error: that's not a leap year!\n");
            return 0;
        }
    } else if ((date->day < 1) || (date->day > days_in_month[date->month - 1])) {
        printf("error: invalid day.\n");
        return 0;
    } else if ((date->month < 1) || (date->month > 12)) {
        printf("error: invalid month.\n");
        return 0;
    } else if (date->year < 2013) {
        printf("error: no records before 2013-04-28\n");
        return 0;
    } else if ((date->year == 2013) && (date->month < 4)) {
        printf("error: no records before 2013-04-28\n");
        return 0;
    } else if ((date->year == 2013) && (date->month == 4) && (date->day < 28)) {
        printf("error: no records before 2013-04-28\n");
        return 0;
    } else if (date_is_in_the_future) {
        /*
         * TODO: implement date check.
         *  convert current system time/date to UTC before checking against
         *  requires timezone and offset and so on... luckily the software works without it
         */
        printf("error: date can't be in the future (UTC)\n");
        return 0;
    }

    return 1;
}

int64_t get_timestamp (struct date_yyyymmdd_t *date) {
    uint32_t days = 0;
    int64_t timestamp;
    
    for (uint32_t year = 1970; year < date->year; year++) {
        days += 365 + is_leap_year(year);
    }
    
    if (date->month > 1) {
        for (uint8_t i = 0; i < date->month - 1; i++) {
            days += days_in_month[i];
        }
    }
    days += date->day - 1;
    
    if (is_leap_year(date->year)) {
        if ((date->month > 2) || ((date->month == 2) && (date->month >= 29))) {
            days++;
        }
    }
    
    timestamp = days * (60*60*24);
#if DEBUG   
    printf("%d days timestamp: %15lld\n", days, timestamp);
#endif  
    return timestamp;
}

uint32_t days_between(struct date_yyyymmdd_t *date_begin, struct date_yyyymmdd_t *date_end) {
    uint32_t days = 0;

    if (is_valid_date(date_begin) == 0) {
        printf("error: date_begin is invalid\n");
        return 0;
    } else if (is_valid_date(date_end) == 0) {
        printf("error: date_end is invalid\n");
        return 0;
    }
    
    days = 1 + (get_timestamp(date_end) - get_timestamp(date_begin) + 1) / (60*60*24);
#if DEBUG   
    printf("days: %d\n", days);
#endif  
    return days;
}

int8_t parse_date(const char *str, struct date_yyyymmdd_t *date) {
    if (sscanf(str, "%u-%u-%u", &(date->year), &(date->month), &(date->day)) != 3) {
        printf("error parsing date. the correct format is: yyyy-mm-dd e.g. 2021-1-01\n");
        return 0;
    }
    
    return 1;
}