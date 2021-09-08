#include "Arduino.h"
#include "CMenu_I2C.h"

                      //01234567890123
static const char in_char[] = "0123456789-.R";  //R - is custom defined character "enter". It will finish input dialog

static const byte downArrow[8] = {
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b10101, // * * *
  0b01110, //  ***
  0b00100  //   *
};

static const byte upArrow[8] = {
  0b00100, //   *
  0b01110, //  ***
  0b10101, // * * *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100  //   *
};

static const byte menuCursor[8] = {
  B01000, //  *
  B00100, //   *
  B00010, //    *
  B00001, //     *
  B00010, //    *
  B00100, //   *
  B01000, //  *
  B00000  //
};


static const byte thick[8] = {
  B00000, //     
  B00000, //     
  B00001, //    *
  B00011, //   **
  B10110, //* ** 
  B11100, //***  
  B01000, // *   
  B00000  //     
};


static const byte enter[8] = {
  B00001, //    *
  B00001, //    * 
  B00001, //    *
  B00101, //  * *
  B01001, // *  *
  B11111, //*****  
  B01000, // *   
  B00100  //  *   
};

List_item::List_item(unsigned long _item_time, byte _item_tt_id, float _item_val) {
  this->item_time = _item_time;
  this->item_tt_id = _item_tt_id;
  this->item_val = _item_val;
}
  
Menu_item::Menu_item(int _par_id, int _act_type, byte _t_id, void* _var_p) {
  this->parent_id = _par_id;
  this->action_type = _act_type;
  this->tt_id = _t_id;
  this->var_point = _var_p; 
}


//Menu:------------------------------------------------------

Menu::Menu() {};

void Menu::begin(LiquidCrystal_I2C LCD, const char* const _tt_point[], unsigned int _menu_refresh, byte _sleep_time, byte _off_time, byte _display_columns, byte _display_rows, bool _date_DMY, byte _max_events) {
  this->LCD = &LCD;
  this->LCD->createChar(0, (uint8_t*) menuCursor);
  this->LCD->createChar(1, (uint8_t*) upArrow);
  this->LCD->createChar(2, (uint8_t*) downArrow);
  this->LCD->createChar(3, (uint8_t*) thick);
  this->LCD->createChar(4, (uint8_t*) enter);
  this->menu_max_id = 0;
  this->list_pos = 0;
  this->list_max_id = 0;
  this->prev = 0;
  this->next = 0;
  this->tt_point = _tt_point;
  this->menu_refresh = _menu_refresh;
  this->sleep_time = _sleep_time;
  this->off_time = _off_time;
  this->date_DMY = _date_DMY;
  this->Display_columns = _display_columns;
  this->Display_rows = _display_rows;
  if (_max_events < Max_Event_items){this->max_events = _max_events;} else {this->max_events = Max_Event_items;}
  if ( max_events < 0){this->max_events = 0;}
  menu_init();
}

void Menu::key_input(bool _analog_input, byte _pin, word _up, word _down, word _left, word _right) {
  analog_input = _analog_input;
  input_pin = _pin;
  input_val[1] = 2;
  input_val[3] = 3;
  input_val[5] = 4;
  input_val[7] = 1;
  if (analog_input) {
    input_val[0] = (byte) _up / 4;
    input_val[2] = (byte) _down / 4;
    input_val[4] = (byte) _left / 4;
    input_val[6] = (byte) _right / 4;
    
    //Ascending sort inputs by analog value
    auxo = true;
    while (auxo) {
      auxo = false;
      for (auxi=0; auxi < 3; auxi++) {
        if (input_val[auxi*2] > input_val[auxi*2 + 2]) {
          auxo = true;
          auxb = input_val[auxi*2];
          input_val[auxi*2] = input_val[auxi*2 + 2];
          input_val[auxi*2 + 2] = auxb;
          auxb = input_val[auxi*2 + 1];
          input_val[auxi*2 + 1] = input_val[auxi*2 + 3];
          input_val[auxi*2 + 3] = auxb;
        }
      }
    }
  } else {
    if (_left == _right) {rotary = 1;} else {rotary = 0;}
    input_val[0] = _up;
    input_val[2] = _down;
    input_val[4] = _left;
    input_val[6] = _right;
  }
}

  //byte var_point_type;
  //0 - bool                 0 ... 1
  //1 - byte                 0 ... 255
  //2 - int            -32 768 ... 32 767
  //3 - word/unsigned int    0 ... 65535
  //4 - long    -2 147 483 648 ... 2 147 483 647
  //5 - time/unsigned long   0 ... 4 294 967 295
  //6 - float   -3.4028235E+38 ... 3.4028235E+38

