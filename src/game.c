#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"
#include <stdio.h>
#include <string.h> // for //memset() declaration
#include <math.h>

void
setup_tim17 ()
{
  // Set this to invoke the ISR 100 times per second.
  RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
  TIM17->PSC = 3200 - 1;
  TIM17->ARR = 1600 - 1;
  TIM17->DIER |= 1;
  TIM17->CR1 |= 1;
  NVIC->ISER[0] |= (1 << TIM17_IRQn);
}
void
setup_tim2 ()
{
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  TIM2->PSC = 2 - 1;
  //TIM2->ARR = 1600 - 1;
  TIM2->DIER |= 1;
  TIM2->CR1 |= 1;
  NVIC->ISER[0] |= (1 << TIM2_IRQn);
}

void
setup_portb ()
{
  // Enable Port B.
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
  // Set PB0-PB3 for output.
  GPIOB->MODER &= ~0xFF;
  GPIOB->MODER |= 0x55;
  // Set PB4-PB7 for input and enable pull-down resistors.
  GPIOB->MODER &= ~0xFF00;
  GPIOB->PUPDR &= ~0xFF00;
  GPIOB->PUPDR |= 0xAA00;
  // Turn on the output for the lower row.
  GPIOB->BSRR = 0xf0000 | (1 << 0);
}

char
check_key ()
{
  // If the '1' key is pressed, return '1'
  if (((GPIOB->IDR & (1 << 4)) >> 4) == 1)
    {
      return '1';
    }
  // If the '2' key is pressed, return '2'
  else if (((GPIOB->IDR & (1 << 5)) >> 5) == 1)
    {
      return '2';
    }
  // If the '3' key is pressed, return '3'
  else if (((GPIOB->IDR & (1 << 6)) >> 6) == 1)
    {
      return '3';
    }
  // If the 'A' key is pressed, return 'A'
  else if (((GPIOB->IDR & (1 << 7)) >> 7) == 1)
    {
      return 'A';
    }
  // Otherwise, return 0
  return 0;
}

void
setup_spi1 ()
{
  // Use setup_spi1() from lab 8.
  //Enable the RCC clock to GPIO Port A.
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
  //Configure pins PA4, PA5, and PA7 for alternate function 0.
  //Configure pins PA2 and PA3 to be outputs.
  GPIOA->MODER &= ~0xCFF0;
  GPIOA->MODER |= 0x8A00 | 0x50;
  //Enable the RCC clock to SPI1.
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  //Configure SPI1 for Master/Bidimode/BidiOE, but set the baud rate as high as possible (make the SCK divisor as small as possible.)
  SPI1->CR1 &= ~SPI_CR1_BR;
  SPI1->CR1 |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
  SPI1->CR1 |= SPI_CR1_MSTR;
  //Set SSOE/NSSP as you did for SPI2, but leave the word size set to 8-bit (the default).
  SPI1->CR2 = SPI_CR2_SSOE | SPI_CR2_NSSP | SPI_CR2_DS_0 | SPI_CR2_DS_1
      | SPI_CR2_DS_2;
  //Enable the SPI port.
  SPI1->CR1 |= SPI_CR1_SPE;
}

// Copy a subset of a large source picture into a smaller destination.
// sx,sy are the offset into the source picture.
void
pic_subset (Picture *dst, const Picture *src, int sx, int sy)
{
  int dw = dst->width;
  int dh = dst->height;
  if (dw + sx > src->width)
    for (;;)
      ;
  if (dh + sy > src->height)
    for (;;)
      ;
  for (int y = 0; y < dh; y++)
    for (int x = 0; x < dw; x++)
      dst->pix2[dw * y + x] = src->pix2[src->width * (y + sy) + x + sx];
}

// Overlay a picture onto a destination picture.
// xoffset,yoffset are the offset into the destination picture that the
// source picture is placed.
// Any pixel in the source that is the 'transparent' color will not be
// copied.  This defines a color in the source that can be used as a
// transparent color.
void
pic_overlay (Picture *dst, int xoffset, int yoffset, const Picture *src,
             int transparent)
{
  for (int y = 0; y < src->height; y++)
    {
      int dy = y + yoffset;
      if (dy < 0 || dy >= dst->height)
        continue;
      for (int x = 0; x < src->width; x++)
        {
          int dx = x + xoffset;
          if (dx < 0 || dx >= dst->width)
            continue;
          unsigned short int p = src->pix2[y * src->width + x];
          if (p != transparent)
            dst->pix2[dy * dst->width + dx] = p;
        }
    }
}

