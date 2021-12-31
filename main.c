/*
    Author: Kari Kuivalainen, 2021

    Purpose: This software offers solutions to the Vincit Recruitment challenge of 2021.
    
             It takes a crypto currency name and yyyy-mm-dd dates as arguments and GETs a json file from goingecko
             that contains price, trading volume and market cap data for the given date range in
             daily, hourly or 5 minute data periods depending on the date range.
             
            The data is further processed from the JSON file into arrays containing one value per day
                regardless of source resolution.
             
    Exercise A: Output longest downward trend in days.
    Exercise B: Output day with highest trading volume.
    Exercise C: Output pair of days when to buy and when to sell for maximum profit
                or indicate there was no opportunity if the price only went down.

    Dependencies: curl library, json library (provided)
                  Install curl library using one of the following. The gnutls version is probably ok.
                    apt-get install libcurl4-gnutls-dev
                    apt-get install libcurl4-openssl-dev
                    apt-get install libcurl4-nss-dev
                  
    Compiling:  gcc -Wall -lm -lcurl timedate.c curl_helpers.c json.c main.c -o moneymaker
    
    Running: ./moneymaker [coin] [date_begin] [date_end]
            e.g. ./moneymaker monero 2021-01-01 2021-06-30

    Copyright: main.c and timedate.c/.h are released to the public domain in so far as they can be
               Adapted maybe a dozen lines from a curl library sample code (MIT license?)
               Using the json library (BSD license)
               
    Additional information:
               Daily data for at least 2015-01-28 is missing and any query that includes it gets weird.
                At least it fails gracefully but the data / date is off by one and wasn't fixed this late to the deadline (2021-12-31)
                The proper way would probably be to calculate the dates from timestamps of the data instead of the array index
                And also should verify daily data and their timestamps that they land on the same day
                Or skip over the entry in the array and add checks for exercises that skip empty days..

 */
 
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "json.h"
#include "curl_helpers.h"
#include "timedate.h"

/* uncomment to enable debug printing */
/* #define DEBUG 1 */

struct pair_t {
    double buy_price;
    double sell_price;
    uint8_t buy_date;
    uint8_t sell_date;
};

struct data_t { 
    int64_t begin_timestamp;
    int64_t end_timestamp;
    struct date_yyyymmdd_t date_begin;
    struct date_yyyymmdd_t date_end;
    uint16_t num_entries;
    int64_t *timestamp;
    double *price;
    double *volume;
    double *market_cap;
};

int8_t exercise_a (struct data_t *data) {
    /*
     * Exercise A: calculate longest down trend for the given date range
     * Expected output: The maximum amount of days bitcoin’s price was decreasing in a row.
     */
    uint16_t days = 0;
    uint16_t max_days = 0;
    uint16_t max_start = 0;
    uint16_t max_stop = 0;
    uint16_t start = 0;
    
    struct date_yyyymmdd_t date_start;
    struct date_yyyymmdd_t date_stop;
    
    for (uint16_t i = 0; i < data->num_entries - 1; i++) {
        if (data->price[i + 1] < data->price[i]) {
            days++;
        } else {
            if (days > max_days) {
                max_days = days;
                max_stop = i;
                max_start = start;
            }
            days = 0;
            start = i + 1;
        }
    }
    
#if DEBUG
    printf("start: %d\tstop: %d\tdays: %d\n", max_start, max_stop, max_days);
#endif

    add_days_to_date(&(data->date_begin), &date_start, max_start);
    add_days_to_date(&(data->date_begin), &date_stop, max_stop);
    printf("Longest bear trend of %d days between %04d-%02d-%02d and %04d-%02d-%02d\n",
           max_days, date_start.year, date_start.month, date_start.day, date_stop.year, date_stop.month, date_stop.day);

    return 0;
}

int8_t exercise_b (struct data_t *data) {

    /*
     * Exercise B: find the max of "total_volumes"
     * Expected output: The date with the highest trading volume and the volume on that day in euros.
     */
     
    struct date_yyyymmdd_t date;
     
    double highest_volume = data->price[0];
    uint16_t day = 0;
    
    for (uint16_t i = 0; i < data->num_entries - 1; i++) {
        if (data->volume[i] > highest_volume) {
            day = i;
            highest_volume = data->volume[i];
        }
    }

#if DEBUG
    printf("day: %d\tvolume: %.4f\n", day, highest_volume);
#endif

    add_days_to_date(&(data->date_begin), &date, day);
    printf("Highest trading volume %f on %04d-%02d-%02d\n",
           highest_volume, date.year, date.month, date.day);

    return 0;
}