byte Menu::add_menu_item(int _par_id, int _act_type, byte _t_id, bool* _var_p, byte var_type) {
  this->menu_item[this->menu_max_id] = new Menu_item(_par_id, _act_type, _t_id, (void*)_var_p);
  this->menu_item[this->menu_max_id]->var_point_type = 0;
  this->menu_max_id=this->menu_max_id + 1;
  return menu_max_id - 1;
}

byte Menu::add_menu_item(int _par_id, int _act_type, byte _t_id, byte* _var_p, byte var_type) {
  this->menu_item[this->menu_max_id] = new Menu_item(_par_id, _act_type, _t_id, (void*)_var_p);
  this->menu_item[this->menu_max_id]->var_point_type = 1;
  this->menu_max_id=this->menu_max_id + 1;
  return menu_max_id - 1;
}

byte Menu::add_menu_item(int _par_id, int _act_type, byte _t_id, int* _var_p, byte var_type) {
  this->menu_item[this->menu_max_id] = new Menu_item(_par_id, _act_type, _t_id, (void*)_var_p);
  this->menu_item[this->menu_max_id]->var_point_type = 2;
  this->menu_max_id=this->menu_max_id + 1;
  return menu_max_id - 1;
}

byte Menu::add_menu_item(int _par_id, int _act_type, byte _t_id, unsigned int* _var_p, byte var_type) {
  this->menu_item[this->menu_max_id] = new Menu_item(_par_id, _act_type, _t_id, (void*)_var_p);
  this->menu_item[this->menu_max_id]->var_point_type = 3;
  this->menu_max_id=this->menu_max_id + 1;
  return menu_max_id - 1;
}

byte Menu::add_menu_item(int _par_id, int _act_type, byte _t_id, long* _var_p, byte var_type) {
  this->menu_item[this->menu_max_id] = new Menu_item(_par_id, _act_type, _t_id, (void*)_var_p);
  this->menu_item[this->menu_max_id]->var_point_type = 4;
  this->menu_max_id=this->menu_max_id + 1;
  return menu_max_id - 1;
}

byte Menu::add_menu_item(int _par_id, int _act_type, byte _t_id, unsigned long* _var_p, byte var_type) {
  this->menu_item[this->menu_max_id] = new Menu_item(_par_id, _act_type, _t_id, (void*)_var_p);
  if (var_type == 255) {
    this->menu_item[this->menu_max_id]->var_point_type = 5;
  } else {
    this->menu_item[this->menu_max_id]->var_point_type = var_type;
  }
  this->menu_max_id=this->menu_max_id + 1;
  return menu_max_id - 1;
}

byte Menu::add_menu_item(int _par_id, int _act_type, byte _t_id, float* _var_p, byte var_type) {
  this->menu_item[this->menu_max_id] = new Menu_item(_par_id, _act_type, _t_id, (void*)_var_p);
  this->menu_item[this->menu_max_id]->var_point_type = 6 + var_type * 20;
  this->menu_max_id=this->menu_max_id + 1;
  return menu_max_id - 1;
}

byte Menu::add_menu_item(int _par_id, int _act_type, byte _t_id, double* _var_p, byte var_type) {
  this->menu_item[this->menu_max_id] = new Menu_item(_par_id, _act_type, _t_id, (void*)_var_p);
  this->menu_item[this->menu_max_id]->var_point_type = 7 + var_type * 20;
  this->menu_max_id=this->menu_max_id + 1;
  return menu_max_id - 1;
}

void Menu::add_event(unsigned long _event_time, byte _event_text_id, float event_var) {
  if (list_max_id < max_events) {
    list_item[list_max_id] = new List_item(_event_time, _event_text_id, event_var);
    list_max_id = list_max_id + 1;   
  }
  //Move old data by one index
  if (list_max_id > 1) {
    for (byte i = (list_max_id - 1); i > 0; i--){
      list_item[i]->item_time  = list_item[i-1]->item_time;
      list_item[i]->item_tt_id = list_item[i-1]->item_tt_id;
      list_item[i]->item_val   = list_item[i-1]->item_val;
    }
    list_item[0]->item_time  = _event_time;
    list_item[0]->item_tt_id = _event_text_id;
    list_item[0]->item_val = event_var;
  }
}

byte Menu::cnt_prev() {
  auxb = 0;
  auxi = menu_pos - 1;
  while (auxi > -1 and menu_item[auxi]->parent_id == menu_item[menu_pos]->parent_id) {
    auxb++;auxi--;
  }
  return auxb;  
}

byte Menu::cnt_next() {
  auxb = 0;
  auxi = menu_pos + 1;
  while (auxi < (int)(sizeof(menu_item) / sizeof(menu_item[0])) and menu_item[auxi]->parent_id == menu_item[menu_pos]->parent_id) {
    auxb++;auxi++;
  }
  return auxb;  
}

