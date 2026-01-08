/*
 * timezones.h - Global Timezone Definitions
 * 
 * Contains POSIX timezone strings for 88 global cities
 * Used for NTP time synchronization with automatic DST handling
 * 
 * To add a new timezone:
 * 1. Find the POSIX TZ string for your location
 * 2. Add entry to timezones[] array: {"City, Country", "TZ_STRING"}
 * 3. Increment will be automatic via sizeof calculation
 * 
 * POSIX TZ String Format:
 * STDoffset[DST[offset],start[/time],end[/time]]
 * 
 * Example: "AEST-10AEDT,M10.1.0,M4.1.0/3"
 * - AEST = Standard time name
 * - -10 = Offset from UTC (note: sign is reversed)
 * - AEDT = Daylight saving time name
 * - M10.1.0 = DST starts month 10, week 1, Sunday
 * - M4.1.0/3 = DST ends month 4, week 1, Sunday at 3am
 */

#ifndef TIMEZONES_H
#define TIMEZONES_H

// ======================== TIMEZONE CONFIGURATION ========================
struct TimezoneInfo {
  const char* name;
  const char* tzString;
};

// Expanded timezone array with 88 global timezones (organized by region)
// Default timezone is index 0 (Sydney, Australia)
const TimezoneInfo timezones[] = {
  // ==================== AUSTRALIA & OCEANIA (0-11) ====================
  {"Sydney, Australia", "AEST-10AEDT,M10.1.0,M4.1.0/3"},  // INDEX 0 - DEFAULT
  {"Adelaide, Australia", "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
  {"Brisbane, Australia", "AEST-10"},
  {"Darwin, Australia", "ACST-9:30"},
  {"Hobart, Australia", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"Melbourne, Australia", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"Perth, Australia", "AWST-8"},
  {"Auckland, New Zealand", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
  {"Wellington, New Zealand", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
  {"Fiji", "FJT-12FJST,M11.1.0,M1.3.0/3"},
  {"Noumea, New Caledonia", "NCT-11"},
  {"Port Moresby, Papua New Guinea", "PGT-10"},

  // ==================== NORTH AMERICA (12-22) ====================
  {"Anchorage, USA", "AKST9AKDT,M3.2.0,M11.1.0"},
  {"Chicago, USA", "CST6CDT,M3.2.0,M11.1.0"},
  {"Denver, USA", "MST7MDT,M3.2.0,M11.1.0"},
  {"Honolulu, USA", "HST10"},
  {"Los Angeles, USA", "PST8PDT,M3.2.0,M11.1.0"},
  {"New York, USA", "EST5EDT,M3.2.0,M11.1.0"},
  {"Phoenix, USA", "MST7"},
  {"Montreal, Canada", "EST5EDT,M3.2.0,M11.1.0"},
  {"Toronto, Canada", "EST5EDT,M3.2.0,M11.1.0"},
  {"Vancouver, Canada", "PST8PDT,M3.2.0,M11.1.0"},
  {"Mexico City, Mexico", "CST6CDT,M4.1.0,M10.5.0"},

  // ==================== SOUTH AMERICA (23-28) ====================
  {"Bogota, Colombia", "COT5"},
  {"Buenos Aires, Argentina", "ART3"},
  {"Caracas, Venezuela", "VET4:30"},
  {"Lima, Peru", "PET5"},
  {"Santiago, Chile", "CLT4CLST,M8.2.6/24,M5.2.6/24"},
  {"Sao Paulo, Brazil", "BRT3BRST,M10.3.0/0,M2.3.0/0"},

  // ==================== WESTERN EUROPE (29-39) ====================
  {"Amsterdam, Netherlands", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Berlin, Germany", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Brussels, Belgium", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Dublin, Ireland", "IST-1GMT0,M10.5.0,M3.5.0/1"},
  {"Lisbon, Portugal", "WET0WEST,M3.5.0/1,M10.5.0"},
  {"London, UK", "GMT0BST,M3.5.0/1,M10.5.0"},
  {"Madrid, Spain", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Paris, France", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Reykjavik, Iceland", "GMT0"},
  {"Rome, Italy", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Vienna, Austria", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Zurich, Switzerland", "CET-1CEST,M3.5.0,M10.5.0/3"},

  // ==================== NORTHERN EUROPE (40-43) ====================
  {"Copenhagen, Denmark", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Helsinki, Finland", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Oslo, Norway", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Stockholm, Sweden", "CET-1CEST,M3.5.0,M10.5.0/3"},

  // ==================== CENTRAL & EASTERN EUROPE (44-51) ====================
  {"Athens, Greece", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Bucharest, Romania", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Budapest, Hungary", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Kiev, Ukraine", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Minsk, Belarus", "MSK-3"},
  {"Moscow, Russia", "MSK-3"},
  {"Prague, Czech Republic", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Warsaw, Poland", "CET-1CEST,M3.5.0,M10.5.0/3"},

  // ==================== MIDDLE EAST (52-56) ====================
  {"Dubai, UAE", "GST-4"},
  {"Istanbul, Turkey", "TRT-3"},
  {"Riyadh, Saudi Arabia", "AST-3"},
  {"Tehran, Iran", "IRST-3:30IRDT,J79/24,J263/24"},
  {"Tel Aviv, Israel", "IST-2IDT,M3.4.4/26,M10.5.0"},

  // ==================== SOUTH ASIA (57-63) ====================
  {"Colombo, Sri Lanka", "IST-5:30"},
  {"Dhaka, Bangladesh", "BST-6"},
  {"Kabul, Afghanistan", "AFT-4:30"},
  {"Karachi, Pakistan", "PKT-5"},
  {"Kathmandu, Nepal", "NPT-5:45"},
  {"Mumbai, India", "IST-5:30"},
  {"Thimphu, Bhutan", "BTT-6"},

  // ==================== SOUTHEAST ASIA (64-70) ====================
  {"Bangkok, Thailand", "ICT-7"},
  {"Ho Chi Minh, Vietnam", "ICT-7"},
  {"Jakarta, Indonesia", "WIB-7"},
  {"Kuala Lumpur, Malaysia", "MYT-8"},
  {"Manila, Philippines", "PHT-8"},
  {"Singapore", "SGT-8"},
  {"Yangon, Myanmar", "MMT-6:30"},

  // ==================== EAST ASIA (71-76) ====================
  {"Hong Kong", "HKT-8"},
  {"Seoul, South Korea", "KST-9"},
  {"Shanghai, China", "CST-8"},
  {"Taipei, Taiwan", "CST-8"},
  {"Tokyo, Japan", "JST-9"},
  {"Ulaanbaatar, Mongolia", "ULAT-8"},

  // ==================== CENTRAL ASIA (77-79) ====================
  {"Almaty, Kazakhstan", "ALMT-6"},
  {"Bishkek, Kyrgyzstan", "KGT-6"},
  {"Tashkent, Uzbekistan", "UZT-5"},

  // ==================== CAUCASUS (80-82) ====================
  {"Baku, Azerbaijan", "AZT-4"},
  {"Tbilisi, Georgia", "GET-4"},
  {"Yerevan, Armenia", "AMT-4"},

  // ==================== AFRICA (83-86) ====================
  {"Cairo, Egypt", "EET-2"},
  {"Johannesburg, South Africa", "SAST-2"},
  {"Lagos, Nigeria", "WAT-1"},
  {"Nairobi, Kenya", "EAT-3"}
};

// Calculate number of timezones automatically
const int numTimezones = sizeof(timezones) / sizeof(timezones[0]);

#endif // TIMEZONES_H
