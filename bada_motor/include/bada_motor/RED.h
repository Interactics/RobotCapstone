/*
RED.h
2015-11-18
Public Domain
*/

#ifndef RED_H
#define RED_H

#include <stdio.h>
#include <stdlib.h>

#include <pigpiod_if2.h>

#include <bada_motor/RED.h>

typedef void (*RED_CB_t)(int);

struct _RED_s;

typedef struct _RED_s RED_t;

#define RED_MODE_DETENT 0
#define RED_MODE_STEP   1

/*

RED starts a rotary encoder on Pi pi with GPIO gpioA,
GPIO gpioB, mode mode, and callback cb_func.  The mode
determines whether the four steps in each detent are
reported as changes or just the detents.

If cb_func in not null it will be called at each position
change with the new position.

The current position can be read with RED_get_position and
set with RED_set_position.

Mechanical encoders may suffer from switch bounce.
RED_set_glitch_filter may be used to filter out edges
shorter than glitch microseconds.  By default a glitch
filter of 1000 microseconds is used.

At program end the rotary encoder should be cancelled using
RED_cancel.  This releases system resources.

*/

RED_t *RED                   (int pi,
                              int gpioA,
                              int gpioB,
                              int mode,
                              RED_CB_t cb_func);

void   RED_cancel            (RED_t *renc);

void   RED_set_glitch_filter (RED_t *renc, int glitch);

void   RED_set_position      (RED_t *renc, int position);

int    RED_get_position      (RED_t *renc);


/* PRIVATE ---------------------------------------------------------------- */

struct _RED_s
{
   int pi;
   int gpioA;
   int gpioB;
   RED_CB_t cb;
   int cb_id_a;
   int cb_id_b;
   int levA;
   int levB;
   int oldState;
   int glitch;
   int mode;
   int step;
};

/*

             +---------+         +---------+      0
             |         |         |         |
   A         |         |         |         |
             |         |         |         |
   +---------+         +---------+         +----- 1

       +---------+         +---------+            0
       |         |         |         |
   B   |         |         |         |
       |         |         |         |
   ----+         +---------+         +---------+  1

*/

static int transits[16]=
{
/* 0000 0001 0010 0011 0100 0101 0110 0111 */
      0,  -1,   1,   0,   1,   0,   0,  -1,
/* 1000 1001 1010 1011 1100 1101 1110 1111 */
     -1,   0,   0,   1,   0,   1,  -1,   0
};

static void _cb(
   int pi, unsigned gpio, unsigned level, uint32_t tick, void *user)
{
   RED_t *self = (RED_t*)user;
   int newState, inc, detent;

   if (level != PI_TIMEOUT)
   {
      if (gpio == self->gpioA)
         self->levA = level;
      else
         self->levB = level;

      newState = self->levA << 1 | self->levB;

      inc = transits[self->oldState << 2 | newState];

      if (inc)
      {
         self->oldState = newState;

         detent = self->step / 4;

         self->step += inc;

         if (self->cb)
         {
            if (self->mode == RED_MODE_DETENT)
            {
               if (detent != (self->step / 4)) (self->cb)(self->step / 4);
            }
            else (self->cb)(self->step);
         }
      }
   }
}

/* PUBLIC ----------------------------------------------------------------- */

RED_t *RED(int pi, int gpioA, int gpioB, int mode, RED_CB_t cb_func)
{
   RED_t *self;

   self = (RED_t*)malloc(sizeof(RED_t));

   if (!self) return NULL;

   self->pi = pi;
   self->gpioA = gpioA;
   self->gpioB = gpioB;
   self->mode = mode;
   self->cb = cb_func;
   self->levA=0;
   self->levB=0;
   self->step = 0;

   set_mode(pi, gpioA, PI_INPUT);
   set_mode(pi, gpioB, PI_INPUT);

   /* pull up is needed as encoder common is grounded */


   set_pull_up_down(pi, gpioA, PI_PUD_UP);
   set_pull_up_down(pi, gpioB, PI_PUD_UP);

   self->glitch = 1000;

   set_glitch_filter(pi, gpioA, self->glitch);
   set_glitch_filter(pi, gpioB, self->glitch);

   self->oldState = (gpio_read(pi, gpioA) << 1) | gpio_read(pi, gpioB);

   /* monitor encoder level changes */

   self->cb_id_a = callback_ex(pi, gpioA, EITHER_EDGE, _cb, self);
   self->cb_id_b = callback_ex(pi, gpioB, EITHER_EDGE, _cb, self);

   return self;
}

void RED_cancel(RED_t *self)
{
   if (self)
   {
      if (self->cb_id_a >= 0)
      {
         callback_cancel(self->cb_id_a);
         self->cb_id_a = -1;
      }

      if (self->cb_id_b >= 0)
      {
         callback_cancel(self->cb_id_b);
         self->cb_id_b = -1;
      }

      set_glitch_filter(self->pi, self->gpioA, 0);
      set_glitch_filter(self->pi, self->gpioB, 0);

      free(self);
   }
}

void RED_set_position(RED_t *self, int position)
{
   if (self->mode == RED_MODE_DETENT)
      self->step = position * 4;
   else
      self->step = position;
}

int RED_get_position(RED_t *self)
{
   if (self->mode == RED_MODE_DETENT)
      return self->step / 4;
   else
      return self->step;
}

void RED_set_glitch_filter(RED_t *self, int glitch)
{
   if (glitch >= 0)
   {
      if (self->glitch != glitch)
      {
         self->glitch = glitch;
         set_glitch_filter(self->pi, self->gpioA, glitch);
         set_glitch_filter(self->pi, self->gpioB, glitch);
      }
   }
}

#endif