void Menu::draw_cursors() {
  
  if (menu_item[menu_pos-row_pos]->action_type < 4 ) {
    auxb = 0;
    while (auxb<Display_rows) {
      this->LCD->setCursor(0, auxb);
      if (auxb==row_pos){this->LCD->write(byte(0));} else {this->LCD->print(" ");}
      auxb++;
    }  
    //Draw the menu cursors (arrows symbols on right side of the display)
    auxb = 0;
    while (auxb<Display_rows) {
      this->LCD->setCursor(Display_columns - 1, auxb);
      if (prev - row_pos > 0 && auxb==0) {this->LCD->write(byte(1));}
      else if (next + row_pos - Display_rows + 1 > 0  && auxb+1 == Display_rows) {this->LCD->write(byte(2));}
      else {this->LCD->print(" ");}
      auxb++;
    } 
  }
  if (menu_item[menu_pos-row_pos]->action_type == 4 ) {
    //Draw the menu cursors (arrows symbols on right side of the display)
    this->LCD->setCursor(Display_columns - 1, 0);
    if (list_pos > 0) {this->LCD->write(byte(1));} else {this->LCD->print(" ");}  
    this->LCD->setCursor(Display_columns - 1, Display_rows - 1);
    if (list_pos < (list_max_id - 1)) {this->LCD->write(byte(2));} else {this->LCD->print(" ");} 
  }
}

int  Menu::evaluate_button_analog(int but) {
  //Serial.println(but);
  if (but < input_val[0]*4) {
    auxi = input_val[1];
  } else if (but < input_val[2]*4) {
    auxi = input_val[3];
  } else if (but < input_val[4]*4) {
    auxi = input_val[5];
  } else if (but < input_val[6]*4) {
    auxi = input_val[7];
  }
  return auxi;  
}
  
void Menu::draw() {
  //Menu will be redrawn each LCD_refresh time
  if (refresh_time < millis() || refresh_time > (millis() + (unsigned long) menu_refresh)) {
    refresh_time = millis() + (unsigned long) menu_refresh;
    if (LCD_off == 0) {
      if (sleep == 1) {
        Print2LCD();
      } else {
        extra_read = 1;
        menu_draw();
      }
    }
  }
  read_keys();
}

void Menu::menu_init() {
  LCD_off = 0;
  row_pos = 0;
  list_pos = 0;
  input_pos_x = 0;
  input_pos_y = 0;
  tmps[0] = '\0';
  txt_pos = 0;
  txt_pos_dir = 1;
  runn_txt_init();
  extra_read = 0;
  this->LCD->noBlink();  
}

void Menu::runn_txt_init() {
  row_sh[0]  = 0;
  row_sh[1]  = 0;
  row_sh[2]  = 0;
  row_sh[3]  = 0;
  row_dir[0] = 1;
  row_dir[1] = 1;
  row_dir[2] = 1;
  row_dir[3] = 1;
}

void Menu::navigate_2submenu() {
  for (auxi=menu_pos; auxi<menu_max_id /*(sizeof(menu_item) / sizeof(menu_item[0]))*/; auxi++) {
    if (menu_item[auxi]->parent_id == menu_pos) {
      menu_pos = auxi;
      break;
    }
  }
  menu_init();
}

void Menu::fill_s() {
  auxb = s.length();
  while (auxb < Display_columns -1) {
    s += " ";
    auxb++;
  }
}

