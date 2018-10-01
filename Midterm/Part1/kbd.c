/********************************************************************
Copyright 2010-2017 K.C. Wang, <kwang@eecs.wsu.edu>
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/

#include "keymap"
#define KCNTL 0x00
#define KSTAT 0x04
#define KDATA 0x08
#define KCLK  0x0C
#define KISTA 0x10

typedef volatile struct kbd{
  char *base;
  char buf[128];
  int head, tail, data, room;
}KBD;

volatile KBD kbd;

u8 keysmap[256] = {0};

void keydown(u8 keycode) {
  keysmap[keycode] = 1;
}

void keyup(u8 keycode) {
  keysmap[keycode] = 0;
}

int iskeydown(u8 keycode) {
  if (keysmap[keycode] == 1) {
    return 1;
  }
  else {
    return 0;
  }
}


int kbd_init()
{
  KBD *kp = &kbd;
  kp->base = (char *)0x10006000;
  *(kp->base + KCNTL) = 0x14;
  *(kp->base + KCLK)  = 8;
  kp->head = kp->tail = 0;
  kp->data = 0; kp->room = 128;
}

/*
    1. KBD structure: no need for change
    2. kgetc(): rewrite kgetc to let process sleep for data if there
    are no keys in the input buffer. in order to prevent race
    conditions between the process and the interrupt handler, the
    process disables interrupts first. Then it checks the data variable
    and modifies the input buffer with interrupts disabled, but it must
    enable interrupts before going to sleep. The modified kgetc()
    function is:

    int kgetc() {
      char c;
      KBD *kp = &kbd;
      while(1) {
          lock();     // disable IRQ interrupts
          if (kp->data == 0) {  // check data with IRQ disabled
              unlock();         // enable IRQ interrupts
              ksleep(&kp->data);  // sleep for data
          }
      }
      c = kp->buf[kp->tail++];
      kp->tail %= BUFSIZE;
      kp->data--;
      unlock();   // enable IRQ interrupts
      return c;
    }


    3. Rewrite kbd handler to wake up sleeping procs, if any, that are
    waiting for data. Since process cannot interfere with interrupt
    handler, there is no need to protect the data variables inside the
    interrupt handler.

    kbd_handler() {
        struct KBD *kp = &kbd;
        scode = *(kp->base+KDATA)
    }

*/


void kbd_handler()
{
  u8 scode, c;
  KBD *kp = &kbd;
  color = CYAN;
  scode = *(kp->base + KDATA);

  if (scode == 0xF0) {
    //return;
  }

  if (iskeydown(scode) == 1) {
    keyup(scode);
    return;
  }

  keydown(scode);

  c = unsh[scode];

  if (c >= 'a' && c <= 'z')
     printf("kbd interrupt: c=%s %c\n", c, c);
  kp->buf[kp->head++] = c;
  kp->head %= 128;
  kp->data++; kp->room--;
}

int kgetc()
{
  char c;
  KBD *kp = &kbd;

  unlock();
  while(kp->data == 0);   // busy-wait for data

  lock();
  c = kp->buf[kp->tail++];
  kp->tail %= 128;
  kp->data--; kp->room++;
  unlock();
  return c;
}

int kgets(char s[ ])
{
  char c;
  while( (c = kgetc()) != '\r'){
    *s++ = c;
  }
  *s = 0;
  return strlen(s);
}