void
setup_tim1 ()
{
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
  GPIOA->MODER &= ~0xFF0000;
  GPIOA->MODER |= 0xAA0000;
  GPIOA->AFR[1] |= 0x2222;
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
  TIM1->BDTR |= TIM_BDTR_MOE;
  TIM1->PSC = 0;
  TIM1->ARR = 2399;
  TIM1->CCMR1 |= 0x6060;
  TIM1->CCMR2 |= 0x6060 + (1 << 11);
  TIM1->CCER |= 0x1111;
  TIM1->CR1 |= 0x1;
}

#define N 1000
#define RATE 20000
short int wavetable[N];

void
init_wavetable (void)
{
  for (int i = 0; i < N; i++)
    {
      wavetable[i] = 32767 * sin (2 * M_PI * i / N);
    }
}

struct
{
  float frequency;
  uint16_t duration;

} song[] =
  {
    { 523.25, 1000 }, // C5
        { 587.33, 1000 }, // D5
        { 659.25, 1000 }, // E5
        { 698.46, 1000 }, // F5
        { 783.99, 1000 }, // G5
        { 880.00, 1000 }, // A6
        { 987.77, 1000 }, // B6
        { 1046.50, 1000 }, // C6
    };

int volume = 2048;
int stepa = 0;
int stepb = 0;
int stepc = 0;
int stepd = 0; // not used
int offseta = 0;
int offsetb = 0;
int offsetc = 0;
int offsetd = 0; // not used

void
set_freq_a (float f)
{
  stepa = f * 1000 / 20000 * (1 << 16);
  if (f == 0)
    {
      offseta = 0;
      stepa = 0;
    }
}

void
set_freq_b (float f)
{
  stepb = f * 1000 / 20000 * (1 << 16);
  if (f == 0)
    {
      offsetb = 0;
      stepb = 0;
    }
}

void
set_freq_c (float f)
{
  stepc = f * 1000 / 20000 * (1 << 16);
  if (f == 0)
    {
      offsetc = 0;
      stepc = 0;
    }
}

const char font[] =
  { [' '] = 0x00, ['0'] = 0x3f, ['1'] = 0x06, ['2'] = 0x5b, ['3'] = 0x4f, ['4'
      ] = 0x66, ['5'] = 0x6d, ['6'] = 0x7d, ['7'] = 0x07, ['8'] = 0x7f, ['9'
      ] = 0x67, ['A'] = 0x77, ['B'] = 0x7c, ['C'] = 0x39, ['D'] = 0x5e, ['*'
      ] = 0x49, ['#'] = 0x76, ['.'] = 0x80, ['?'] = 0x53, ['b'] = 0x7c, ['r'
      ] = 0x50, ['g'] = 0x6f, ['i'] = 0x10, ['n'] = 0x54, ['u'] = 0x1c, ['s'
      ] = 0x6d, ['o'] = 0x1d, ['e'] = 0x7b, ['E'] = 0x79, ['y'] = 0x6e, ['u'
      ] = 0x1c, ['L'] = 0x38, };

char offset;
char history[16];
char display[8];
char queue[2];
int qin;
int qout;

extern const Picture background;
extern const Picture square;
extern const Picture square2;

const int border = 20;
int xmin; // Farthest to the left the center of the square can go
int xmax; // Farthest to the right the center of the square can go
int ymin; // Farthest to the top the center of the square can go
int ymax; // Farthest to the bottom the center of the square can go
int x, y; // Center of square
const int ypos = 173; //NEW                         //new positions
const int x1 = 55;
const int x2 = 98;
const int x3 = 140;
const int x4 = 182;
int lastx = 55;
int enabled = 0; //NEW                         //if box is on
int started = 0; //is game started
int simonsTurn = 1;
char pattern[30];
int turn = 0; //total turns so far
int curr = 0; //current turn iteration
int score = 0;
int randomized = 0;
int inputting = 0; //if user is inputting

// This C macro will create an array of Picture elements.
// Really, you'll just use it as a pointer to a single Picture
// element with an internal pix2[] array large enough to hold
// an image of the specified size.
// BE CAREFUL HOW LARGE OF A PICTURE YOU TRY TO CREATE:
// A 100x100 picture uses 20000 bytes.  You have 32768 bytes of SRAM.
#define TempPicturePtr(name,width,height) Picture name[(width)*(height)/6+2] = { {width,height,2} }