void Menu::read_keys() {
  if (analog_input) { //reading pressed key from analog value
    auxi = analogRead(input_pin);
    if (auxi < 1000 && (lastkey >=1000 || extra_read)) {auxb = evaluate_button_analog(auxi);} else {auxb = 0;}
    lastkey = auxi;
  } else { //reading pressed key from binary values
    if (rotary) { //reading rotary encoder
    //input_val[0] = _up; =CLK
    //input_val[1] = 2; =UP 
    //input_val[2] = _down; =DT
    //input_val[3] = 3; =DWN
    //input_val[4] = _left; =SW
    //input_val[5] = 4; =BACKWARD
    //input_val[6] = _right; =SW
    //input_val[7] = 1; =FORWARD
      
      auxb = 0;
      auxo = digitalRead(input_val[0]);
    //if (CLK != DT)  
      if (auxo != auxo1) {
        if (auxo != digitalRead(input_val[2])) {
          if (millis() - last_keypress > 180) {
            if (last_direction == 1) { //CW
              auxb = input_val[3];
            } else {
              last_direction = 1; //CW
            }
          }  
        } else {
          if (millis() - last_keypress > 180) {
            if (last_direction == 2) { //CCW
              auxb = input_val[1];
            } else {
              last_direction = 2; //CCW
            }
          }
        }
      }
      auxo1 = auxo;
      if (auxb == 0) {
        if (digitalRead(input_val[4]) == 1 && millis() - last_keypress > 100) {
          if (double_click == 0) {auxl1 = millis(); double_click = 1;} 
          if (double_click == 2) {double_click = 3;}
        } else {
          if (double_click == 1 && (millis() - auxl1) < (unsigned long)double_click_delay) {double_click = 2;}          
        }
      }
      if ((millis() - auxl1) >= (unsigned long)double_click_delay && (double_click == 1 || double_click == 2)) {double_click = 0; auxb = input_val[7];}
      if ((millis() - auxl1) >= (unsigned long)double_click_delay && double_click == 3) {double_click = 0; auxb = input_val[5];}
      lastkey = auxb; 
    } else { //reading pressed key from 4 binary inputs
      auxi = 0;
      if (digitalRead(input_val[6]) == 1) {auxi = input_val[7];}
      if (digitalRead(input_val[2]) == 1) {auxi = input_val[3];}
      if (digitalRead(input_val[0]) == 1) {auxi = input_val[1];}
      if (digitalRead(input_val[4]) == 1) {auxi = input_val[5];}
      if (auxi != 0 && (lastkey == 0 || extra_read)) {auxb = (byte) auxi;} else {auxb = 0;}
      lastkey = auxi;
    }
  }
  extra_read = 0;
  if (auxb!=0) {
    last_keypress = millis();
    if (LCD_off) {
        this->LCD->backlight();
        this->LCD->display();
        LCD_off = 0;
    } else {
      if (sleep==1) {
        sleep=0;
      }
    }
  } else {
    if (millis() > (last_keypress + ((unsigned long)off_time * 60000)) && LCD_off == 0 && off_time < 255) {
          LCD_off = 1;
          this->LCD->noDisplay();
          this->LCD->noBacklight();
    }
  }
  if (sleep==0) {
  auxb2 = menu_item[menu_pos]->var_point_type % 20;
  switch (auxb) {
    case 0: // When button returns as 0 there is no action taken
      if (millis() > last_keypress + ((unsigned long)sleep_time * 1000)) {
        Menu_Off();
        sleep = 1;
        auxb=0;
      }
    break;
    //**********"Forward" button is pressed*************************************
    case 1:  // "Forward" button is pressed
      switch (menu_item[menu_pos]->action_type) {
        case 0: //menu type: 0 - menu
          navigate_2submenu();
        break;
        case 1: //1 - menu + display int value
          navigate_2submenu();
        break;    
        case 2: //2 - menu + set int to 1/0 - tick [v]/[ ]
          if (*(bool*)menu_item[menu_pos]->var_point == 0) {*(bool*)menu_item[menu_pos]->var_point = 1;} else {*(bool*)menu_item[menu_pos]->var_point = 0;}
        break;
        case 3: //3 - menu + set int to 1/0 - ON/OFF
          if (*(bool*)menu_item[menu_pos]->var_point == 0) {*(bool*)menu_item[menu_pos]->var_point = 1;} else {*(bool*)menu_item[menu_pos]->var_point = 0;}
        break;
        case 5: //5 - set value
          if ((byte)in_char[input_pos_y]==82) { //ENTER character selected
            s = tmps;
            auxl = s.toInt();
            switch (auxb2) {
              case 0:
                if (auxl==0) {*(bool*)menu_item[menu_pos]->var_point = auxl;}
                if (auxl==1) {*(bool*)menu_item[menu_pos]->var_point = auxl;}
              case 1:
                if (auxl > -1 && auxl < 256) {*(byte*)menu_item[menu_pos]->var_point = auxl;}
              break;
              case 2:
                if (auxl > -32769 && auxl < 32768) {*(int*)menu_item[menu_pos]->var_point = auxl;}
              break;
              case 3:
                if (auxl > -1 && auxl < 65536) {*(unsigned int*)menu_item[menu_pos]->var_point = auxl;}
              break;
              case 4:
                *(long*)menu_item[menu_pos]->var_point = auxl;
              break;
              case 5:
                *(unsigned long*)menu_item[menu_pos]->var_point = auxl;
              break;
              case 6:
                *(float*)menu_item[menu_pos]->var_point = s.toFloat();
              break;
              case 7:
                *(double*)menu_item[menu_pos]->var_point = s.toDouble();
              break;
            }
            if (auxb2 == 8 || auxb2 == 10) {
                if (date_DMY) { //DD.MM.YY
                  tm.Day = ((byte)tmps[0]-48)*10+((byte)tmps[1]-48);
                  tm.Month = ((byte)tmps[3]-48)*10+((byte)tmps[4]-48);
                } else { //MM.DD.YY
                  tm.Month = ((byte)tmps[0]-48)*10+((byte)tmps[1]-48);
                  tm.Day = ((byte)tmps[3]-48)*10+((byte)tmps[4]-48);
                }
                tm.Year = ((byte)tmps[6]-48)*10+((byte)tmps[7]-48) + 30;
                if (auxb2 == 8) {auxl2=*(unsigned long*)menu_item[menu_pos]->var_point;} else {auxl2=*(unsigned long*)menu_item[menu_pos]->var_point + millis()/1000;}
                tm.Hour = hour(auxl2);
                tm.Minute = minute(auxl2);
                tm.Second = second(auxl2);
                if (auxb2 == 8) {*(unsigned long*)menu_item[menu_pos]->var_point = makeTime(tm);} else {*(unsigned long*)menu_item[menu_pos]->var_point = makeTime(tm) - millis()/1000;}              
            }
            if (auxb2 == 9 || auxb2 == 11) {
                tm.Hour = ((byte)tmps[0]-48)*10+((byte)tmps[1]-48);
                tm.Minute = ((byte)tmps[3]-48)*10+((byte)tmps[4]-48);
                tm.Second = ((byte)tmps[6]-48)*10+((byte)tmps[7]-48);
                if (auxb2 == 9) {auxl2=*(unsigned long*)menu_item[menu_pos]->var_point;} else {auxl2=*(unsigned long*)menu_item[menu_pos]->var_point + millis()/1000;}
                tm.Day = day(auxl2);
                tm.Month = month(auxl2);
                tm.Year = year(auxl2)-1970;
                if (auxb2 == 9) {*(unsigned long*)menu_item[menu_pos]->var_point = makeTime(tm);} else {*(unsigned long*)menu_item[menu_pos]->var_point = makeTime(tm) - millis()/1000;}
            }
            if (menu_item[menu_pos]->parent_id > -1) {
              menu_pos = menu_item[menu_pos]->parent_id;
              prev = this->cnt_prev();
              next = this->cnt_next();
              while  (row_pos < prev && (row_pos + next) < Display_rows) {row_pos++;}         
            }             
          } else {
            tmps[input_pos_x] = in_char[input_pos_y];
            input_pos_x++;
            if (auxb2 == 8 || auxb2 == 10) {
              if (input_pos_x == 2) {tmps[input_pos_x] = 46;input_pos_x++;}
              if (input_pos_x == 5) {tmps[input_pos_x] = 46;input_pos_x++;}
            }
            if (auxb2 == 9 || auxb2 == 11) {
                if (input_pos_x == 2) {tmps[input_pos_x] = 58;input_pos_x++;}
                if (input_pos_x == 5) {tmps[input_pos_x] = 58;input_pos_x++;}
            }
            }
            tmps[input_pos_x] = '\0';
            input_pos_y = sizeof(in_char) - 2;
            input_pos_y = move_input_pos_y(input_pos_y, menu_item[menu_pos]->var_point_type, 1);                        
        break;
      }
      menu_draw();
      break;
    case 2:  // "Up" button is pressed
      if (menu_item[menu_pos]->action_type < 4) {
        if (prev > 0) {
          menu_pos--; 
        }
        if (row_pos > 0) {
          row_pos--;
        }
      } else {
        if (menu_item[menu_pos]->action_type == 4) {
          if (list_pos > 0) {list_pos--;}
        }
        if (menu_item[menu_pos]->action_type == 5) {
          if (rotary) {
            input_pos_y = move_input_pos_y(input_pos_y, menu_item[menu_pos]->var_point_type, 0);
          } else {
            input_pos_y = move_input_pos_y(input_pos_y, menu_item[menu_pos]->var_point_type, 1);
          }
        }
      }
      runn_txt_init();
      menu_draw();
      break;
    case 3: // "Down" button is pressed
      if (menu_item[menu_pos]->action_type < 4) {
        if (next > 0) {
          menu_pos++;
        }
        if (row_pos < Display_rows - 1) {
          row_pos++; 
        }
      } else {
        if (menu_item[menu_pos]->action_type == 4) {
          if (list_pos < (list_max_id - 1)) {list_pos++;}
        }
        if (menu_item[menu_pos]->action_type == 5) {
          if (rotary) { 
            input_pos_y = move_input_pos_y(input_pos_y, menu_item[menu_pos]->var_point_type, 1);
          } else {
            input_pos_y = move_input_pos_y(input_pos_y, menu_item[menu_pos]->var_point_type, 0);
          }
        }
      }
      runn_txt_init();
      menu_draw();
      break;
    case 4: // "Left" button is pressed
      if (menu_item[menu_pos]->parent_id > -1) {
          menu_pos = menu_item[menu_pos]->parent_id;
          menu_init();
          prev = this->cnt_prev();
          next = this->cnt_next();
          while  (row_pos < prev && (row_pos + next) < Display_rows) {row_pos++;}
      } else {
        last_keypress = millis() - (unsigned long) sleep_time * 1000;
      }
    menu_draw();
    break;
  }
  }
  //if (sleep==1 && auxb!=0) {sleep=0;menu_pos = 0;menu_init();}  
}

