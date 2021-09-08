// Example of simple alarm clock
// Configuration
// * 1 alarm time
// * alarm activation setting for each day of the week
// * Digital output buzz_pin is activated during alarm
// * time compensation of slow up or quick up the internal arduino clock
// * menu control by rotary encoder input (or 4 buttons via 4 binary inputs or 1 analog input)
// * display on 16x2 LCD display (or 20x4) using LiquidCrystal_I2C.h library

#include <Time.h>               //time library used for handling with datetime variables type
#include <TimeLib.h>            //time library used for handling with datetime variables type
#include <Wire.h>               //I2C communication library 
#include <LiquidCrystal_I2C.h>  //LCD display library

// LiquidCrystal_I2C setup
// Define new instance (lcd) of LiquidCrystal_I2C class
// LiquidCrystal_I2C <instance name>(<I2C address>, <display columns>, <display rows>)

#define display_columns 16
#define display_rows 2

// LiquidCrystal_I2C setup
// Define new instance (lcd) of LiquidCrystal_I2C class
// LiquidCrystal_I2C <instance name>(<I2C address>, <display columns>, <display rows>)

LiquidCrystal_I2C lcd(0x3f,display_columns,display_rows);

#include "CMenu_I2C.h" //Read the Menu library

//Texts for menu dialog like on/off, open/close for whole menu
const String On_label  = "ON ";       //set on/off labels string for menu dialog (not used in this example)
const String Off_label = "OFF";       //set on/off labels string for menu dialog (not used in this example)
const String New_val   = "Enter:";    //set input label string for menu dialog

static const byte bell[8] = {
  B00100, //  *   
  B01110, // ***
  B01110, // ***
  B01110, // *** 
  B11111, //*****
  B00000, //  
  B00100, //  *    
  B00000  //      
};

//Texts of individual menu items (variable names and programs/rutines names). Texts are stored in the program memory to spare space in the local memory
const char t00[] PROGMEM = "Date and time";        
const char t01[] PROGMEM = "Alarm";      
const char t02[] PROGMEM = "Date";       
const char t03[] PROGMEM = "Time";      
const char t04[] PROGMEM = "TimeErrComp";
const char t05[] PROGMEM = "AlmActive";      
const char t06[] PROGMEM = "AlmTime";    
const char t07[] PROGMEM = "Monday";   
const char t08[] PROGMEM = "Tuesday";   
const char t09[] PROGMEM = "Wednesday";
const char t10[] PROGMEM = "Thursday";
const char t11[] PROGMEM = "Friday";   
const char t12[] PROGMEM = "Saturday";
const char t13[] PROGMEM = "Sunday";
const char t14[] PROGMEM = "Next alarm";
const char t15[] PROGMEM = "AlmDate";
const char t16[] PROGMEM = "AlmTime";

//Construction of the menu texts array in the program memory.
const char* const tt[] PROGMEM = {t00, t01, t02, t03, t04, t05, t06, t07, t08, t09, t10, t11, t12, t13, t14, t15, t16};

Menu MyMenu; //Define new instance MyMenu class

//User program variables declaration
byte up_pin = 2; //navigation Up pin
byte down_pin = 3; //navigation Down pin
byte forward_pin = 4; //navigation Forward pin
byte backward_pin = 4; //navigation Backward pin
byte analog_nav_pin = 0; //navigation Backward pin
byte buzz_pin = 11; //Alarm pin
bool play = 0; //alarm pulsation
unsigned long alarm_time = 0; //00:00:00 (number of seconds since 00:00:00 1.1.1970)
unsigned long next_start = 0; //Alarm start time - time stamp
unsigned long midnight;       //Midnight - time stamp

bool alarm_active = 0; //alarm is running (activated)
bool alarm_on = 0; //alarm time is set
bool mo = 0; //alarms are activated on Monday
bool tu = 0; //alarms are activated on Tuesday
bool we = 0; //alarms are activated on Wednesday
bool th = 0; //alarms are activated on Thursday
bool fr = 0; //alarms are activated on Friday
bool sa = 0; //alarms are activated on Saturday
bool su = 0; //alarms are activated on Sunday
bool valid = 0; //boolean auxiliary variable
byte b = 0;
unsigned long actual_time_reference = 0; // Could be set from the menu (number of seconds since 00:00:00 1.1.1970)
unsigned long alarm_endtime = 0; //alarm end - time stamp
int alarm_duration = 60; //alarm duration in seconds

//value in seconds for compensation of slow up or quick up the internal arduino clock (for example value should be 30 when internal clock during 24 hours slow up 30 seconds)
bool time_shift = 0;
int daily_time_shift = 0;
unsigned long time_shifts = 0;
unsigned long time_shift_fraction = 0;