// Create a 52x52 object to hold the square plus padding
TempPicturePtr(object, 52, 52);
TempPicturePtr(object2, 52, 52);

void
TIM17_IRQHandler (void)
{
  TIM17->SR &= ~TIM_SR_UIF;
  char sc[2];
  sprintf (sc, "%d", score);
  if (enabled == 0 && inputting == 0)
    {
      display[0] = font['s'];
      display[1] = font['C'];
      display[2] = font['0'];
      display[3] = font['r'];
      display[4] = font['E'];
      display[5] = font[' '];
      display[6] = font[(int) sc[0]];
      display[7] = font[(int) sc[1]];
      TIM1->BDTR &= ~TIM_BDTR_MOE;
    }
  char key = check_key ();
  if (started == 0 && key != 'A')
    {
      return;
    }
  else if (started == 0 && key == 'A')
    {
      display[0] = font[' '];
      display[1] = font[' '];
      display[2] = font[' '];
      display[3] = font[' '];
      display[4] = font[' '];
      display[5] = font[' '];
      display[6] = font[' '];
      display[7] = font[' '];
      memset (display, 0, 8);
      started = 1;
      char started[] = "Watch Simon!        ";
      LCD_DrawString (40, 100, 0, 65535, started, 16, 0);
      nano_wait (1000000000);
      if (randomized == 0)
        {
          srandom (TIM2->CNT);
          TIM2->CR1 &= ~1;
          randomized = 1;
        }
      score = 0;
      setup_tim1 ();
      TIM1->CR1 &= ~1;
      return;
    }

  if (simonsTurn == 1 && enabled == 0)
    {
      inputting = 1;
      //TIM1->BDTR |= TIM_BDTR_MOE;
      if (curr != turn)
        {
          key = pattern[curr];
          curr++;
        }
      else
        {
          key = random () % 4;
          switch (key)
            {
            case 0:
              key = '1';
              break;
            case 1:
              key = '2';
              break;
            case 2:
              key = '3';
              break;
            case 3:
              key = 'A';
              break;
            }
          pattern[curr] = key;
          curr++;
        }
    }
  else if (simonsTurn == 0 && enabled == 0) //player turn
    {

      if (curr < turn)
        {
          if (key != 0)
            {
              inputting = 1;
              TIM1->BDTR |= TIM_BDTR_MOE;
              //TIM7->CR1 &= ~TIM_CR1_CEN;
              //memset (display, 0, 8);
              if (key == pattern[curr])
                {
                  curr++;
                }
              else if (key != pattern[curr])
                {
                  char lose[] = "You lost! A to retry";
                  LCD_DrawString (40, 100, 0, 65535, lose, 16, 0);
                  char end[] = "Final Score:";
                  LCD_DrawString (40, 250, 0, 65535, end, 16, 0);
                  char sc[2];
                  sprintf (sc, "%d", score);
                  LCD_DrawString (160, 250, 0, 65535, sc, 16, 0);
                  started = 0;
                  curr = 0;
                  turn = 0;
                  TIM1->BDTR |= TIM_BDTR_MOE;
                  display[0] = font['y'];
                  display[1] = font['0'];
                  display[2] = font['u'];
                  display[3] = font[' '];
                  display[4] = font['L'];
                  display[5] = font['0'];
                  display[6] = font['s'];
                  display[7] = font['E'];
                  //TIM1->BDTR &= ~TIM_BDTR_MOE;

                  //srandom (TIM2->CNT);
                  return;
                }

            }

        }
      else
        {
          simonsTurn = 1;
        }

    }

  TempPicturePtr(tmp, 52, 52); // Create a temporary 52x52 image.
  TempPicturePtr(tmp2, 52, 52); // Create a temporary 52x52 image.

  //////////////////////////////////////////////////////////////entering code
  if (enabled != 0)

    {
      //memset (display, 0, 8);
      if (enabled != 1)
        {
          //nano_wait (200000000);
          TIM1->BDTR |= TIM_BDTR_MOE;
          enabled--;
        }
      else
        {
          pic_subset (tmp2, &background, lastx - tmp2->width / 2,
                      ypos - tmp2->height / 2); // Copy the background
          pic_overlay (tmp2, 0, 0, object2, 0xffff); // Overlay the object
          LCD_DrawPicture (lastx - tmp2->width / 2, ypos - tmp2->height / 2,
                           tmp2); //white
          enabled = 0;
          display[0] = font[' '];
                display[1] = font[' '];
                display[2] = font[' '];
                display[3] = font[' '];
                display[4] = font[' '];
                display[5] = font[' '];
                display[6] = font[' '];
                display[7] = font[' '];
          memset (display, 0, 8);
        }
    }
  else if (key == '1')
    {
      //TIM7->CR1 &= ~TIM_CR1_CEN;
      set_freq_a (100); // Middle 'C'
      set_freq_b (100); // The 'E' above middle 'C'
      set_freq_c (100); // The 'G' above middle 'C'
      TIM1->CR1 |= 1;
      display[0] = font['1'];
      display[1] = font[' '];
      display[2] = font[' '];
      display[3] = font[' '];
      display[4] = font[' '];
      display[5] = font[' '];
      display[6] = font[' '];
      display[7] = font[' '];
      pic_subset (tmp, &background, x1 - tmp->width / 2,
                  ypos - tmp->height / 2); // Copy the background
      pic_overlay (tmp, 0, 0, object, 0xffff); // Overlay the object

      LCD_DrawPicture (x1 - tmp->width / 2, ypos - tmp->height / 2, tmp); //black
      lastx = x1;
      enabled = 5;
    }
  else if (key == '2')
    {
      set_freq_a (300); // Middle 'C'
      set_freq_b (300); // The 'E' above middle 'C'
      set_freq_c (300); // The 'G' above middle 'C'
      TIM1->CR1 |= 1;
      display[0] = font['2'];
      display[1] = font['2'];
      display[2] = font['2'];
      display[3] = font['2'];
      display[4] = font['2'];
      display[5] = font['2'];
      display[6] = font[' '];
      display[7] = font[' '];
      pic_subset (tmp, &background, x2 - tmp->width / 2,
                  ypos - tmp->height / 2); // Copy the background
      pic_overlay (tmp, 0, 0, object, 0xffff); // Overlay the object

      LCD_DrawPicture (x2 - tmp->width / 2, ypos - tmp->height / 2, tmp); //black
      lastx = x2;
      enabled = 5;
    }
  else if (key == '3')
    {
      set_freq_a (698.46); // Middle 'C'
      set_freq_b (400); // The 'E' above middle 'C'
      set_freq_c (100); // The 'G' above middle 'C'

      TIM1->CR1 |= 1;
      display[0] = font['3'];
      display[1] = font['3'];
      display[2] = font['3'];
      display[3] = font['3'];
      display[4] = font[' '];
      display[5] = font[' '];
      display[6] = font[' '];
      display[7] = font[' '];
      pic_subset (tmp, &background, x3 - tmp->width / 2,
                  ypos - tmp->height / 2); // Copy the background
      pic_overlay (tmp, 0, 0, object, 0xffff); // Overlay the object

      LCD_DrawPicture (x3 - tmp->width / 2, ypos - tmp->height / 2, tmp); //black
      lastx = x3;
      enabled = 5;
    }
  else if (key == 'A')
    {
      TIM1->CR1 |= 1;
      display[0] = font['A'];
      display[1] = font['A'];
      display[2] = font['A'];
      display[3] = font[' '];
      display[4] = font[' '];
      display[5] = font[' '];
      display[6] = font[' '];
      display[7] = font[' '];
      pic_subset (tmp, &background, x4 - tmp->width / 2,
                  ypos - tmp->height / 2); // Copy the background
      pic_overlay (tmp, 0, 0, object, 0xffff); // Overlay the object
      LCD_DrawPicture (x4 - tmp->width / 2, ypos - tmp->height / 2, tmp); //black
      lastx = x4;
      enabled = 5;
    }

  //////////////////////////////////////////////////////////////entering code end
  if (curr > turn && enabled == 0)
    {
      if (simonsTurn == 1)
        {
          //memset (display, 0, 8);
          turn++; //maybe do after player turn?
          curr = 0;
          simonsTurn = 0;
          inputting = 0;
          //nano_wait (1000000000);
          char started[] = "Repeat the pattern! ";
          LCD_DrawString (40, 100, 0, 65535, started, 16, 0);
          TIM7->CR1 |= TIM_CR1_CEN;
        }
    }
  if (curr == turn && simonsTurn == 0 && enabled == 0)
    {
      curr = 0;
      score++;
      if (score == 25)
        {
          char win[] = "25 points? you win...";
          LCD_DrawString (40, 100, 0, 65535, win, 16, 0);
          char end[] = "Final Score:";
          LCD_DrawString (40, 250, 0, 65535, end, 16, 0);
          char sc[2];
          sprintf (sc, "%d", score);
          LCD_DrawString (160, 250, 0, 65535, sc, 16, 0);
          started = 0;
          curr = 0;
          turn = 0;
          //srandom (TIM2->CNT);
          return;
        }
      //memset (display, 0, 8);
      simonsTurn = 1;
      nano_wait (1000000000);
      char started[] = "Watch Simon!        ";
      LCD_DrawString (40, 100, 0, 65535, started, 16, 0);
      //memset (display, 0, 8);
      nano_wait (1000000000);
    }
//  TIM2->CR1 &= ~1;
//  TIM2->CR1 |= 1;
//  TIM7->CR1 &= ~1;
//  TIM7->CR1 |= 1;
//  TIM17->CR1 &= ~1;
//  TIM17->CR1 |= 1;
}