void Menu::get_val() {
          switch (auxi1) {
            case 0:
              s = String(*(bool*)menu_item[auxb1]->var_point);
            break;
            case 1:
              s = String(*(byte*)menu_item[auxb1]->var_point);
            break;
            case 2:
              s = String(*(int*)menu_item[auxb1]->var_point);
            break;
            case 3:
              s = String(*(unsigned int*)menu_item[auxb1]->var_point);
            break;
            case 4:
              s = String(*(long*)menu_item[auxb1]->var_point);
            break;
            case 5:
              s = String(*(unsigned long*)menu_item[auxb1]->var_point);
            break;
            case 6:
              s = String(*(float*)menu_item[auxb1]->var_point, int(menu_item[auxb1]->var_point_type / 20));
            break;
            case 7:
              s = String(*(double*)menu_item[auxb1]->var_point, int(menu_item[auxb1]->var_point_type / 20));
            break;
          }
          if (auxi1 == 8 || auxi1 == 10) {
            if (auxi1 == 8) {auxl = *(unsigned long*)menu_item[auxb1]->var_point;} else {auxl = *(unsigned long*)menu_item[auxb1]->var_point + millis()/1000;}
            s="";
            if (date_DMY) {
              if (day(auxl)<10) {s+="0";s+=day(auxl);} else {s+=day(auxl);}
              s+=".";
            }
            if (month(auxl)<10) {s+="0";s+=month(auxl);} else {s+=month(auxl);}
            s+=".";
            if (!date_DMY) {
              if (day(auxl)<10) {s+="0";s+=day(auxl);} else {s+=day(auxl);}
              s+=".";
            }  
            s+=String(year(auxl)).substring(2);             
          }
          if (auxi1 == 9 || auxi1 == 11) {
            if (auxi1 == 9) {auxl = *(unsigned long*)menu_item[auxb1]->var_point;} else {auxl = *(unsigned long*)menu_item[auxb1]->var_point + millis()/1000;}
            s="";
            if (hour(auxl)<10) {s+="0";s+=hour(auxl);} else {s+=hour(auxl);}
            s+=":";
            if (minute(auxl)<10) {s+="0";s+=minute(auxl);} else {s+=minute(auxl);}
            s+=":";
            if (second(auxl)<10) {s+="0";s+=second(auxl);} else {s+=second(auxl);} 
          }  
}