int8_t exercise_c (struct data_t *data, uint32_t principal) {
    /* 
     * Exercise C: find the biggest price difference where date_price_min precedes date_price_max
     * Expected output: A pair of days: The day to buy and the day to sell.
     *  In the case when one should neither buy nor sell, return an indicative output of your choice.
     *
     */
     
    uint16_t range = data->num_entries;
    double *price = data->price;

    struct pair_t trade;
    trade.sell_price = 0;
    trade.sell_date = 0;
    trade.buy_price = 0;
    trade.buy_date = 0;
    
    struct pair_t pair[2];
    pair[0].buy_date = 0;
    pair[0].buy_price = price[0];
    pair[0].sell_price = 0;
    pair[0].sell_date = 0;
    uint8_t num_pairs = 1;
    
    double price_min = price[0];
    double price_max = price[0];    

    pair[1].buy_date = 0;
    pair[1].buy_price = 0;  
    pair[1].sell_date = 0;
    pair[1].sell_price = 0;
    
    struct date_yyyymmdd_t date_buy;
    struct date_yyyymmdd_t date_sell;
    
#if DEBUG
    uint8_t something_was_done = 0;
#endif

    /*
     * Start from beginning and set the first price as minimum and start comparing with next day's data
     *  replace minimum with new minimum if found before a max
     * If new global minimum is found, start second comparison pair
     * If new global max is found after there are 2 pairs, eliminate 1st pair, move 2nd to 1st.
     */

    for (uint16_t i = 0; i < range; i++) {
#if DEBUG       
        printf ("\n\n-----Day: %d\n\n", i);
        something_was_done = 0;
#endif      
        if (price[i] > price_max) {
            price_max = price[i];
#if DEBUG           
            printf("New global maximum at %.4f on %d\n", price_max, i);
#endif          
            /* if a new max is found then the minimum-maximum pair wins */
            if (num_pairs > 1) {
#if DEBUG               
                printf("\nRemoving pair[0]\tbuy date: %d\tsell date: %d\tdifference: %.2f\nreturn on investment: %.2f (%.2f pct)\n\n",
                           pair[0].buy_date,
                           pair[0].sell_date,
                           (pair[0].buy_price - pair[0].sell_price),
                           ((principal / pair[0].buy_price) * (pair[0].sell_price - pair[0].buy_price)),
                           (((principal / pair[0].buy_price) * (pair[0].sell_price - pair[0].buy_price)) / principal) * 100);
#endif                         
                    pair[0].buy_date = pair[1].buy_date;
                    pair[0].buy_price = pair[1].buy_price;
                    pair[0].sell_price = pair[1].sell_price;
                    pair[0].sell_date = pair[1].sell_date;
                    num_pairs = 1;              
                pair[0] = pair[1];
                num_pairs = 1;
            }
            pair[0].sell_date = i;
            pair[0].sell_price = price[i];
            
#if DEBUG           
            printf("Adjusting winning pair's sell price to %.4f (%d)\n",
                   pair[0].sell_price, pair[0].sell_date);          
            something_was_done = 1;
#endif                 

            
        } else if (price[i] < price_min) {
            price_min = price[i];
            
#if DEBUG           
            printf("New global minimum at %.4f on %d\n", price_min, i);
            something_was_done = 1;
#endif                  

            
            if (pair[0].sell_price == 0) {
                pair[0].buy_date = i;
                pair[0].buy_price = price[i];
            } else {
                num_pairs = 2;
                pair[1].buy_date = i;
                pair[1].buy_price = price[i];
                pair[1].sell_date = 0;
                pair[1].sell_price = 0;
            }
#if DEBUG           
            printf("Adjusting latest pair's buy price to %.4f (%d)\n",
                           pair[num_pairs - 1].buy_price, pair[num_pairs - 1].buy_date);
#endif
        }
        
        if (num_pairs > 1) {
                if ((pair[1].sell_price < price[i]) && (pair[1].sell_price > 0)) {
                    pair[1].sell_price = price[i];
                    pair[1].sell_date = i;
#if DEBUG                   
                    printf("Adjusting pair[1] sell price to %.4f (%d)\n",
                               pair[1].sell_price, pair[1].sell_date);                  
                    something_was_done = 1;
#endif
                } 
                
                /* remove old pair if new is better */
                if (((pair[1].sell_price - pair[1].buy_price) - (pair[0].sell_price - pair[0].buy_price)) > 0) {
#if DEBUG                   
                    printf("\nRemoving pair[0]\tbuy date: %d\tsell date: %d\tdifference: %.2f\nreturn on investment: %.2f (%.2f pct)\n\n",
                           pair[0].buy_date,
                           pair[0].sell_date,
                           (pair[0].buy_price - pair[0].sell_price),
                           ((principal / pair[0].buy_price) * (pair[0].sell_price - pair[0].buy_price)),
                           (((principal / pair[0].buy_price) * (pair[0].sell_price - pair[0].buy_price)) / principal) * 100);
                    something_was_done = 1;
#endif
                    pair[0].buy_date = pair[1].buy_date;
                    pair[0].buy_price = pair[1].buy_price;
                    pair[0].sell_price = pair[1].sell_price;
                    pair[0].sell_date = pair[1].sell_date;
                    num_pairs = 1;
                }
            }       

#if DEBUG
        if (something_was_done == 1) {
            for (uint8_t p = 0; p < num_pairs; p++) {
                printf("\npair %d\tbuy date: %d\tsell date: %d\tdifference: %.2f\nreturn on investment: %.2f (%.2f pct)\n\n",
                    p, pair[p].buy_date, pair[p].sell_date,
                    (pair[p].buy_price - pair[p].sell_price),
                    ((principal / pair[p].buy_price) * (pair[p].sell_price - pair[p].buy_price)),
                    (((principal / pair[p].buy_price) * (pair[p].sell_price - pair[p].buy_price)) / principal) * 100);
            }
        }
#endif                  

    }
    
    trade.sell_price = pair[0].sell_price;
    trade.buy_price = pair[0].buy_price;
    trade.sell_date = pair[0].sell_date;
    trade.buy_date = pair[0].buy_date;
/* TODO: convert day index to yyyy-mm-dd */
    if (trade.sell_price > 0) {
#if DEBUG
        printf("buy date: %d\tsell date: %d\tdifference: %.2f\nreturn on investment: %.2f pct\n\n",
               trade.buy_date, trade.sell_date,
               (trade.sell_price - trade.buy_price),
               (((principal / trade.buy_price) * (trade.sell_price - trade.buy_price)) / principal) * 100);
#endif
        
        add_days_to_date(&(data->date_begin), &date_buy, trade.buy_date);
        add_days_to_date(&(data->date_begin), &date_sell, trade.sell_date);
        
        printf("Dates for the best deal at %.2f pct ROI (diff: %.2f)\n            Buy on: %04d-%02d-%02d\tSell on: %04d-%02d-%02d\n\n",
               (((principal / trade.buy_price) * (trade.sell_price - trade.buy_price)) / principal) * 100,
               (trade.sell_price - trade.buy_price),
               date_buy.year, date_buy.month, date_buy.day,
               date_sell.year, date_sell.month, date_sell.day);
    } else {
        printf("No opportunity for hodling but consider shorting if you're not afraid of margin calls!\n");
    }
     
    return 0;
}