void
show_digit ()
{
  //Read the 3-bit value from offset and set that value on pins PC[10:8]
  int off = offset & 7;
  GPIOC->ODR = (off << 8) | display[off];
  //Look up the array element display[offset] and output that value to PC[7:0]

}

void
enable_ports ()
{
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN + RCC_AHBENR_GPIOCEN;
  GPIOB->MODER &= ~0xFFFF;
  GPIOB->MODER |= 0x55;
  GPIOB->PUPDR &= ~0xFF00;
  GPIOB->PUPDR |= 0xAA00;
  GPIOC->MODER &= ~0x3FFFFF;
  GPIOC->MODER |= 0x155555;
}

int
get_cols ()
{
  return (GPIOB->IDR >> 4) & 0xf;
}

void
insert_queue (int n)
{
  queue[qin] = n | 0x80;
  qin = !qin;
}

void
setup_tim7 ()
{
  RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
  TIM7->CR1 &= ~TIM_CR1_DIR;
  TIM7->PSC = 50 - 1;
  TIM7->ARR = 1000 - 1;
  TIM7->CCR3 = 1000 - 1;
  TIM7->DIER |= TIM_DIER_UIE;
  TIM7->CR1 |= TIM_CR1_CEN;
  NVIC->ISER[0] |= (1 << TIM7_IRQn);
}