/*******************************************************************************/
/************** draw DRAW ******************************************************/
/*******************************************************************************/
    // action_type;
    //0 - menu
    //1 - menu + display value
    //2 - menu + set int to 1/0 - thick [v]/[ ]
    //3 - menu + set int to 1/0 - ON/OFF
    //4 - display events: 1st row - datetime, 2nd row - event description + int value
    //5 - set value
void Menu::menu_draw() {
  refresh_time = millis() + (unsigned long) menu_refresh;
  
  prev = this->cnt_prev();
  next = this->cnt_next();

  if (next == 0) {if (prev < Display_rows - 1) {row_pos = prev;} else {row_pos = Display_rows - 1;}}
  auxb = row_pos + next + 1;
  if (auxb > Display_rows) {auxb = Display_rows;}
  auxb1 = menu_pos-row_pos;
  
  if (menu_item[auxb1]->action_type < 4) {
      for(auxi = 0; auxi < auxb; auxi++) {
      auxb1 = menu_pos-row_pos+auxi;
      auxi1 = menu_item[auxb1]->var_point_type % 20;
      switch (menu_item[auxb1]->action_type) {
        //------------
        case 0: //menu
          strcpy_P(tt_text, (char*)pgm_read_word(&tt_point[menu_item[auxb1]->tt_id]));
          lcdprint(auxi, 1, tt_text);
        break;
        //----------------------------
        case 1: //menu + display value
          strcpy_P(tt_text, (char*)pgm_read_word(&tt_point[menu_item[auxb1]->tt_id]));
          get_val();
          lcdprint(auxi, 1, tt_text, Display_columns - 3 - s.length());
          this->LCD->setCursor(Display_columns - 1 - s.length(), auxi);
          this->LCD->print(s);
        break;
        //---------------------------------------------
        case 2: //menu + set int to 1/0 - thick [v]/[ ]
          strcpy_P(tt_text, (char*)pgm_read_word(&tt_point[menu_item[auxb1]->tt_id]));
          lcdprint(auxi, 1, tt_text, Display_columns - 6);
          this->LCD->setCursor(Display_columns - 5, auxi);
          this->LCD->print(" ");
          if (*(bool*)menu_item[auxb1]->var_point==0) {
            this->LCD->print("[ ]");
          } else {
            this->LCD->print("[");
            this->LCD->write(byte(3));
            this->LCD->print("]");
          }
        break;
        //--------------------------------------
        case 3: //menu + set int to 1/0 - ON/OFF
          strcpy_P(tt_text, (char*)pgm_read_word(&tt_point[menu_item[auxb1]->tt_id]));
          if (*(bool*)menu_item[auxb1]->var_point==0) {
            s = Off_label;
          } else {
            s = On_label;
          }
          lcdprint(auxi, 1, tt_text, Display_columns - 3 - s.length());
          this->LCD->setCursor(Display_columns - 1 - s.length(), auxi);
          this->LCD->print(s);
        break;
      }
    }
    while (auxb < Display_rows) {
      this->LCD->setCursor(0, auxb);
      this->LCD->print(empty_str);
      auxb++;
    }
  } else {
    //-----------------------------------------------------------------------------------
    //display events: 1st row - datetime, 2nd row - event description + int value
    if (menu_item[auxb1]->action_type == 4) {
      //Serial.print("list_pos:");Serial.print(list_pos);Serial.print(", list_max_id:");Serial.print(list_max_id);
      s="";
      if (date_DMY) { //DD.MM.YY   
        s += day(list_item[list_pos]->item_time);
        s += ".";
      }
      s += month(list_item[list_pos]->item_time);
      s += ".";
      if (!date_DMY) { //DD.MM.YY
        s += day(list_item[list_pos]->item_time);
        s += ".";
      }
      s += String(year(list_item[list_pos]->item_time)).substring(2);
      auxi = s.length();      
      lcdprint(0, 0, s, auxi);
      //lcdprint(0, 0, s, 7);
      s="";
      s += hour(list_item[list_pos]->item_time);
      s += ":";
      if (minute(list_item[list_pos]->item_time)<10) {s += "0";}
      s += minute(list_item[list_pos]->item_time);
      s += ":";
      if (second(list_item[list_pos]->item_time)<10) {s += "0";}
      s += second(list_item[list_pos]->item_time);

      lcdprint(0, auxi + 1, s, Display_columns - 2 - auxi);
      //lcdprint(0, auxi + 1, s, 7);

      strcpy_P(tt_text, (char*)pgm_read_word(&tt_point[list_item[list_pos]->item_tt_id]));
      s = String(list_item[list_pos]->item_val);
      lcdprint(1, 0, tt_text, Display_columns - 2 - s.length());
      this->LCD->setCursor(Display_columns - 1 - s.length(), 1);
      this->LCD->print(s);
      if (Display_rows > 2) {
        this->LCD->setCursor(0, 2);
        this->LCD->print(empty_str);
        this->LCD->setCursor(0, 3);
        this->LCD->print(empty_str);
      }          
    }
    //-----------------
    //Set value
    
    if (menu_item[auxb1]->action_type == 5) {
        auxb1 = menu_pos;
        auxi1 = menu_item[auxb1]->var_point_type % 20;
        get_val();
        strcpy_P(tt_text, (char*)pgm_read_word(&tt_point[menu_item[menu_pos]->tt_id]));
        lcdprint(0, 0, tt_text, Display_columns - 2 - s.length());
        this->LCD->setCursor(Display_columns - 1 - s.length(), 0);
        this->LCD->print(s);
        lcdprint(1, 0, New_val);
        this->LCD->setCursor(New_val.length() + 1, 1);
        this->LCD->print(tmps);
        this->LCD->setCursor(New_val.length() + 1 + input_pos_x, 1);
        if ((byte) in_char[input_pos_y] ==82) {this->LCD->write(byte(4));} else {this->LCD->print(in_char[input_pos_y]);}
        if (Display_rows > 2) {
          this->LCD->setCursor(0, 2);
          this->LCD->print(empty_str);
          this->LCD->setCursor(0, 3);
          this->LCD->print(empty_str);
        }
        this->LCD->setCursor(New_val.length() + 1 + input_pos_x, 1);
        this->LCD->blink(); 
    }
  }
  draw_cursors();
} 

