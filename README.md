# vincit

    Author: Kari Kuivalainen, 2021

    Purpose: This software offers solutions to the Vincit Recruitment challenge of 2021.
    
             It takes a crypto currency name and yyyy-mm-dd dates as arguments and GETs a json file from coingecko
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
