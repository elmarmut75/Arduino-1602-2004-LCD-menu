// Example of simple irigation system
// Configuration
// * 3 independent watering loop (3x soil humidity sensors, 3x valves)
// * 2 time based watering program
// * 1 low humidity based watering program
// * simple event recording
// * time compensation of slow up or quick up the internal arduino clock
// * menu control by rotary encoder input (or 4 buttons via 4 binary inputs or 1 analog input)
// * display on 20x4 LCD display (or 16x2) using LiquidCrystal_I2C.h library

#include <Time.h>               //time library used for handling with datetime variables type
#include <TimeLib.h>            //time library used for handling with datetime variables type
#include <Wire.h>               //I2C communication library 
#include <LiquidCrystal_I2C.h>  //LCD display library

#define display_columns 16
#define display_rows 2

// LiquidCrystal_I2C setup
// Define new instance (lcd) of LiquidCrystal_I2C class
// LiquidCrystal_I2C <instance name>(<I2C address>, <display columns>, <display rows>)

LiquidCrystal_I2C lcd(0x3f,display_columns,display_rows);

#include "CMenu_I2C.h" //Read the Menu library

//Texts for menu dialog like on/off, open/close for whole menu
const String On_label  = "ON ";       //set on/off labels string for menu dialog
const String Off_label = "OFF";       //set on/off labels string for menu dialog
const String New_val   = "Enter:";    //set input label string for menu dialog

//Texts of individual menu items (variable names and programs/rutines names). Texts are stored in the program memory to spare space in the local memory
const char t00[] PROGMEM = "Sensors";        
const char t01[] PROGMEM = "Settings";      
const char t02[] PROGMEM = "Programs";       
const char t03[] PROGMEM = "Commands";      
const char t04[] PROGMEM = "Records";      
const char t05[] PROGMEM = "Humidity 1";    
const char t06[] PROGMEM = "Humidity 2";   
const char t07[] PROGMEM = "Humidity 3";   
const char t08[] PROGMEM = "Treshold";
const char t09[] PROGMEM = "Cycle time";
const char t10[] PROGMEM = "Start time1";   
const char t11[] PROGMEM = "Start time2";
const char t12[] PROGMEM = "Act. time";
const char t13[] PROGMEM = "Act. date";   
const char t14[] PROGMEM = "Humid.treshld";
const char t15[] PROGMEM = "Time prog.1";
const char t16[] PROGMEM = "Time prog.2";
const char t17[] PROGMEM = "Valve1";
const char t18[] PROGMEM = "Valve2";
const char t19[] PROGMEM = "Valve3";
const char t20[] PROGMEM = "Low humid. 1";
const char t21[] PROGMEM = "Low humid. 2";
const char t22[] PROGMEM = "Low humid. 3";
const char t23[] PROGMEM = "Daily time shift";

//Construction of the menu texts array in the program memory.
const char* const tt[] PROGMEM = {t00, t01, t02, t03, t04, t05, t06, t07, t08, t09, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19, t20, t21, t22, t23};

Menu MyMenu; //Define new instance MyMenu class

//User program variables declaration
int valve1_pin = 11;
int valve2_pin = 12;
int valve3_pin = 13;
bool Valve1 = 0;
bool Valve2 = 0;
bool Valve3 = 0;

int humidity1_pin = 1;
int humidity2_pin = 2;
int humidity3_pin = 3;
int Humidity1, Humidity2, Humidity3;

float humidity_treshold = 400; //Humidity treshold value for start of watering
int   cycle_time = 300; //Watering cycle period time is seconds
bool  watering_ON; //Watering cycle running flag

unsigned long start_time1 = 28800; //Watering start day time 1 08:00:00 (number of seconds since 00:00:00 1.1.1970)
unsigned long start_time2 = 68400; //Watering start day time 2 19:00:00 (number of seconds since 00:00:00 1.1.1970)
unsigned long next_start1 = 0; //Watering start time 1 - time stamp
unsigned long next_start2 = 0; //Watering start time 2 - time stamp
unsigned long midnight; //Midnight - time stamp
unsigned long watering_endtime = 0;  //watering end - time stamp

bool prog_low_humidity = 0; //Program of watering on low humidity level flag
bool prog_time1 = 0; //Time 1 watering program flag
bool prog_time2 = 0; //Time 2 watering program flag


unsigned long actual_time_reference = 0; // actual time - time stamp
bool time_restart = false; //millis() are restarting from zero each 49.7 days. Restart flag