unsigned long tm = 0; //time calculaion auxiliary variable
String sval; //string auxiliary variable

void setup() {

  //LCD initialization
  lcd.init();
  lcd.backlight();
  
  //I/O setup of the User IO global variables
  pinMode(up_pin, INPUT);   // pin 2 is used for buttons/ro tary encoder input
  pinMode(down_pin, INPUT);   // pin 3 is used for buttons/rotary encoder input
  pinMode(forward_pin, INPUT);   // pin 4 is used for buttons/rotary encoder input
  pinMode(buzz_pin, OUTPUT); // pin 11 is used for buzzer

  //-------------------------------------
  //Menu initialization (CMenu.h library)
  //Initialize menu (LCD_object, PROGMEM_text_table,  refresh_time_[ms], sleep_time [s], off_time[min] (255=off), display_columns, display_rows, date_DMY [1: date format DD.MM.YY, 0: date format MM.DD.YY, default is DD.MM.YY, max_events (0..50)])
        MyMenu.begin(       lcd,                 tt,                500,             30,                       5, display_columns, display_rows,                1                                                                       , 0          );
        
  lcd.createChar(5, (uint8_t*) bell); //Add bell icon to the LCD. Chars 1..4 are used for menu library

  //Menu input settings
  //void key_input(bool _analog_input, byte _pin, word _up, word _down, word _left, word _right)
  // * _analog_input - Analog or digital input type. 
  
  //                 - Analog type: Arrays of input switches distinguish which switch was pressed by different resistors connected to each switch
  //                 - Parameters _up, _down, _left, _right represents value on analog input pin when you press corresponding key [0-1024]
  // * _pin          - For analog input type its analog pin number used for the input.

  //                 - Digital type: 4 press buttons or rotary encoder
  //                 - 4 press buttons: parameters _up, _down, _left, _right represents digital pin number used for reading corresponding key
  //                 - rotary encoder : _up: digital pin number of CLK, _down: digital pin number of DT, _left: digital pin number of SW, _right: digital pin number of SW
  //                 - rotary encoder : move to left(back) is realized by doubleclick, move to right(front) is realized by click
  
  
  //Examples of the menu input settings:
  //       key_input(analog_input,              pin,     up,     down,         left,       right)
  //MyMenu.key_input(           1,   analog_nav_pin,     40,      160,           10,         250); //analog input pin = analog_nav_pin, up, down, left, right represents value on analog input pin when you press corresponding key [0-1024]
  //MyMenu.key_input(           0,                0, up_pin, down_pin, backward_pin, forward_pin); //digital input pins 3 up, 4 down, 5 left, 2 right
    MyMenu.key_input(           0,                0, up_pin, down_pin, backward_pin, forward_pin); //rotary encoder pins 2 CLK, 3 DT, 4 SW

  //Creation of the new Menu item.
  //  Maximal number of menu items is 100.
  //  int item_id Menu::add_menu_item(int parent_id, int action_type, byte tt[], int variable_pointer (optional), byte var_type (optional));
  // * Parent_id/item_id is the identificator of the menu item for making multi level menus. You can name the item_id to make menu interlinks. Root level id is -1
  // * tt[] is index of the menu text stored in te program memory
  // * Variable_pointer is the pointer to the address of the memory where is stored a variable, which should be dynamically displayed in the menu
  // * Valid var_point_types are bool, byte, int, unsigned int, long, unsigned long, float, datetime
  // * var_type - specifies number of decimal places for float and double
  // * var_type - for datetime specify which part (date or time) you want to display (var_type 7 or 8)

  //int action_types
  //0 - menu
  //1 - menu + display value
  //2 - menu + set int to 1/0 - thick [v]/[ ]
  //3 - menu + set int to 1/0 - ON/OFF
  //4 - display events: 1st row - datetime, 2nd row - event description + int value
  //5 - set value

  //byte var_point_type;
  //0 - bool                 0 ... 1
  //1 - byte                 0 ... 255
  //2 - int            -32 768 ... 32 767
  //3 - word/unsigned int    0 ... 65535
  //4 - long    -2 147 483 648 ... 2 147 483 647
  //5 - unsigned long   0 ... 4 294 967 295
  //6 - float   -3.4028235E+38 ... 3.4028235E+38
  //7 - double
  //8 - datetime(unsigned long): date DD.MM.YY or MM.DD.YY (se parameter date_DMY). Unsigned long - number of seconds since 1.1.1970 00:00:00 (Time.h, TimeLib.h library)
  //9 - datetime(unsigned long): time hh:mm:ss. Unsigned long - number of seconds since 1.1.1970 00:00:00 (Time.h, TimeLib.h library)
  //10- datetime(unsigned long): date + millis();
  //11- datetime(unsigned long): time + millis();

  //Example: Making of the root menu items ("Sensors", "Settings", "Programs", "Commands" and "Records")
  //Example: menu_type 0 is the simple text menu
  //int item_id    Menu::add_menu_item( int parent_id,  int action_type, byte tt[], int variable_pointer (optional), byte var_type (optional));
  //                 add_menu_item(    root level, menu - only text, text array reference)
  byte act  = MyMenu.add_menu_item(            -1,                0,                    0);
  byte alm  = MyMenu.add_menu_item(            -1,                0,                    1);
  byte n_alm= MyMenu.add_menu_item(            -1,                0,                   14);

  //Example: "Actual date, time" submenu declaration
  //Example: variable Sensors contain index of the "Sensors" menu item declared above
  //                   add_menu_item(parent menu id, menu - menu + value, text array reference, pointer to the variable, variable type)
  byte act_dt = MyMenu.add_menu_item(           act,                   1,                    2,  &actual_time_reference,            10);
  byte act_tm = MyMenu.add_menu_item(           act,                   1,                    3,  &actual_time_reference,            11);
  byte day_sh = MyMenu.add_menu_item(           act,                   1,                    4,       &daily_time_shift               );

  //Example: "Alarm" submenu declaration
  //                     add_menu_item(parent menu id,           menu type, text array reference, pointer to the variable, variable type)
                  MyMenu.add_menu_item(           alm,                   2,                    5,           &alarm_active               );
  byte altm     = MyMenu.add_menu_item(           alm,                   1,                    6,             &alarm_time,             9);
                  MyMenu.add_menu_item(           alm,                   2,                    7,                     &mo               );
                  MyMenu.add_menu_item(           alm,                   2,                    8,                     &tu               );
                  MyMenu.add_menu_item(           alm,                   2,                    9,                     &we               );
                  MyMenu.add_menu_item(           alm,                   2,                   10,                     &th               );
                  MyMenu.add_menu_item(           alm,                   2,                   11,                     &fr               );
                  MyMenu.add_menu_item(           alm,                   2,                   12,                     &sa               );
                  MyMenu.add_menu_item(           alm,                   2,                   13,                     &su               );
                  
  //Example: "Next alarm" submenu declaration
  //Example: variable Sensors contain index of the "Sensors" menu item declared above
  //                   add_menu_item(parent menu id, menu type =  menu + value, text array reference, pointer to the variable, variable type)
  MyMenu.add_menu_item(         n_alm,                   1,                   15,             &next_start,             8);
  MyMenu.add_menu_item(         n_alm,                   1,                   16,             &next_start,             9);

  //Example: "Actual date, time" input submenu declaration
  //Example: menu_type 5 enable to set/input new value for a varible. It will show input dialog. New value is set from the keyboard step by step by selecting numbers. Input is finished and value is writen to the variable by selecting "enter" symbol.
  //     add_menu_item(parent menu id, menu type = value input dialog, text array reference, pointer to the variable, variable type)
  MyMenu.add_menu_item(        act_dt,                         5,                    2,  &actual_time_reference,            10);
  MyMenu.add_menu_item(        act_tm,                         5,                    3,  &actual_time_reference,            11);
  MyMenu.add_menu_item(        day_sh,                         5,                    4,       &daily_time_shift               );

 
  //Example: "Alarm" input submenu declaration
  //Example: menu_type 5 enable to set/input new value for a varible. It will show input dialog. New value is set from the keyboard step by step by selecting numbers. Input is finished and value is writen to the variable by selecting "enter" symbol.
  //     add_menu_item(parent menu id, menu - value input dialog, text array reference, pointer to the variable, variable type)
  MyMenu.add_menu_item(          altm,                         5,                    6,             &alarm_time,             9);

  Serial.begin(115200);
}