void Menu::lcdprint(byte row, byte col, String str, byte chrs) {
  if (chrs==255) {chrs = Display_columns - 1;}
  this->LCD->setCursor(col, row);
  if (str.length() <= chrs) {
    this->LCD->print(str);
    byte mb = 0;
    while (mb <= (chrs - str.length())) {this->LCD->print(" "); mb++;}
  } else {
    this->LCD->print(str.substring(row_sh[row], row_sh[row] + chrs));
    this->LCD->print(" ");
    switch (row_dir[row]) {
     /* case 1:
        row_dir[row] = 2;
      break;
      case 2:
        if (row_sh[row] >= str.length() - chrs) {
          row_sh[row] = str.length() - chrs;
          row_dir[row] = 3;
        } else {
          row_sh[row]++;
        }
      break;
      case 3:
        row_dir[row] = 4;
      break;
      case 4:
        if (row_sh[row] == 0) {
          row_dir[row] = 1;
        } else {
          row_sh[row]--;
        }
      break;*/
      case 1:
        if (row_sh[row] >= str.length() - chrs) {
          row_sh[row] = str.length() - chrs;
          row_dir[row] = 2;
        } else {
          row_sh[row]++;
        }
      break;
      case 2:
        if (row_sh[row] == 0) {
          row_dir[row] = 1;
        } else {
          row_sh[row]--;
        }
      break;
    }
  }
}