//value in seconds for compensation of slow up or quick up the internal arduino clock (for example value should be 30 when internal clock during 24 hours slow up 30 seconds)
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
  pinMode(2, INPUT);   // pin 2 is used for buttons/rotary encoder input
  pinMode(3, INPUT);   // pin 3 is used for buttons/rotary encoder input
  pinMode(4, INPUT);   // pin 4 is used for buttons/rotary encoder input
  pinMode(5, INPUT);   // pin 5 is used for buttons/rotary encoder input
  pinMode(valve1_pin, OUTPUT); // pin 11 is used for operating of valve 1
  pinMode(valve2_pin, OUTPUT); // pin 12 is used for operating of valve 2
  pinMode(valve3_pin, OUTPUT); // pin 13 is used for operating of valve 2


  //-------------------------------------
  //Menu initialization (CMenu.h library)
  //Initialize menu (LCD_object, PROGMEM_text_table,  refresh_time_[ms], sleep_time [s], off_time[min] (255=off), display_columns, display_rows, date_DMY [1: date format DD.MM.YY, 0: date format MM.DD.YY, default is DD.MM.YY, max_events (0..50)])
  MyMenu.begin(       lcd,                 tt,               1000,                   30,                       5, display_columns, display_rows,                         1                                                                       , 10);

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
  //       key_input(analog_input, pin, up, down, left, right)
  //MyMenu.key_input(           1,   0, 40,  160,   10,   250); //analog input pin A0, up, down, left, right represents value on analog input pin when you press corresponding key [0-1024]
  //MyMenu.key_input(           0,   0,  3,    4,    5,     2); //digital input pins 3 up, 4 down, 5 left, 2 right
    MyMenu.key_input(           0,   0,  2,    3,    4,     4); //rotary encoder pins 2 CLK, 3 DT, 4 SW

  //Creation of the new Menu item.
  //  Maximal number of menu items is 100.
  //int item_id Menu::add_menu_item(int parent_id, int action_type, byte tt[], int variable_pointer (optional), byte var_type (optional));
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
  //                     add_menu_item(    root level, menu - only text, text array reference)
  byte Sensors  = MyMenu.add_menu_item(            -1,                0,                    0);
  byte Settings = MyMenu.add_menu_item(            -1,                0,                    1);
  byte Programs = MyMenu.add_menu_item(            -1,                0,                    2);
  byte Commands = MyMenu.add_menu_item(            -1,                0,                    3);
  byte Records  = MyMenu.add_menu_item(            -1,                0,                    4);

  //Example: "Sensors" submenu declaration
  //Example: menu_type 1 will show actual value of the Humidity1 variable in the menu
  //Example: &Humidity1 is pointer to the user variable Humidity1
  //Example: variable Sensors contain index of the "Sensors" menu item declared above
  //     add_menu_item(parent menu id, menu - only text, text array reference, pointer to the variable)
  MyMenu.add_menu_item(       Sensors,                1,                    5,                   &Humidity1);
  MyMenu.add_menu_item(       Sensors,                1,                    6,                   &Humidity2);
  MyMenu.add_menu_item(       Sensors,                1,                    7,                   &Humidity3);


  //Example: "Settings" submenu declaration
  //                     add_menu_item(parent menu id, menu - text + value, text array reference, pointer to the variable, variable type)
  byte treshold = MyMenu.add_menu_item(      Settings,                   1,                    8,      &humidity_treshold                );
  byte cycle    = MyMenu.add_menu_item(      Settings,                   1,                    9,             &cycle_time                );
  byte start1   = MyMenu.add_menu_item(      Settings,                   1,                   10,            &start_time1,              9);
  byte start2   = MyMenu.add_menu_item(      Settings,                   1,                   11,            &start_time2,              9);
  byte ref_time = MyMenu.add_menu_item(      Settings,                   1,                   12,  &actual_time_reference,             11);
  byte ref_date = MyMenu.add_menu_item(      Settings,                   1,                   13,  &actual_time_reference,             10);
  byte day_shift= MyMenu.add_menu_item(      Settings,                   1,                   23,       &daily_time_shift                );

  //Example: "Programs" submenu declaration
  //Example: menu_type 2 enable to set logical 0 or 1 to a variable
  //Example: It can be used for switching on/off some program rutines from the menu
  //     add_menu_item(parent menu id, menu - text + set value 1/0, text array reference, pointer to the variable)
  MyMenu.add_menu_item(      Programs,                           2,                   14,      &prog_low_humidity);
  MyMenu.add_menu_item(      Programs,                           2,                   15,             &prog_time1);
  MyMenu.add_menu_item(      Programs,                           2,                   16,             &prog_time2);

  //Example: "Commands" submenu declaration
  //Example: menu_type 3 enable to set logical 0 or 1 to a variable
  //Example: It is simillar menu_type 2. It can be used for switching on/off I/O
  //     add_menu_item(parent menu id, menu - text + set value 1/0, text array reference, pointer to the variable)  
  MyMenu.add_menu_item(      Commands,                           3,                   17,                   &Valve1);
  MyMenu.add_menu_item(      Commands,                           3,                   18,                   &Valve2);
  MyMenu.add_menu_item(      Commands,                           3,                   19,                   &Valve3);

  //Example: "Records" submenu declaration
  //Example: menu_type 4 displays record list. Each record contains event_time, event_name and some value at the time when event occure
  //     add_menu_item(parent menu id, menu - display events, text array reference)
  MyMenu.add_menu_item(       Records,                     4,                    4);
  
  //Example: "Settings" input submenu declaration
  //Example: menu_type 5 enable to set/input new value for a varible. It will show input dialog. New value is set from the keyboard step by step by selecting numbers. Input is finished and value is writen to the variable by selecting "enter" symbol.
  //     add_menu_item(parent menu id, menu - value input dialog, text array reference, pointer to the variable, variable type)
  MyMenu.add_menu_item(      treshold,                         5,                    8,      &humidity_treshold               );
  MyMenu.add_menu_item(         cycle,                         5,                    9,             &cycle_time               );
  MyMenu.add_menu_item(        start1,                         5,                   10,            &start_time1,             9);
  MyMenu.add_menu_item(        start2,                         5,                   11,            &start_time2,             9);
  MyMenu.add_menu_item(      ref_time,                         5,                   12,  &actual_time_reference,            11);
  MyMenu.add_menu_item(      ref_date,                         5,                   13,  &actual_time_reference,            10);
  MyMenu.add_menu_item(     day_shift,                         5,                   23,       &daily_time_shift               );

  //Example: Adding some events
  //void add_event(unsigned long  _event_time, byte _event_text_id, float event_var);
  // * _event_time is standard unix time (number of seconds since 1.1.1970 00:00:00)
  // * _event_text_id is index of the text in tt[] stored in te program memory
  // * event_var is value to be stored to the memory
  
  MyMenu.add_event(1557297120, 18, 532);
  MyMenu.add_event(1557302400, 15, 324);
  MyMenu.add_event(1557388800, 15, 342);
  MyMenu.add_event(1557475200, 15, 356);
  MyMenu.add_event(1557561600, 15, 379);
  MyMenu.add_event(1557648000, 15, 398);
  MyMenu.add_event(1557734400, 15, 425);
  MyMenu.add_event(1557820800, 15, 461);
  MyMenu.add_event(1557907200, 15, 492);
  MyMenu.add_event(1557911580, 18, 333);
  MyMenu.add_event(1557922394, 18, 321);
  
  Serial.begin(115200);
  watering_ON = false;
}

