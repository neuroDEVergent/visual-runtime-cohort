#pragma once

struct Keys {
  bool ZERO;
  bool ONE;
  bool TWO;
  bool THREE;
  bool FOUR;
  bool FIVE;
};

struct Input {
  double         MOUSE_X;
  double    LAST_MOUSE_X;
  double         MOUSE_Y;
  double    LAST_MOUSE_Y;
  double        SCROLL_X;
  double        SCROLL_Y = 1.0f;
  unsigned int  SCREEN_WIDTH;
  unsigned int  SCREEN_HEIGHT;
  bool               ONE;
  bool               TWO;
  bool             THREE;
  bool              FOUR;
  bool              FIVE;
  bool       LCLICK_DOWN;  // Button being held down
  bool       RCLICK_DOWN;
  bool    LCLICK_PRESSED;  // Signal that button has been clicked (signaled once per frame)
  bool    RCLICK_PRESSED;
  bool   LCLICK_RELEASED;  // Button is NOT being held down / it's released
  bool   RCLICK_RELEASED;
  Keys              KEYS;
};