// Use this function to print texts on LCD when menu is not active.
// Function Print2LCD is called with menu refresh time period.
void Print2LCD() {
  tm = actual_time_reference + millis()/1000;
  lcd.setCursor(0,0);
  //print actual time on the 1st LCD row
  sval="   ";
  if (hour(tm)<10) {sval+="0";sval+=hour(tm);} else {sval+=hour(tm);}
  sval+=":";
  if (minute(tm)<10) {sval+="0";sval+=minute(tm);} else {sval+=minute(tm);}
  sval+=":";
  if (second(tm)<10) {sval+="0";sval+=second(tm);} else {sval+=second(tm);}
  sval+=" ";
  lcd.print(sval);
  sval="   ";
  if (alarm_active) {lcd.write(byte(5));lcd.print("       ");} else {lcd.print("        ");}
  lcd.setCursor(0,1);  
  if (day(tm)<10) {sval+="0";sval+=day(tm);} else {sval+=day(tm);}
  sval+=".";
  if (month(tm)<10) {sval+="0";sval+=month(tm);} else {sval+=month(tm);}
  sval+=".";
  if (year(tm)<10) {sval+="0";sval+=year(tm);} else {sval+=year(tm);}
  sval+="   ";
  lcd.print(sval);
}

// Function Menu_Off is called when the menu goes to inactive state
void Menu_Off() {
  calc_start();
  //calculates fraction of the day in milliseconds, when 1 sec time compensation of will be applied
  time_shift_fraction = 86400000 / abs(daily_time_shift);
  time_shifts = millis() / time_shift_fraction;
}

