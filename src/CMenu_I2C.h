#ifndef CMenu_I2C_h
#define CMenu_I2C_h

#include "Arduino.h"
#include <Time.h>
#include <TimeLib.h>
#include <LiquidCrystal_I2C.h>

extern const char* const tt[] PROGMEM;

extern const String On_label;
extern const String Off_label;
extern const String New_val;

const char* const empty_str = "                    ";

#ifndef Max_Menu_items
#define Max_Menu_items 100
#endif

#ifndef Max_Event_items
#define Max_Event_items 10
#endif

extern void Print2LCD();
extern void Menu_Off();

class List_item {
  public:
    unsigned long item_time;
    byte item_tt_id;
    float item_val;
    List_item(unsigned long _item_time, byte _item_tt_id, float _item_val);
};

class Menu_item {
  public:
    int parent_id;
    int action_type;
    //0 - menu
    //1 - menu + display value
    //2 - menu + set int to 1/0 - thick [v]/[ ]
    //3 - menu + set int to 1/0 - ON/OFF
    //4 - display events: 1st row - datetime, 2nd row - event description + int value
    //5 - set value


    byte tt_id;
    void* var_point;
    byte var_point_type;
    //0 - bool                 0 ... 1
    //1 - byte                 0 ... 255
    //2 - int            -32 768 ... 32 767
    //3 - word/unsigned int    0 ... 65535
    //4 - long    -2 147 483 648 ... 2 147 483 647
    //5 - unsigned long   0 ... 4 294 967 295
    //6 - float   -3.4028235E+38 ... 3.4028235E+38
    //7 - double
    //8 - datetime(unsigned long): date DD.MM.YY. Unsigned long - number of seconds since 1.1.1970 00:00:00 (Time.h, TimeLib.h library)
    //9 - datetime(unsigned long): time hh:mm:ss. Unsigned long - number of seconds since 1.1.1970 00:00:00 (Time.h, TimeLib.h library)

  public:
    Menu_item(int _par_id, int _act_type, byte _t_id, void* _var_p);
};

class Menu {

  private:
    LiquidCrystal_I2C* LCD;

    byte Display_columns;
    byte Display_rows;

    //Menu Items
    Menu_item* menu_item[Max_Menu_items];  //Max_Menu_items  value is defined in the Sketch
    List_item* list_item[Max_Event_items]; //Max_Event_items value is defined in the Sketch
    const char* const *tt_point;    //pointer to the text table array stored in PROGMEM

    //1: date format DD.MM.YY, 0: date format MM.DD.YY
    bool date_DMY;

    //Running text
    byte row_sh[4];      //running text - actual shift
    byte row_dir[4];     //running text - direction: 1 - right_ready, 2 - right_go, 3 - left_ready, 4 - left_go

    char tt_text[20];  //buffer for actual menu item text

    //Actual positions
    byte row_pos;      //actual row possition on the display: 0 - first row, 1 - second row
    int  menu_pos;     //actual menu possition in Menu_item[] array
    byte menu_max_id;  //size of the Menu_item[] array
    int  list_pos;     //actual list possition in List_item[] array
    byte list_max_id;  //size of the List_item[] array
    byte input_pos_x;  //actual input string position x axis (input text)
    byte input_pos_y;  //actual input string position y axis (input menu)
    byte prev;         //Amount of previous menu rows to the actual menu position
    byte next;         //Amount of next menu rows after the actual menu position
    byte max_events;   //Maximal number of events records in the list

    tmElements_t tm;

    byte txt_pos;
    int  txt_pos_dir;

    char tmps[20] = "";
    String s;

    //User inputs
    bool analog_input;
    byte input_pin;
    byte input_val[8];
    bool rotary;
    int  lastkey;      //Last pressed key
    byte double_click;
    int  double_click_delay = 700;

    unsigned long refresh_time;
    unsigned int  menu_refresh;
    unsigned long last_keypress;
    byte last_direction; //direction of last rotary movement: 0 - no rotation, 1 - CW, 2 - CCW
    bool extra_read;
    byte sleep_time;
    byte off_time;
    bool LCD_off;
    bool sleep = 0;

//Auxiliary variables
    bool auxo;
    bool auxo1;
    byte auxb;
    byte auxb1;
    byte auxb2;
    int  auxi;
    int  auxi1;
    long auxl;
    unsigned long auxl1;
    unsigned long auxl2;
    
//Methods
    byte cnt_prev();
    byte cnt_next();
    void draw_cursors();
    int  evaluate_button_analog(int but);
    void menu_draw();
    void navigate_2submenu();
    void read_keys();
    void menu_init();
    void runn_txt_init();
    byte move_input_pos_y(byte input_pos_y, byte var_type, bool fwd);
    void lcdprint(byte row, byte col, String str, byte chrs = 255);
    void t_fl();
    void t_us();
    void t_si();
    void t_d();
    void t_t();
    void d_s(unsigned long dt, byte mls);
    void t_s(unsigned long dt, byte mls);
    void fill_s();
    void get_val();

  public:
    Menu();
    void begin(LiquidCrystal_I2C LCD, const char* const _tt_point[], unsigned int _menu_refresh, byte _sleep_time, byte _off_time, byte _display_columns, byte _display_rows, bool _date_DMY = 1, byte _max_events = 10);
    void key_input(bool _analog_input, byte _pin, word _up, word _down, word _left, word _right);
    
    byte add_menu_item(int _par_id, int _act_type, byte _t_id, bool* _var_p = 0, byte var_type = 0);
    byte add_menu_item(int _par_id, int _act_type, byte _t_id, byte* _var_p, byte var_type = 1);
    byte add_menu_item(int _par_id, int _act_type, byte _t_id, int* _var_p, byte var_type = 2);
    byte add_menu_item(int _par_id, int _act_type, byte _t_id, word* _var_p, byte var_type = 3);
    byte add_menu_item(int _par_id, int _act_type, byte _t_id, long* _var_p, byte var_type = 255);
    byte add_menu_item(int _par_id, int _act_type, byte _t_id, unsigned long* _var_p, byte var_type = 5);
    byte add_menu_item(int _par_id, int _act_type, byte _t_id, float* _var_p, byte var_type = 3);
    byte add_menu_item(int _par_id, int _act_type, byte _t_id, double* _var_p, byte var_type = 3);

    void add_event(unsigned long _event_time, byte _event_text_id, float event_var);
    void draw();
};

#endif