void
TIM7_IRQHandler (void)
{
  TIM7->SR &= ~TIM_SR_UIF;             // acknowledge bit
  show_digit ();
  //int cols = get_cols ();
  //update_hist (cols);
  offset = (offset + 1) & 0x7; // count 0 ... 7 and repeat
}
void
set_row ()
{
  int row = offset & 3;
  GPIOB->BSRR = 0xf0000 | (1 << row);
}
void
update_hist (int cols)
{
  int row = offset & 3;
  for (int i = 0; i < 4; i++)
    {
      history[4 * row + i] = (history[4 * row + i] << 1) + ((cols >> i) & 1);
      if (history[4 * row + i] == 0x1)
        {
          insert_queue (4 * row + i);
        }
    }
}

int
main (void)
{
  setup_portb ();
  setup_spi1 ();
  LCD_Init ();
  enable_ports ();
  setup_tim7 ();
  init_wavetable ();
  set_freq_a (261.626); // Middle 'C'
  set_freq_b (329.628); // The 'E' above middle 'C'
  set_freq_c (391.996); // The 'G' above middle 'C'

  //make pattern list all -1
  for (int i = 0; i < (sizeof(pattern) / sizeof(pattern[0])); i++)
    {
      pattern[i] = -1;
    }

  // Draw the background.
  LCD_DrawPicture (0, 0, &background);

  char title[] = "Simon Says: repeat the pattern";
  LCD_DrawString (0, 50, 0, 65535, title, 16, 0);

  char start[] = "Press A to begin";
  LCD_DrawString (40, 100, 0, 65535, start, 16, 0);

  // Set all pixels in the object to white.
  for (int i = 0; i < 52 * 52; i++)
    object->pix2[i] = 0xffff;

  // Center the 55x55 square into center of the 52x52 object.
  // Now, the 55x55 square has 5-pixel white borders in all directions.
  pic_overlay (object, 5, 5, &square, 0xffff);

  for (int i = 0; i < 52 * 52; i++)
    object2->pix2[i] = 0xffff;
  pic_overlay (object2, 5, 5, &square2, 0xffff);

  // Initialize the game state.
  xmin = border + square.width / 2;
  xmax = background.width - border - square.width / 2;
  ymin = border + square.width / 2;
  ymax = background.height - border - square.height / 2;
  x = (xmin + xmax) / 2; // Center of square
  y = ymin;

  setup_tim17 ();
  setup_tim2 ();
}
