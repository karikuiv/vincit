struct date_yyyymmdd_t {
  uint32_t year;
  uint32_t month;
  uint32_t day;
};

int8_t add_days_to_date(struct date_yyyymmdd_t *date, struct date_yyyymmdd_t *output, uint16_t add_days);
uint8_t is_leap_year(int year);
uint8_t is_valid_date(struct date_yyyymmdd_t *date);
int64_t get_timestamp (struct date_yyyymmdd_t *date);
uint32_t days_between(struct date_yyyymmdd_t *date_begin, struct date_yyyymmdd_t *date_end);
int8_t parse_date(const char *str, struct date_yyyymmdd_t *date);