byte Menu::move_input_pos_y(byte input_pos_y, byte var_type, bool fwd) {
  auxo = false;
  auxi1 = var_type % 20;
  while (!auxo) {
    if (input_pos_y == 0 && !fwd) {input_pos_y = sizeof(in_char) - 1;}
    if (fwd) {input_pos_y++;} else {input_pos_y--;}
    if (input_pos_y > sizeof(in_char) - 2) {input_pos_y = 0;}
    if (auxi1 == 0) {
      if (input_pos_y == 0) {auxo = true;};
      if (input_pos_y == 1) {auxo = true;};
      if (input_pos_y == sizeof(in_char) - 2) {auxo = true;};
      if (input_pos_x > 0) {input_pos_y = sizeof(in_char) - 2;auxo = true;}; //Maximum of input chars is 1
    }
    if (auxi1 == 1 || auxi1 == 3 || auxi1 == 5) {//unsigned
      if (auxi1 == 1) {auxb2 = 2;}
      if (auxi1 == 3) {auxb2 = 4;}
      if (auxi1 == 5) {auxb2 = 9;}
      if (input_pos_y < 10) {auxo = true;}; //numbers 0123456789
      if (input_pos_x > 0 && input_pos_y == sizeof(in_char) - 2) {auxo = true;}; //from second char Enter will be visible 
      if (input_pos_x > auxb2) {input_pos_y = sizeof(in_char) - 2;auxo = true;};       
    }
    if (auxi1 == 2 || auxi1 == 4) {//signed
      if (auxi1 == 2) {auxb2 = 4;} else {auxb2 = 9;}
      if (input_pos_y < 10) {auxo = true;}; //numbers 0123456789
      if (input_pos_y == 10 && input_pos_x == 0) {auxo = true;}; //first char can be "-"
      if (input_pos_x > 0 && input_pos_y == sizeof(in_char) - 2) {auxo = true;}; //from second char Enter will be visible 
      if (input_pos_x > auxb2) {input_pos_y = sizeof(in_char) - 2;auxo = true;};       
    }
    if (auxi1 == 6 || auxi1 == 7) {
      if (input_pos_y < 10) {auxo = true;}; //numbers 0123456789
      if (input_pos_y == 10 && input_pos_x == 0) {auxo = true;}; //first char can be "-"
      auxo1=false;for(auxi=0;auxi<19;auxi++){if((byte)tmps[auxi]==46){auxo1=true;}}
      if (input_pos_y == sizeof(in_char) - 3 && !auxo1 && input_pos_x > 0) {auxo = true;} //from second char decimal places separator "." can writen
      if (input_pos_x > 0 && input_pos_y == sizeof(in_char) - 2) {auxo = true;}; //from second char Enter will be visible 
      if (input_pos_x > 9) {input_pos_y = sizeof(in_char) - 2;auxo = true;}; //Maximum of input chars is 10       
    }
    if (auxi1 > 7 && auxi1 < 12) {
      if (auxi1 == 8 || auxi1 == 10) {
        if (date_DMY) { //DD.MM.YY
          if (input_pos_x == 0 && input_pos_y < 4) {auxo = true;}; //numbers 0123
          if (input_pos_x == 3 && input_pos_y < 2) {auxo = true;}; //numbers 01
        } else {
          if (input_pos_x == 0 && input_pos_y < 2) {auxo = true;}; //numbers 01
          if (input_pos_x == 3 && input_pos_y < 4) {auxo = true;}; //numbers 0123
        }     
        if (input_pos_x == 6 && input_pos_y < 10) {auxo = true;}; //numbers 0123456789  
      }
      if (auxi1 == 9 || auxi1 == 11) {
        if (input_pos_x == 0 && input_pos_y < 3) {auxo = true;}; //numbers 012
        if (input_pos_x == 3 && input_pos_y < 6) {auxo = true;}; //numbers 012345
        if (input_pos_x == 6 && input_pos_y < 6) {auxo = true;}; //numbers 012345
      }
      if (input_pos_x == 1 && input_pos_y < 10) {auxo = true;}; //numbers 0123456789  
      if (input_pos_x == 4 && input_pos_y < 10) {auxo = true;}; //numbers 0123456789   
      if (input_pos_x == 7 && input_pos_y < 10) {auxo = true;}; //numbers 0123456789
      if (input_pos_x > 7) {input_pos_y = sizeof(in_char) - 2;auxo = true;}; //Maximum of input chars is 10
    }
    if (input_pos_x == 0 && input_pos_y == sizeof(in_char) - 2) {auxo = false;}
  }
  return input_pos_y;
}