// Use this function to print texts on LCD when menu is not active.
// Function Print2LCD is called with menu refresh time period.
void Print2LCD() {
  //print actual time on the 1st LCD row
  sval="";
  tm = actual_time_reference + millis()/1000;
  if (day(tm)<10) {sval+="0";sval+=day(tm);} else {sval+=day(tm);}
  sval+=".";
  if (month(tm)<10) {sval+="0";sval+=month(tm);} else {sval+=month(tm);}
  sval+=".";
  if (year(tm)<10) {sval+="0";sval+=year(tm);} else {sval+=year(tm);}
  sval+=" ";
  if (hour(tm)<10) {sval+="0";sval+=hour(tm);} else {sval+=hour(tm);}
  sval+=":";
  if (minute(tm)<10) {sval+="0";sval+=minute(tm);} else {sval+=minute(tm);}
  sval+=":";
  if (second(tm)<10) {sval+="0";sval+=second(tm);} else {sval+=second(tm);}
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(sval);

  //print actual humidity levels from the H1, H2, H3 sensors on 2nd LCD row
  lcd.setCursor(0,1);
  lcd.print("H1:");
  lcd.print(Humidity1);
  lcd.print("   ");
  lcd.setCursor(7,1);
  lcd.print("H2:");
  lcd.print(Humidity2);
  lcd.print("   ");
  lcd.setCursor(14,1);
  lcd.print("H3:");
  lcd.print(Humidity3);
  lcd.print("   ");
}

// Function Menu_Off is called when the menu goes to inactive state
void Menu_Off() {
  calc_starts();  //Calculates next watering start times
  time_shift_fraction = 86400000 / abs(daily_time_shift);  //Calculates new constants for compensation of slow up or quick up the internal arduino clock.
  time_shifts = millis() / time_shift_fraction;  //Calculates new constants for compensation of slow up or quick up the internal arduino clock.
}

//Example: reading the Arduino Uno I/O and storing values to the variables
void readIO() {
  if (Valve1 == true) {
    digitalWrite(valve1_pin, HIGH);
  } else {
    digitalWrite(valve1_pin, LOW);
  }
  if (Valve2 == true) {
    digitalWrite(valve2_pin, HIGH);
  } else {
    digitalWrite(valve2_pin, LOW);
  }
  if (Valve3 == true) {
    digitalWrite(valve3_pin, HIGH);
  } else {
    digitalWrite(valve3_pin, LOW);
  }
  Humidity1 = analogRead(humidity1_pin);
  Humidity2 = analogRead(humidity2_pin);
  Humidity3 = analogRead(humidity3_pin);
}