/* get data from json into arrays by matching hardcoded object identifiers */
/* for non daily data, finds the closest timestamp to midnight */
/* could increase resolution by getting data with finer granularity for the intended range with multiple <=90 day queries */
int process_json_data (struct data_t *data, json_value *value, uint8_t not_daily_data) {
    json_object_entry object;
    json_value *array;
    
    uint32_t length = value->u.object.length;
    int64_t *timestamp = data->timestamp;
    double *saved_data;
    
    uint32_t array_length = 0;
    uint32_t day;
    
    /* when comparing timestamps to midnight */
    uint32_t dist_prev;
    uint32_t dist_cur;
    int64_t timestamp_cur;
    int64_t timestamp_prev;
    int64_t timestamp_midnight;
    
    /*
     * 0: Off
     * 1: use the last data of the previous day if its timestamp is closer to midnight
     */
    uint8_t autism = 0;
    
    for (uint8_t i = 0; i < length; i++) {
        object = value->u.object.values[i];
        array_length = object.value->u.array.length;
#if DEBUG       
        printf("Object %d: %s\n", i, object.name);
        printf("array length: %d\n", array_length);
#endif
        if (strcmp(value->u.object.values[i].name, "prices") == 0) {
            saved_data = data->price;
        } else if (strcmp(value->u.object.values[i].name, "market_caps") == 0) {
            saved_data = data->market_cap;
        } else if (strcmp(value->u.object.values[i].name, "total_volumes") == 0) {
            saved_data = data->volume;
        } else {
#if DEBUG           
            printf("all required objects parsed!");
#endif
            return 1;
        }

        /* if hourly or 5 minute data, find the entry whose timestamp is closest to midnight */
        if (not_daily_data) {
#if DEBUG           
            printf("Not daily data...\n");
#endif          
            day = 0;
            array = object.value->u.array.values[0];
            timestamp[day] = (int64_t) array->u.array.values[0]->u.integer / 1000;;
            saved_data[day] = (double) array->u.array.values[1]->u.dbl;
            timestamp_midnight = get_timestamp(&(data->date_begin)) + day * (60*60*24);
#if DEBUG           
            printf("day: %03d: %15lld: timestamp[%d]: %15lld (%15lld)\n",
                   day, timestamp_midnight-timestamp[day], 0, timestamp[day], timestamp_midnight);
#endif
            for (uint32_t j = 1; j < array_length; j++) {
                if (day == (data->num_entries - 1)) {
#if DEBUG                   
                    printf("got all data. break\n");
#endif                  
                    break;
                }
                
                array = object.value->u.array.values[j];
                timestamp_cur = (int64_t) array->u.array.values[0]->u.integer / 1000;
                timestamp_midnight = get_timestamp(&(data->date_begin)) + (day + 1) * (60*60*24);
#if DEBUG               
                printf("day: %03d: %15lld: timestamp[%d]: %15lld (%15lld)\n",
                       day, timestamp_midnight-timestamp_cur, j, timestamp_cur, timestamp_midnight);
#endif              
                /* if found the first timestamp for the next day or the last in the array... */
                if ((timestamp_cur >= timestamp_midnight) || (j == (array_length - 1))) {
                    /* if feeling pedantic then could check the previous entry here if it's closer and use that becase
                    *   11:59 is closer to midnight than 12:02 unless meant "closest time to midnight on the same day :D"
                    */
#if DEBUG
                    printf("next day\n");
#endif                  
                    day++;
                    
                    dist_cur = timestamp_cur - timestamp_midnight;
                    
                    array = object.value->u.array.values[j - 1];
                    timestamp_prev = (int64_t) array->u.array.values[0]->u.integer / 1000;
                    dist_prev = timestamp_midnight - timestamp_prev;
                    
                    if ((dist_prev < dist_cur) && (autism == 1)) {
                        timestamp[day] = timestamp_prev;
#if DEBUG                       
                        printf("using yesterday's timestamp %15lld\n", timestamp_prev);
#endif                      
                    } else {
                        timestamp[day] = timestamp_cur;
#if DEBUG
                        printf("using today's timestamp %15lld\n", timestamp_cur);
#endif
                        array = object.value->u.array.values[j];
                    }
                    
                    saved_data[day] = (double) array->u.array.values[1]->u.dbl;
                }
#if DEBUG                   
                printf("timestamp: %15lld\n", timestamp[day]);
                printf("value: %f\n", saved_data[day]);
#endif
            }
                
        } else {
            /* daily data, trust that it's consistent and just copy 1:1 */
            for (uint32_t j = 0; j < array_length; j++) {
                    array = object.value->u.array.values[j]; /* the array storing timestamp & data */
                    timestamp[j] = (int64_t) array->u.array.values[0]->u.integer / 1000;
                    saved_data[j] = (double) array->u.array.values[1]->u.dbl;
#if DEBUG                   
                    printf("timestamp %03d: %15lld\n", j, timestamp[j]);
                    printf("value %03d: %f\n", j, saved_data[j]);
#endif                  
                    
            }
        }
    }
    
    day = array_length - 1;
#if DEBUG
    printf("recv: %d expected: %d\n", day, data->num_entries - 1);
#endif  
    if (day < (data->num_entries - 1)) {
        printf("warning: didn't receive enough data. recv: %d expected: %d\n", day, data->num_entries - 1);
        data->num_entries = day + 1;
    }
    
    return 0;
}