void alm() {
  if (alarm_active) {  //time program 1 is active
    if (next_start < (actual_time_reference + millis()/1000) && (next_start + 60) > (actual_time_reference + millis()/1000) && !alarm_on) { // start at alarm_time (within 1 minute window)
      alarm_endtime = (actual_time_reference + millis()/1000) + (unsigned long)alarm_duration;
      alarm_on = true;
    }
  }
  if (alarm_on && (actual_time_reference + millis()/1000) % 2 == 0 && play == 0) {tone(buzz_pin, 2000, 1000);play = 1;} //play tone 2000 hz each even second
  if (alarm_on && (actual_time_reference + millis()/1000) % 2 == 1 && play == 1) {play = 0;}
  
  if (alarm_endtime < (actual_time_reference + millis()/1000) && alarm_on) {digitalWrite(buzz_pin, 0);alarm_on = false;play = 0;calc_start();} //stops the alarm
}

void calc_start() {  //Calculates next alarm time
  tm = actual_time_reference + millis()/1000;
  midnight = tm - ((unsigned long) hour(tm)*3600 + minute(tm)*60 + second(tm));
  if (actual_time_reference + millis()/1000 - midnight < alarm_time) {//program will start today at start_time1 hour
    next_start = midnight + alarm_time;  
  } else {//program will start next day at start_time1 hour
    next_start = midnight + alarm_time + 86400; 
  }
  valid = 0;
  b = 1;
  while (!valid && b <= 7) {
    if (weekday(next_start)==1 && su==1) {valid=1;}  //next_start day is Sunday and alarm on Sunday is enabled
    if (weekday(next_start)==2 && mo==1) {valid=1;}  //next_start day is Monday and alarm on Monday is enabled
    if (weekday(next_start)==3 && tu==1) {valid=1;}  //next_start day is Tuesday and alarm on Tuesday is enabled
    if (weekday(next_start)==4 && we==1) {valid=1;}  //next_start day is Wednesday and alarm on Wednesday is enabled
    if (weekday(next_start)==5 && th==1) {valid=1;}  //next_start day is Thursday and alarm on Thursday is enabled
    if (weekday(next_start)==6 && fr==1) {valid=1;}  //next_start day is Friday and alarm on Friday is enabled
    if (weekday(next_start)==7 && sa==1) {valid=1;}  //next_start day is Saturday and alarm on Saturday is enabled
    if (!valid) {next_start += 86400;b++;}
  }
  if (!valid) {alarm_active = 0;next_start = 0;} //if not found any valid alarm start during a week, switch off alarms and set next start to 0 
}

void time_shifts_compensation() { //Compensation of slow up or quick up the internal arduino clock.
  if (time_shifts != millis() / time_shift_fraction) {
    time_shifts = millis() / time_shift_fraction;
    if (daily_time_shift > 0) {actual_time_reference += 1;} else {actual_time_reference -= 1;}
  }
}

void millis_restart() { //millis() are restarting from zero each 49.7 days. Few seconds before restart actual_time_reference is shifted by 49.7 days forward
  if (millis() > 4294960000) {
    noInterrupts ();
    timer0_millis = 0;
    actual_time_reference += 4294960;
    interrupts ();
  }
  //if (millis()+ test > 4294960000 && time_restart == false) {time_restart=true;}          //old version code
  //if (millis()+ test <= 4294960000 && time_restart == true) {time_restart=false;actual_time_reference += 4294967;}   //old version code
}

void loop() {
  alm(); //Makes alarm in preset time
  time_shifts_compensation(); //Compensation of slow up or quick up the internal arduino clock.
  millis_restart(); //millis() are restarting from zero each 49.7 days. Few seconds before restart actual_time_reference is shifted by 49.7 days forward
  MyMenu.draw(); //calling method to show the menu on the screen of the display
}