void Low_Humidity() {
  if (prog_low_humidity) { //program for watering on low soil humidity level is enabled
    if (Humidity1 > humidity_treshold) { //soil humidity on sensor 1 is less then treshold (value red from the sensor is higher than the treshold)
      Valve1=1;  //opens valve 1
      watering_endtime = (actual_time_reference + millis()/1000) + (unsigned long)cycle_time;
      watering_ON = true;
      MyMenu.add_event((actual_time_reference + millis()/1000), 20, Humidity1); //add event to the event log
    }
    if (Humidity2 > humidity_treshold) { //soil humidity on sensor 2 is less then treshold (value red from the sensor is higher than the treshold)
      Valve2=1;  //opens valve 2
      watering_endtime = (actual_time_reference + millis()/1000) + (unsigned long)cycle_time;
      watering_ON = true;
      MyMenu.add_event((actual_time_reference + millis()/1000), 21, Humidity2); //add event to the event log
    }
    if (Humidity3 > humidity_treshold) { //soil humidity on sensor 3 is less then treshold (value red from the sensor is higher than the treshold)
      Valve3=1;  //opens valve 3
      watering_endtime = (actual_time_reference + millis()/1000) + (unsigned long)cycle_time;
      watering_ON = true;
      MyMenu.add_event((actual_time_reference + millis()/1000), 22, Humidity3); //add event to the event log
    }
  }
}

void Time_Prog1() {
  if (prog_time1) {  //time program 1 is active
    if (next_start1 < (actual_time_reference + millis()/1000) && (next_start1 + 60) > (actual_time_reference + millis()/1000) && !watering_ON) { // start at start_time1 (within 1 minute window)
      Valve1=1;  //opens valve 1
      Valve2=1;  //opens valve 2
      Valve3=1;  //opens valve 3
      watering_endtime = (actual_time_reference + millis()/1000) + (unsigned long)cycle_time;
      watering_ON = true;
      MyMenu.add_event((actual_time_reference + millis()/1000), 10, 0); //add event to the event log
    }
  }
  if (watering_endtime < (actual_time_reference + millis()/1000) && watering_ON) {Valve1=0;Valve2=0;Valve3=0;watering_ON = false;calc_starts();} //stop the watering when watering cycle ends and calculate next start times
}

void Time_Prog2() {
  if (prog_time2) {  //time program 2 is active
    //Serial.print("next_start2:");Serial.print(next_start2);Serial.print(",act_time:");Serial.println(actual_time_reference + millis()/1000);
    if (next_start2 < (actual_time_reference + millis()/1000) && (next_start2 + 60) > (actual_time_reference + millis()/1000) && !watering_ON) { // start at start_time1 (within 1 minute window)
      Valve1=1;  //opens valve 1
      Valve2=1;  //opens valve 2
      Valve3=1;  //opens valve 3
      watering_endtime = (actual_time_reference + millis()/1000) + (unsigned long)cycle_time;
      watering_ON = true;
      MyMenu.add_event((actual_time_reference + millis()/1000), 11, 0); //add event to the event log
    }
  }
  if (watering_endtime < (actual_time_reference + millis()/1000) && watering_ON) {Valve1=0;Valve2=0;Valve3=0;watering_ON = false;calc_starts();} //stop the watering when watering cycle ends and calculate next start times
}

void calc_starts() {
  tm = actual_time_reference + millis()/1000;
  midnight = tm - ((unsigned long) hour(tm)*3600 + minute(tm)*60 + second(tm));
  if (actual_time_reference + millis()/1000 - midnight < start_time1) {//program will start today at start_time1 hour
    next_start1 = midnight + start_time1;  
  } else {//program will start next day at start_time1 hour
    next_start1 = midnight + start_time1 + 86400; 
  }
  if (actual_time_reference + millis()/1000 - midnight < start_time2) {//program will start today at start_time1 hour
    next_start2 = midnight + start_time2;  
  } else {//program will start next day at start_time2 hour
    next_start2 = midnight + start_time2 + 86400; 
  }  
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
  Low_Humidity();              //Starts watering when soil humidity is less then preset treshold
  Time_Prog1();                //Starts watering on preset time 1
  Time_Prog2();                //Starts watering on preset time 2
  time_shifts_compensation();  //Compensation of slow up or quick up the internal arduino clock.
  millis_restart();            //millis() are restarting from zero each 49.7 days. Few seconds before restart actual_time_reference is shifted by 49.7 days forward
  readIO();                    //Reading the Arduino Uno I/O and storing values to the variables 
  MyMenu.draw();               //calling method to show the menu on the screen of the display
}