int main (int argc, char *argv[]) {
    uint32_t principal = 1000;
    uint8_t not_daily_data = 1;
    
    struct data_t data;

    struct MemoryStruct chunk;
    char *req;

    req = malloc(sizeof(char) * 111);
    if (req == NULL) {
        printf("error: malloc req\n");
        return -1;
    }
    
#if DEBUG   
    for (uint8_t arg = 0; arg < argc; arg++) {
        printf("%s ", argv[arg]);
    }
    printf("\n");
#endif
    
    if (argc >= 4) {
        /* first argument is coin_name */
        printf("coin: %s\n", argv[1]);

        /* second is begin date, third is end date */
        /* parse dates from strings */
        if (parse_date(argv[2], &(data.date_begin)) == 0) {
            printf("error: invalid date format\n");
            return 1;
        }
        if (parse_date(argv[3], &(data.date_end)) == 0) {
            printf("error: invalid date format\n");
            return 1;
        }
        
        if (0 == is_valid_date(&data.date_begin)) {
            printf("error: invalid begin date\n");
            return 1;
        }
        
        if (0 == is_valid_date(&data.date_end)) {
            printf("error: invalid end date\n");
            return 1;
        }       
        
        /* get unix timestamps and add 1 hour to end time to make sure the last day's data is included */
        data.begin_timestamp = get_timestamp(&(data.date_begin));
        data.end_timestamp = get_timestamp(&(data.date_end)) + 6 * (60*60);
    
        data.num_entries = days_between(&(data.date_begin), &(data.date_end));
        
        printf("begin date: %s (%lld)\n", argv[2], data.begin_timestamp);
        printf("end date: %s (%lld)\n", argv[3], data.end_timestamp);
        printf("days: %d\n", data.num_entries);

        data.timestamp = malloc(sizeof(int64_t) * data.num_entries);
        data.price = malloc(sizeof(double) * data.num_entries);
        data.volume = malloc(sizeof(double) * data.num_entries);
        data.market_cap = malloc(sizeof(double) * data.num_entries);
    
        /* optional fourth argument is the amount of money to use for exercise C. defaults to something */
        if (argc == 5) {
            principal = atoi(argv[4]);
        }
    } else {
        printf("error: invalid number of arguments\nusage: %s [coin_name] [from] [to]\ne.g.: %s monero 2021-09-16 2021-11-01\n",
                argv[0], argv[0]);
        return 1;
    }
    
    /* Get json file */
    sprintf(req, "https://api.coingecko.com/api/v3/coins/%s/market_chart/range?vs_currency=eur&from=%" PRIu64 "&to=%" PRIu64, 
                            argv[1], data.begin_timestamp, data.end_timestamp);

    printf("req: %s\n", req);
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    request(req, &chunk);
    
    if (chunk.size < 100) {
        printf("error: invalid response or no data\n");
        return 0;
    }
    
#if DEBUG   
    printf("response size: %d\n", chunk.size);
    
    for (uint32_t i = 0; i < chunk.size; i++) {
        printf("%c", chunk.memory[i]);
    }
#endif  

    /* Prints and parses the json file */
    uint32_t file_size;
    char *file_contents;
    json_char *json;
    json_value *value;

    file_size = chunk.size;
    file_contents = &chunk.memory[0];
#if DEBUG
    printf("%s\n", file_contents);
    printf("--------------------------------\n\n");
#endif

    json = (json_char *)file_contents;

    value = json_parse(json, file_size);

    if (value == NULL) {
            fprintf(stderr, "Unable to parse data\n");
            free(file_contents);
            exit(1);
    }
    
    /* Calculate whether not_daily_data based on something
        a) response size
            ~28000 / 2 = 14 000 bytes per day for 2 days of 5 min data
            ~2400 bytes per day for hourly data
            ~100 bytes for daily data
        b) array size: >280 for 2 days at 5 min, >20 / day for hourly, <2 / day for daily
        c) checking if days >= 90 else if = <90 else if ((day_begin = yesterday) && (day_end = today))
    */  
    if ((file_size / data.num_entries) < 200) {
        /* daily data */
        not_daily_data = 0;
        printf("data is in daily format\n");
    } else if ((file_size / data.num_entries) < 3000) {
        /* hourly data */
        not_daily_data = 1;
        printf("data is in hourly format\n");
    } else {
        /* 5 minute data */
        not_daily_data = 1;
        printf("data is in 5 min format\n");
    }
#if DEBUG   
    printf("size per day: %u not_daily_data: %d\n", (uint32_t) (file_size / data.num_entries), not_daily_data);
#endif

    /* load entries from json into arrays */
    process_json_data(&data, value, not_daily_data);
    
#if DEBUG   
    printf("data processed\n");
    printf("data:\n");
    for (uint16_t i = 0; i < data.num_entries; i++) {
        printf("%03d: %15lld\t%.4f\t%f\t%f\n", i, data.timestamp[i], data.price[i], data.volume[i], data.market_cap[i]);
    }
    
    printf("\n");
#endif

    /* exercises */
    printf("\nExercise A: ");
    exercise_a(&data);
    printf("\n");
    
    printf("Exercise B: ");
    exercise_b(&data);
    printf("\n");
    
    printf("Exercise C: ");
    exercise_c (&data, principal);
    printf("\n");
    
    json_value_free(value);
    free(file_contents);
    
    free(data.timestamp);
    free(data.price);
    free(data.volume);
    free(data.market_cap);
    free(req);
    
    return 0;
}
