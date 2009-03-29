/****************************************************************************
 *  menu.c
 *
 *  Genesis Plus GX menu
 *
 *  code by Softdev (March 2006), Eke-Eke (2007,2008)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ***************************************************************************/

#include "shared.h"
#include "dvd.h"
#include "font.h"
#include "file_dvd.h"
#include "file_fat.h"
#include "filesel.h"

#include "Banner_main.h"
#include "Banner_bottom.h"
#include "Banner_top.h"
#include "Background_main.h"
#include "Main_logo.h"

#include "Main_play.h"
#include "Main_load.h"
#include "Main_options.h"
#include "Main_file.h"
#include "Main_reset.h"
#include "Main_info.h"

#include "Option_ctrl.h"
#include "Option_ggenie.h"
#include "Option_sound.h"
#include "Option_video.h"
#include "Option_system.h"

#include "Load_recent.h"
#include "Load_sd.h"
#include "Load_dvd.h"

#include "Button_text.h"
#include "Button_text_over.h"
#include "Button_icon.h"
#include "Button_icon_over.h"
#include "Button_up.h"
#include "Button_down.h"
#include "Button_up_over.h"
#include "Button_down_over.h"

#ifdef HW_RVL
#include "Load_usb.h"
#include "Key_A_wii.h"
#include "Key_B_wii.h"
#include "generic_point.h"
#else
#include "Key_A_gcn.h"
#include "Key_B_gcn.h"
#endif
#include "Key_home.h"

#ifdef HW_RVL
#include <wiiuse/wpad.h>
#include <di/di.h>
#endif

/*****************************************************************************/
/*  Generic GUI structures                                                   */
/*****************************************************************************/

/* Controller inputs structure */
typedef struct
{
  u16 keys;
#ifdef HW_RVL
  struct ir_t ir;
} gui_input;

/* Item descriptor*/
typedef struct
{
  png_texture *texture; /* temporary texture data                               */
  const u8 *data;       /* pointer to png image data (items icon only)          */
  const char *text;     /* item string (items list only)                        */
  char *comment;        /* item comment                                         */
  u16 x;                /* button image or text X position (upper left corner)  */
  u16 y;                /* button image or text Y position (upper left corner)  */
  u16 w;                /* button image or text width                           */
  u16 h;                /* button image or text height                          */
} gui_item;

/* Button descriptor */
typedef struct
{
  png_texture *texture[2];  /* temporary texture datas                       */
  const u8 *image[2];       /* pointer to png image datas (default)         */
} butn_data;

/* Button descriptor */
typedef struct
{
  butn_data *data;          /* pointer to button image/texture data         */
  u16 x;                    /* button image X position (upper left corner)  */
  u16 y;                    /* button image Y position (upper left corner)  */
  u16 w;                    /* button image pixels width                    */
  u16 h;                    /* button image pixels height                   */
} gui_butn;

/* Image descriptor */
typedef struct
{
  png_texture *texture; /* temporary texture data                               */
  const u8 *data;       /* pointer to png image data                            */
  u16 x;                /* button image or text X position (upper left corner)  */
  u16 y;                /* button image or text Y position (upper left corner)  */
  u16 w;                /* button image or text width                           */
  u16 h;                /* button image or text height                          */
} gui_image;

/* Menu descriptor */
typedef struct
{
  char *title;                /* menu title                                         */
  s8 selected;                /* index of selected item                             */
  u8 shift;                   /* number of items by line                            */
  u8 offset;                  /* items list offset                                  */
  u8 max_items;               /* total number of items                              */
  u8 max_buttons;             /* total number of buttons (not necessary identical)  */
  gui_item *items;            /* menu items table                                   */
  gui_butn *buttons;          /* menu buttons table                                 */
  gui_image *frames[2];       /* top and bottom frames                              */
  gui_image *background;      /* background image                                   */
  gui_image *logo;            /* logo image                                         */
  gui_item *helper[2];        /* key helpers                                        */
  gui_butn *arrows[2];        /* items list arrows                                  */
} gui_menu;

#ifdef HW_RVL
static png_texture w_pointer = {0,0,0,0};
#endif

static gui_input m_input;

/*****************************************************************************/
/*  Generic Images                                                           */
/*****************************************************************************/
static gui_image logo_main =
{
  NULL,Main_logo,204,372,232,56
};

static gui_image logo_small =
{
  NULL,Main_logo,466,40,152,44
};

static gui_image top_frame =
{
  NULL,Banner_top,0,0,640,108
};

static gui_image bottom_frame =
{
  NULL,Banner_top,0,388,640,92
};

static gui_image main_frame =
{
  NULL,Banner_top,0,356,640,124
};

static gui_image background_center =
{
  NULL,Background_main,146,74,348,288
};

static gui_image background_right =
{
  NULL,Background_main,368,132,348,288
};

/*****************************************************************************/
/*  Shared buttons data                                                      */
/*****************************************************************************/
static butn_data arrow_up_data =
{
  {NULL,NULL},
  {Button_up,Button_up_over}
};

static butn_data arrow_down_data =
{
  {NULL,NULL},
  {Button_down,Button_down_over}
};

static butn_data button_text_data =
{
  {NULL,NULL},
  {Button_text,Button_text_over}
};

static butn_data button_icon_data =
{
  {NULL,NULL},
  {Button_icon,Button_icon_over}
};

/*****************************************************************************/
/*  Generic Arrow Buttons                                                    */
/*****************************************************************************/
static gui_butn arrow_up =
{
  &arrow_up_data,172,82,36,36
};

static gui_butn arrow_down =
{
  &arrow_down_data,172,360,36,36
};

/*****************************************************************************/
/*  Generic Items                                                            */
/*****************************************************************************/
static gui_item action_cancel =
{
#ifdef HW_RVL
  NULL,Key_B_wii,NULL,"Back to previous",10,422,28,28
#else
  NULL,Key_B_gcn,NULL,"Back to previous",10,422,28,28
#endif
};

static gui_item action_select =
{
#ifdef HW_RVL
  NULL,Key_A_wii,NULL,"Select",602,422,28,28
#else
  NULL,Key_A_gcn,NULL,"Select",602,422,28,28
#endif
};

static gui_item action_exit =
{
  NULL,Key_home,NULL,"Exit",10,388,24,24
};


/*****************************************************************************/
/*  Menu Items description                                                   */
/*****************************************************************************/

/* Audio options menu */
static gui_item items_audio[5] =
{
  {NULL,NULL,"PSG Volume:",    "Adjust PSG output level",           0,0,0,0},
  {NULL,NULL,"FM Volume:",     "Adjust FM output level",            0,0,0,0},
  {NULL,NULL,"Volume Boost:",  "Adjust general output level",       0,0,0,0},
  {NULL,NULL,"LowPass Filter:","Enable/disable sound filtering",    0,0,0,0},
  {NULL,NULL,"HQ YM2612:",     "Adjust FM emulation accuracy level",0,0,0,0}
};

/* System options menu */
static gui_item items_system[6] =
{
  {NULL,NULL,"Console Region:","Select system region",                   0,0,0,0},
  {NULL,NULL,"System Lockups:","Enable/disable original system lock-ups",0,0,0,0},
  {NULL,NULL,"System BIOS:",   "Enable/disable TMSS BIOS support",       0,0,0,0},
  {NULL,NULL,"SVP Cycles",     "Adjust SVP chip emulation speed",        0,0,0,0},
  {NULL,NULL,"Auto SRAM",      "Enable/disable automatic SRAM",          0,0,0,0},
  {NULL,NULL,"Auto STATE",     "Enable/disable automatic Freeze State",  0,0,0,0}
};

/* Video options menu */
static gui_item items_video[8] =
{
  {NULL,NULL,"Aspect Ratio:",   "Select display aspect ratio",               0,0,0,0},
  {NULL,NULL,"Display:",        "Select video mode type",                    0,0,0,0},
  {NULL,NULL,"TV mode:",        "Select video refresh rate",                 0,0,0,0},
  {NULL,NULL,"Bilinear Filter:","Enable/disable hardware filtering",         0,0,0,0},
  {NULL,NULL,"NTSC Filter:",    "Enable/disable NTSC software filtering",    0,0,0,0},
  {NULL,NULL,"Borders:",        "Enable/disable original overscan emulation",0,0,0,0},
  {NULL,NULL,"DISPLAY POSITION","Adjust display position",                   0,0,0,0},
  {NULL,NULL,"DISPLAY SIZE",    "Adjust display size",                       0,0,0,0}
};

/* Main menu */
static gui_item items_main[6] =
{
  {NULL,Main_play   ,NULL,NULL,108, 76,92,88},
  {NULL,Main_load   ,NULL,NULL,280, 72,80,92},
  {NULL,Main_options,NULL,NULL,456, 76,60,88},
  {NULL,Main_file   ,NULL,NULL,114,216,80,92},
  {NULL,Main_reset  ,NULL,NULL,282,224,76,84},
  {NULL,Main_info   ,NULL,NULL,446,212,88,96}
};

#ifdef HW_RVL
static gui_item items_load[4] =
{
  {NULL,Load_recent,NULL,"Load recent files",             276,120,88,96},
  {NULL,Load_sd    ,NULL,"Load ROM files from SDCARD",    110,266,88,96},
  {NULL,Load_usb   ,NULL,"Load ROM files from USB device",276,266,88,96},
  {NULL,Load_dvd   ,NULL,"Load ROM files from DVD",       442,266,88,96}
};
#else
static gui_item items_load[3] =
{
  {NULL,Load_recent,NULL,"Load recent files",         110,198,88,96},
  {NULL,Load_sd    ,NULL,"Load ROM files from SDCARD",276,198,88,96},
  {NULL,Load_dvd   ,NULL,"Load ROM files from DVD",   442,198,88,96}
};
#endif

static gui_item items_options[5] =
{
  {NULL,Option_system,NULL,"Configure System settings", 114,142,80,92},
  {NULL,Option_video ,NULL,"Configure Video settings",  288,150,64,84},
  {NULL,Option_sound ,NULL,"Configure Audio settings",  464,154,44,80},
  {NULL,Option_ctrl  ,NULL,"Configure Input settings",  192,286,88,92},
  {NULL,Option_ggenie,NULL,"Configure Game Genie Codes",360,282,88,96}
};


/*****************************************************************************/
/*  Menu Buttons description                                                 */
/*****************************************************************************/

/* Generic list menu */
static gui_butn buttons_generic[4] =
{
  &button_text_data,52,132,276,48},
  &button_text_data,52,188,276,48},
  &button_text_data,52,244,276,48},
  &button_text_data,52,300,276,48},
};

/* Main menu */
static gui_butn buttons_main[6] =
{
  &button_icon_data, 80, 50,148,132},
  &button_icon_data,246, 50,148,132},
  &button_icon_data,412, 50,148,132},
  &button_icon_data, 80,194,148,132},
  &button_icon_data,246,194,148,132},
  &button_icon_data,412,194,148,132}
};

/* Load Game menu */
#ifdef HW_RVL
static gui_butn buttons_load[4] =
{
  &button_icon_data,246,102,148,132},
  &button_icon_data, 80,248,148,132},
  &button_icon_data,246,248,148,132},
  &button_icon_data,412,248,148,132}
#else
static gui_butn buttons_load[3] =
{
  &button_icon_data, 80,180,148,132},
  &button_icon_data,246,180,148,132},
  &button_icon_data,412,180,148,132}
};
#endif

/* Options menu */
static gui_butn buttons_options[5] =
{
  &button_icon_data, 80,120,148,132},
  &button_icon_data,246,120,148,132},
  &button_icon_data,412,120,148,132},
  &button_icon_data,162,264,148,132},
  &button_icon_data,330,264,148,132}
};

/*****************************************************************************/
/*  Menu descriptions                                                        */
/*****************************************************************************/

/* Main menu */
static gui_menu menu_main =
{
  NULL,
  0,0,6,6,3,
  items_main,
  buttons_main,
  NULL,
  &logo_main,
  {NULL,&main_frame},
  {&action_exit,NULL},
  {NULL,NULL}
};

/* Load Game menu */
static gui_menu menu_load =
{
  "Load Game",
#ifdef HW_RVL
  0,0,4,4,3,
#else
  0,0,3,3,0,
#endif
  items_load,
  buttons_load,
  &background_center,
  &logo_small,
  {&top_frame,&bottom_frame},
  {&action_cancel, &action_select},
  {NULL, NULL}
};

/* Options menu */
static gui_menu menu_options =
{
  "Emulator Settings",
  0,0,5,5,3,
  items_options,
  buttons_options,
  &background_center,
  &logo_small,
  {&top_frame,&bottom_frame},
  {&action_cancel, &action_select},
  {NULL, NULL}
};

/* System Options menu */
static gui_menu menu_system =
{
  "System Options",
  0,0,6,4,1,
  items_system,
  buttons_generic,
  &background_right,
  &logo_small,
  {&top_frame,&bottom_frame},
  {&action_cancel, &action_select},
  {NULL, NULL}
};

/* Video Options menu */
static gui_menu menu_video =
{
  "Video Options",
  0,0,8,4,1,
  items_video,
  buttons_generic,
  &background_right,
  &logo_small,
  {&top_frame,&bottom_frame},
  {&action_cancel, &action_select},
  {NULL, NULL}
};

/* Sound Options menu */
static gui_menu menu_audio =
{
  "Sound Options",
  0,0,5,4,1,
  items_audio,
  buttons_generic,
  &background_right,
  &logo_small,
  {&top_frame,&bottom_frame},
  {&action_cancel, &action_select},
  {NULL, NULL}
};

/*****************************************************************************/
/*  Generic GUI routines                                                     */
/*****************************************************************************/
static void menu_initialize(gui_menu *menu)
{
  int i,j;
  png_texture *texture;
  gui_item *item;
  gui_butn *button;
  gui_image *image;
  butn_data *button_data;

#ifdef HW_RVL
  /* allocate wiimote pointer data (only done once) */
  if (w_pointer.data == NULL)
  {
    OpenPNGFromMemory(&w_pointer, generic_point);
  }
#endif

  /* allocate background image texture */
  if (menu->background)
  {
    texture = (png_texture *)malloc(sizeof(png_texture));
    if (texture)
    {
      image = menu->background;
      OpenPNGFromMemory(texture, image->data);
      image->texture = texture;
    }
  }

  /* allocate logo texture */
  if (menu->logo)
  {
    texture = (png_texture *)malloc(sizeof(png_texture));
    if (texture)
    {
      image = menu->logo;
      OpenPNGFromMemory(texture, image->data);
      image->texture = texture;
    }
  }

  /* allocate background elements textures */
  for (i=0; i<2; i++)
  {
    /* frames */
    if (menu->frames[i])
    {
      texture = (png_texture *)malloc(sizeof(png_texture));
      if (texture)
      {
        image = menu->frames[i];
        OpenPNGFromMemory(texture, image->data);
        image->texture = texture;
      }
    }

    /* key helpers */
    if (menu->helpers[i])
    {
      texture = (png_texture *)malloc(sizeof(png_texture));
      if (texture)
      {
        image = menu->helpers[i];
        OpenPNGFromMemory(texture, image->data);
        image->texture = texture;
      }
    }

    /* up/down buttons */
    if (menu->arrows[i])
    {
      button_data = menu->arrows[i]->data;
      texture = (png_texture *)malloc(sizeof(png_texture));
      if (texture)
      {
        OpenPNGFromMemory(texture, button_data->image);
        button_data->texture = texture;
      }
    }

    /* menu buttons */
    for (j=0; j<menu->max_buttons; j++)
    {
      button_data = menu->buttons[j]->data;
      if (!button_data->texture[i])
      {
        texture = (png_texture *)malloc(sizeof(png_texture));
        if (texture)
        {
          OpenPNGFromMemory(texture, buttons_data->image[i]);
          buttons_data->texture[i] = texture;
        }
      }
    }
  }

  /* allocate item textures */
  for (i=0; i<menu->max_items; i++)
  {
    item = menu->items[i];
    if (item->data)
    {
      texture = (png_texture *)malloc(sizeof(png_texture));
      if (texture)
      {
        OpenPNGFromMemory(texture, item->data);
        items->texture = texture;
      }
    }
  }
}

static void menu_delete(gui_menu *menu)
{
  int i,j;
  png_texture *texture;
  gui_item *item;
  gui_butn *button;
  gui_image *image;
  butn_data *button_data;

  /* free background image texture */
  if (menu->background)
  {
    texture = menu->background->texture;
    if (texture)
    {
      if (texture->data) free(texture.data);
      free(texture);
    }
  }

  /* allocate logo texture */
  if (menu->logo)
  {
    texture = menu->logo->texture;
    if (texture)
    {
      if (texture->data) free(texture.data);
      free(texture);
    }
  }

  /* allocate background elements textures */
  for (i=0; i<2; i++)
  {
    /* frames */
    if (menu->frames[i])
    {
      texture = menu->frames[i]->texture;
      if (texture)
      {
        if (texture->data) free(texture.data);
        free(texture);
        texture = NULL;
      }
    }

    /* key helpers */
    if (menu->helpers[i])
    {
      texture = menu->helpers[i]->texture;
      if (texture)
      {
        if (texture->data) free(texture.data);
        free(texture);
        texture = NULL;
      }
    }

    /* up/down buttons */
    if (menu->arrows[i])
    {
      texture = menu->arrows[i]->texture;
      if (texture)
      {
        if (texture->data) free(texture.data);
        free(texture);
        texture = NULL;
      }
    }

    /* menu buttons */
    for (j=0; j<menu->max_buttons; j++)
    {
      button_data = menu->buttons[j]->data;
      texture = button_data->texture[i];
      if (texture)
      {
        if (texture->data) free(texture.data);
        free(texture);
        texture = NULL;
      }
    }
  }

  /* allocate item textures */
  for (i=0; i<menu->max_items; i++)
  {
    texture = menu->items[i]->texture;
    if (texture)
    {
      if (texture->data) free(texture.data);
      free(texture);
      texture = NULL;
    }
  }
}

static void menu_draw(gui_menu *menu)
{
  int i;
  gui_item *item;
  gui_butn *button;
  gui_image *image;

  /* clear EFB */
  ClearScreen ((GXColor)BLACK);

  /* draw background image */
  image = menu->background;
  if (image) DrawTexture(image->texture,image->x,image->y,image->w,image->h);

  /* draw logo */
  image = menu->logo;
  if (image) DrawTexture(image->texture,image->x,image->y,image->w,image->h);

  /* draw title */
  if (menu->title) FONT_alignLeft(menu->title, 22,10,56);

  /* draw background frames */
  for (i=0; i<2; i++)
  {
    image = menu->frames[i];
    if (image) DrawTexture(image->texture,image->x,image->y,image->w,image->h);
  }

  /* draw left helper */
  item = menu->helper[0];
  if (item)
  {
    DrawTexture(item->texture, item->x, item->y, item->w, item->h);
    FONT_alignLeft(item->comment, 16, item->x+item->w+6,item->y+(item->h-16)/2 + 16);
  }

  /* draw right helper */
  item = menu->helper[1];
  if (item)
  {
    DrawTexture(item->texture, item->x, item->y, item->w, item->h);
    FONT_alignRight(item->comment, 16, item->x - 6, item->y+(item->h-16)/2 + 16);
  }

  /* draw buttons + items */
  for (i=0; i<menu->max_buttons; i++)
  {
    /* draw button */ 
    button = &menu->buttons[i];
    DrawTexture(button->texture[i == menu->selected], button->x, button->y, button->w, button->h);

    /* draw item */
    item = &menu->items[menu->offset +i];
    if (item->texture) DrawTexture(item->texture, item->x, item->y, item->w, item->h);
    else FONT_writeCenter(item->text, 18, button->x, button->x + button->w, button->y + (button->h - 18)/2 + 18);
  }

  /* Arrows (Items list only) */
  for (i=0; i<2; i++)
  {
    button = menu->arrows[i];
    if (button)
    {
      if (menu->selected == menu->max_buttons + i)
      {
        DrawTexture(button->texture[1], button->x-2, button->y-2, button->w+4, button->h+4);
      }
      else
      {
        DrawTexture(button->texture[0], button->x, button->y, button->w, button->h);
      }
    }
  }

#ifdef HW_RVL
  if (m_input.ir.valid)
  {
    /* draw wiimote pointer */
    gxResetCamera(m_input.ir.angle);
    DrawTexture(&w_pointer, m_input.ir.x, m_input.ir.y, w_pointer.width, w_pointer.height);
    gxResetCamera(0.0);
  }
#endif

  /* copy EFB to XFB */
  SetScreen ();
}

static int menu_callback(gui_menu *menu)
{
  u16 p;

#ifdef HW_RVL
  int i,x,y;
  gui_butn *button;
#endif

  for(;;)
  {
    menu_draw(menu);

    p = m_input.keys;

#ifdef HW_RVL
    if (Shutdown)
    {
      /* autosave SRAM/State */
      memfile_autosave();

      /* shutdown Wii */
      DI_Close();
      SYS_ResetSystem(SYS_POWEROFF, 0, 0);
    }
    else if (m_input.ir.valid)
    {
      /* get cursor position */
      x = m_input.ir.x;
      y = m_input.ir.y;

      /* check for valid buttons */
      for (i=0; i<menu->max_buttons; i++)
      {
        button = &menu->buttons[i];
        if ((x >= button->x) && (x <= (button->x + button->w)) && (y >= button->y) && (y <= (button->y + button->h)))
        {
          menu->selected = i;
          break;
        }
      }

      /* no valid buttons */
      if (i == menu->max_buttons)
      {
        menu->selected = i + 2;
        
        /* check for arrow buttons */
        if (menu->arrows[0])
        {
          button = menu->arrows[0];
          if ((x >= button->x) && (x <= (button->x + button->w)) && (y >= button->y) && (y <= (button->y + button->h)))
            menu->selected = i;
        }
        if (menu->arrows[1])
        {
          button = menu->arrows[1];
          if ((x >= button->x) && (x <= (button->x + button->w)) && (y >= button->y) && (y <= (button->y + button->h)))
            menu->selected = i +1;
        }
      }
    }
    else if (p & PAD_BUTTON_UP)
    {
      if (menu->selected == 0)
      {
        if (menu->offset) menu->offset --;
      }
      else if (menu->selected >= menu->shift)
      {
        menu->selected -= menu->shift;
      }
    }
    else if (p & PAD_BUTTON_DOWN)
    {
      if (menu->selected == (menu->max_buttons -1))
      {
        if ((menu->offset + menu->selected < (menu->max_items - 1))) menu->offset ++;
      }
      else if ((menu->selected + menu->shift) < menu->max_buttons)
      {
        menu->selected += menu->shift;
      }
    }
#else
    if (p & PAD_BUTTON_UP)
    {
      if (menu->selected == 0)
      {
        if (menu->offset) menu->offset --;
      }
      else if (menu->selected >= menu->shift)
      {
        menu->selected -= menu->shift;
      }
    }
    else if (p & PAD_BUTTON_DOWN)
    {
      if (menu->selected == (menu->max_buttons -1))
      {
        if ((menu->offset + menu->selected < (menu->max_items - 1))) menu->offset ++;
      }
      else if ((menu->selected + menu->shift) < menu->max_buttons)
      {
        menu->selected += menu->shift;
      }
    }
 #endif
    else if (p & PAD_BUTTON_LEFT)
    {
      if (menu->shift > 1)
      {
        menu->selected --;
        if (menu->selected < 0) menu->selected = 0;
      }
      else
      {
        return 0-2-menu->offset-menu->selected;
      }
    }
    else if (p & PAD_BUTTON_RIGHT)
    {
      if (menu->shift > 1)
      {
        menu->selected ++;
        if (menu->selected >= menu->max_buttons) menu->selected = menu->max_buttons - 1;
      }
      else
      {
        return (menu->offset + menu->selected);
      }
    }

    if (p & PAD_BUTTON_A)
    {
      if (menu->selected == menu->max_buttons) menu->offset --;
      else if (menu->selected == menu->max_buttons + 1) menu->offset ++;
      else if (menu->selected < menu->max_buttons) return (menu->offset + menu->selected);
    }
    else if (p & PAD_BUTTON_B)
    {
      return  -1;
    }

    /* update arrows status (items list) */
    if (menu->offset > 0)
      menu->arrows[0] = &arrow_up;
    else
      menu->arrows[0] = NULL;

    if (menu->offset + menu->max_buttons < menu->max_items)
      menu->arrows[1] = &arrow_down;
    else
      menu->arrows[1] = NULL;

    /* update comment */
    if (menu->helper[1])
    {
      gui_item *item = &menu->items[menu->offset + menu->selected];
      strcpy(menu->helper[1]->comment,item->comment);
    }
  }
}

void menu_update_inputs(u32 cnt)
{
  /* get gamepad inputs */
  PAD_ScanPads();
  u16 p = PAD_ButtonsDown(0);
  s8 x  = PAD_StickX(0);
  s8 y  = PAD_StickY(0);
  if (x > 70) p |= PAD_BUTTON_RIGHT;
  else if (x < -70) p |= PAD_BUTTON_LEFT;
  if (y > 60) p |= PAD_BUTTON_UP;
  else if (y < -60) p |= PAD_BUTTON_DOWN;

#ifdef HW_RVL
  /* get wiimote + expansions inputs */
  WPAD_ScanPads();
  u32 q = WPAD_ButtonsDown(0);
  u32 h = WPAD_ButtonsHeld(0);
  x = WPAD_StickX(0, 0);
  y = WPAD_StickY(0, 0);

  /* Wiimote orientation */
  WPAD_IR(0, &M_inputs.ir);

  /* wiimote directions */
  if (m_inputs.ir.valid)
  {
    if (q & WPAD_BUTTON_UP)         p |= PAD_BUTTON_UP;
    else if (q & WPAD_BUTTON_DOWN)  p |= PAD_BUTTON_DOWN;
    else if (q & WPAD_BUTTON_LEFT)  p |= PAD_BUTTON_LEFT;
    else if (q & WPAD_BUTTON_RIGHT) p |= PAD_BUTTON_RIGHT;
  }
  else
  {
    if (q & WPAD_BUTTON_UP)         p |= PAD_BUTTON_LEFT;
    else if (q & WPAD_BUTTON_DOWN)  p |= PAD_BUTTON_RIGHT;
    else if (q & WPAD_BUTTON_LEFT)  p |= PAD_BUTTON_DOWN;
    else if (q & WPAD_BUTTON_RIGHT) p |= PAD_BUTTON_UP;
  }

  /* analog sticks */
  if (y > 70)       p |= PAD_BUTTON_UP;
  else if (y < -70) p |= PAD_BUTTON_DOWN;
  if (x < -60)      p |= PAD_BUTTON_LEFT;
  else if (x > 60)  p |= PAD_BUTTON_RIGHT;

  /* classic controller directions */
  if (q & WPAD_CLASSIC_BUTTON_UP)         p |= PAD_BUTTON_UP;
  else if (q & WPAD_CLASSIC_BUTTON_DOWN)  p |= PAD_BUTTON_DOWN;
  if (q & WPAD_CLASSIC_BUTTON_LEFT)       p |= PAD_BUTTON_LEFT;
  else if (q & WPAD_CLASSIC_BUTTON_RIGHT) p |= PAD_BUTTON_RIGHT;

  /* wiimote keys */
  if (q & WPAD_BUTTON_MINUS)  p |= PAD_TRIGGER_L;
  if (q & WPAD_BUTTON_PLUS)   p |= PAD_TRIGGER_R;
  if (q & WPAD_BUTTON_A)      p |= PAD_BUTTON_A;
  if (q & WPAD_BUTTON_B)      p |= PAD_BUTTON_B;
  if (q & WPAD_BUTTON_2)      p |= PAD_BUTTON_A;
  if (q & WPAD_BUTTON_1)      p |= PAD_BUTTON_B;
  if (q & WPAD_BUTTON_HOME)   p |= PAD_TRIGGER_Z;

  /* classic controller keys */
  if (q & WPAD_CLASSIC_BUTTON_FULL_L) p |= PAD_TRIGGER_L;
  if (q & WPAD_CLASSIC_BUTTON_FULL_R) p |= PAD_TRIGGER_R;
  if (q & WPAD_CLASSIC_BUTTON_A)      p |= PAD_BUTTON_A;
  if (q & WPAD_CLASSIC_BUTTON_B)      p |= PAD_BUTTON_B;
  if (q & WPAD_CLASSIC_BUTTON_HOME)   p |= PAD_TRIGGER_Z;
#endif

  m_input.keys = p;
}

/***************************************************************************
 * drawmenu
 *
 * As it says, simply draws the menu with a highlight on the currently
 * selected item :)
 ***************************************************************************/
char menutitle[60] = { "" };
int menu = 0;

void drawmenu (char items[][25], int maxitems, int selected)
{

  int i;
  int ypos;

  ypos = (226 - (fheight * maxitems)) >> 1;
  ypos += 130;

  /* reset texture data */
  png_texture texture;
  memset(&texture,0,sizeof(png_texture));

  /* draw background items */
  ClearScreen ((GXColor)BLACK);
  OpenPNGFromMemory(&texture, Background_main);
  DrawTexture(&texture, (640-texture.width)/2, (480-124-texture.height)/2,  texture.width, texture.height);
  OpenPNGFromMemory(&texture, Banner_bottom);
  DrawTexture(&texture, 640-texture.width, 480-texture.height, texture.width, texture.height);
  OpenPNGFromMemory(&texture, Banner_top);
  DrawTexture(&texture, 640-texture.width, 0, texture.width, texture.height);
  OpenPNGFromMemory(&texture, Main_logo);
  DrawTexture(&texture, 444, 28, 176, 48);

 // WriteCentre (134, menutitle);

  for (i = 0; i < maxitems; i++)
  {
      if (i == selected) WriteCentre_HL (i * fheight + ypos, (char *) items[i]);
      else WriteCentre (i * fheight + ypos, (char *) items[i]);
  }

  SetScreen ();
}

int domenu (char items[][25], int maxitems, u8 fastmove)
{
  int redraw = 1;
  int quit = 0;
  short p;
  int ret = 0;


  while (quit == 0)
  {
    if (redraw)
    {
      drawmenu (&items[0], maxitems, menu);
      redraw = 0;
    }
    
    p = ogc_input__getMenuButtons(0);
    
    if (p & PAD_BUTTON_UP)
    {
      redraw = 1;
      menu--;
      if (menu < 0) menu = maxitems - 1;
    }
    else if (p & PAD_BUTTON_DOWN)
    {
      redraw = 1;
      menu++;
      if (menu == maxitems) menu = 0;
    }

    if (p & PAD_BUTTON_A)
    {
      quit = 1;
      ret = menu;
    }
    else if (p & PAD_BUTTON_B)
    {
      quit = 1;
      ret = -1;
    }

    if (fastmove)
    {
      if (p & PAD_BUTTON_RIGHT)
      {
        quit = 1;
        ret = menu;
      }
      else if (p & PAD_BUTTON_LEFT)
      {
        quit = 1;
        ret = 0 - 2 - menu;
      }
    }
  }

  return ret;
}

/****************************************************************************
 * Sound Option menu
 *
 ****************************************************************************/
void soundmenu ()
{
  int ret;
  int quit = 0;
  gui_item *items = menu_audio.items;

  while (quit == 0)
  {
    sprintf (items[0].text, "PSG Volume: %1.2f", (double)config.psg_preamp/100.0);
    sprintf (items[1].text, "FM Volume: %1.2f", (double)config.fm_preamp/100.0);
    sprintf (items[2].text, "Volume Boost: %dX", config.boost);
    sprintf (items[3].text, "LowPass Filter: %s", config.filter ? " ON":"OFF");
    if (config.hq_fm == 0) sprintf (items[4].text, "HQ YM2612: OFF");
    else if (config.hq_fm == 1) sprintf (items[4].text, "HQ YM2612: LINEAR");
    else sprintf (items[4].text, "HQ YM2612: SINC");

    ret = MenuCall(&menu_audio);
    switch (ret)
    {
      case 0:
      case -2:
        if (ret<0) config.psg_preamp --;
        else config.psg_preamp ++;
        if (config.psg_preamp < 0) config.psg_preamp = 500;
        if (config.psg_preamp > 500) config.psg_preamp = 0;
        break;

      case 1:
      case -3:
        if (ret<0) config.fm_preamp --;
        else config.fm_preamp ++;
        if (config.fm_preamp < 0) config.fm_preamp = 500;
        if (config.fm_preamp > 500) config.fm_preamp = 0;
        break;

      case 2:
        config.boost ++;
        if (config.boost > 4) config.boost = 0;
        break;
      
      case 3:
        config.filter ^= 1;
        break;

      case 4:
        config.hq_fm ++;
        if (config.hq_fm>2) config.hq_fm = 0;
        if (genromsize) 
        {
          unsigned char *temp = malloc(YM2612GetContextSize());
          if (!temp) break;
          memcpy(temp, YM2612GetContextPtr(), YM2612GetContextSize());
          audio_init(48000);
          YM2612Restore(temp);
          free(temp);
        }
        break;

      case -1:
        quit = 1;
        break;
    }
  }
}

/****************************************************************************
 * Misc Option menu
 *
 ****************************************************************************/
void miscmenu ()
{
  int ret;
  int quit = 0;
  gui_item *items = menu_system.items;

  while (quit == 0)
  {
    if (config.region_detect == 0)      sprintf (items[0].text, "Region: AUTO");
    else if (config.region_detect == 1) sprintf (items[0].text, "Region:  USA");
    else if (config.region_detect == 2) sprintf (items[0].text, "Region:  EUR");
    else if (config.region_detect == 3) sprintf (items[0].text, "Region:  JAP");
    sprintf (items[1].text, "Force DTACK: %s", config.force_dtack ? "Y" : "N");
    if (config.bios_enabled & 1) sprintf (items[2].text, "Use BIOS: ON");
    else sprintf (items[2].text, "Use BIOS: OFF");
    sprintf (items[3].text, "SVP Cycles: %d", SVP_cycles);
    if (config.sram_auto == 0) sprintf (items[4].text, "Auto SRAM: FAT");
    else if (config.sram_auto == 1) sprintf (items[4].text, "Auto SRAM: MCARD A");
    else if (config.sram_auto == 2) sprintf (items[4].text, "Auto SRAM: MCARD B");
    else sprintf (items[4].text, "Auto SRAM: OFF");
    if (config.freeze_auto == 0) sprintf (items[5].text, "Auto FREEZE: FAT");
    else if (config.freeze_auto == 1) sprintf (items[5].text, "Auto FREEZE: MCARD A");
    else if (config.freeze_auto == 2) sprintf (items[5].text, "Auto FREEZE: MCARD B");
    else sprintf (items[5].text, "Auto FREEZE: OFF");

    ret = MenuCall(&menu_system);
    switch (ret)
    {
      case 0:  /*** Region Force ***/
        config.region_detect = (config.region_detect + 1) % 4;
        if (genromsize)
        {
          /* force region & cpu mode */
          set_region();
          
          /* reinitialize timings */
          system_init ();
          unsigned char *temp = malloc(YM2612GetContextSize());
          if (temp) memcpy(temp, YM2612GetContextPtr(), YM2612GetContextSize());
          audio_init(48000);
          YM2612Restore(temp);
          if (temp) free(temp);

          /* reinitialize HVC tables */
          vctab = (vdp_pal) ? ((reg[1] & 8) ? vc_pal_240 : vc_pal_224) : vc_ntsc_224;
          hctab = (reg[12] & 1) ? cycle2hc40 : cycle2hc32;

          /* reinitialize overscan area */
          bitmap.viewport.x = config.overscan ? ((reg[12] & 1) ? 16 : 12) : 0;
          bitmap.viewport.y = config.overscan ? (((reg[1] & 8) ? 0 : 8) + (vdp_pal ? 24 : 0)) : 0;
        }
        break;

      case 1:  /*** force DTACK ***/
        config.force_dtack ^= 1;
        break;

      case 2:  /*** BIOS support ***/
        config.bios_enabled ^= 1;
        if (genromsize || (config.bios_enabled == 3)) 
        {
          system_init ();
          audio_init(48000);
          system_reset ();
        }
        break;

      case 3:  /*** SVP emulation ***/
      case -5:
        if (ret<0) SVP_cycles = SVP_cycles ? (SVP_cycles-1) : 1500;
        else SVP_cycles++;
        if (SVP_cycles > 1500) SVP_cycles = 0;
        break;

      case 4:  /*** SRAM autoload/autosave ***/
        config.sram_auto ++;
        if (config.sram_auto > 2) config.sram_auto = -1;
        break;

      case 5:  /*** FreezeState autoload/autosave ***/
        config.freeze_auto ++;
        if (config.freeze_auto > 2) config.freeze_auto = -1;
        break;

      case -1:
        quit = 1;
        break;
    }
  }
}

/****************************************************************************
 * Display Option menu
 *
 ****************************************************************************/
void dispmenu ()
{
  int ret;
  int quit = 0;
  gui_item *items = menu_video.items;

  while (quit == 0)
  {
    sprintf (items[0].text, "Aspect: %s", config.aspect ? "ORIGINAL" : "STRETCHED");
    if (config.render == 1) sprintf (items[1].text,"Display: INTERLACED");
    else if (config.render == 2) sprintf (items[1].text, "Display: PROGRESSIVE");
    else sprintf (items[1].text, "Display: ORIGINAL");
    if (config.tv_mode == 0) sprintf (items[2].text, "TV Mode: 60HZ");
    else if (config.tv_mode == 1) sprintf (items[2].text, "TV Mode: 50HZ");
    else sprintf (items[2].text, "TV Mode: 50/60HZ");
    sprintf (items[3].text, "Bilinear Filter: %s", config.bilinear ? " ON" : "OFF");
    if (config.ntsc == 1) sprintf (items[4].text, "NTSC Filter: COMPOSITE");
    else if (config.ntsc == 2) sprintf (items[4].text, "NTSC Filter: S-VIDEO");
    else if (config.ntsc == 3) sprintf (items[4].text, "NTSC Filter: RGB");
    else sprintf (items[4].text, "NTSC Filter: OFF");
    sprintf (items[5].text, "Borders: %s", config.overscan ? " ON" : "OFF");
    strcpy (items[6].text, "DISPLAY POSITION");
    strcpy (items[7].text, "DISPLAY SIZE");

    ret = MenuCall(&menu_video);
    switch (ret)
    {
      case 0: /*** config.aspect ratio ***/
        config.aspect ^= 1;
        break;

      case 1:  /*** rendering ***/
        config.render = (config.render + 1) % 3;
        if (config.render == 2)
        {
          if (VIDEO_HaveComponentCable())
          {
            /* progressive mode (60hz only) */
            config.tv_mode = 0;
          }
          else
          {
            /* do nothing if component cable is not detected */
            config.render = 0;
          }
        }
        break;

      case 2: /*** tv mode ***/
        if (config.render == 2) break; /* 60hz progressive only */
        config.tv_mode = (config.tv_mode + 1) % 3;
        break;
    
      case 3: /*** bilinear filtering ***/
        config.bilinear ^= 1;
        break;

      case 4: /*** NTSC filter ***/
        config.ntsc ++;
        if (config.ntsc > 3) config.ntsc = 0;
        break;

      case 5: /*** overscan emulation ***/
        config.overscan ^= 1;
        bitmap.viewport.x = config.overscan ? ((reg[12] & 1) ? 16 : 12) : 0;
        bitmap.viewport.y = config.overscan ? (((reg[1] & 8) ? 0 : 8) + (vdp_pal ? 24 : 0)) : 0;
        break;

 /*     case 6: 
      case -8:
        if (ret<0) config.xshift --;
        else config.xshift ++;
        break;

      case 7: 
      case -9:
        if (ret<0) config.yshift --;
        else config.yshift ++;
        break;
      
      case 8: 
      case -10:
        if (config.aspect) break;
        if (ret<0) config.xscale --;
        else config.xscale ++;
        break;

      case 9: 
      case -11:
        if (config.aspect) break;
        if (ret<0) config.yscale --;
        else config.yscale ++;
        break;
*/
      case -1:
        quit = 1;
        break;
    }
  }
}

/****************************************************************************
 * ConfigureJoypads
 ****************************************************************************/
extern int old_system[2];
void ConfigureJoypads ()
{
  int ret, max_players;
  int i = 0;
  int quit = 0;
  int prevmenu = menu;
  char padmenu[8][25];

  int player = 0;
#ifdef HW_RVL
  u32 exp;
#endif

  strcpy (menutitle, "Press B to return");

  menu = 0;
  while (quit == 0)
  {
    /* update max players */
    max_players = 0;
    if (input.system[0] == SYSTEM_GAMEPAD)
    {
      sprintf (padmenu[0], "Port 1: GAMEPAD");
      max_players ++;
    }
    else if (input.system[0] == SYSTEM_MOUSE)
    {
      sprintf (padmenu[0], "Port 1: MOUSE");
      max_players ++;
    }
    else if (input.system[0] == SYSTEM_WAYPLAY)
    {
      sprintf (padmenu[0], "Port 1: 4-WAYPLAY");
      max_players += 4;
    }
    else if (input.system[0] == SYSTEM_TEAMPLAYER)
    {
      sprintf (padmenu[0], "Port 1: TEAMPLAYER");
      max_players += 4;
    }
    else
      sprintf (padmenu[0], "Port 1: NONE");

    if (input.system[1] == SYSTEM_GAMEPAD)
    {
      sprintf (padmenu[1], "Port 2: GAMEPAD");
      max_players ++;
    }
    else if (input.system[1] == SYSTEM_MOUSE)
    {
      sprintf (padmenu[1], "Port 2: MOUSE");
      max_players ++;
    }
    else if (input.system[1] == SYSTEM_WAYPLAY)
    {
      sprintf (padmenu[1], "Port 2: 4-WAYPLAY");
    }
    else if (input.system[1] == SYSTEM_TEAMPLAYER)
    {
      sprintf (padmenu[1], "Port 2: TEAMPLAYER");
      max_players += 4;
    }
    else if (input.system[1] == SYSTEM_MENACER)
    {
      sprintf (padmenu[1], "Port 2: MENACER");
      max_players += 1;
    }
    else if (input.system[1] == SYSTEM_JUSTIFIER)
    {
      sprintf (padmenu[1], "Port 2: JUSTIFIERS");
      max_players += 2;
    }
    else
      sprintf (padmenu[1], "Port 2: NONE");

    /* JCART special case */
    if (j_cart) max_players +=2;

    /* reset current player nr */
    if (player >= max_players)
    {
      /* remove duplicate assigned inputs */
      if ((0!=player) && (config.input[0].device == config.input[player].device) && (config.input[0].port == config.input[player].port))
      {
          config.input[0].device = -1;
          config.input[0].port = i%4;
      }
      player = 0;
    }

    sprintf (padmenu[2], "Gun Cursor: %s", config.gun_cursor ? " ON":"OFF");
    sprintf (padmenu[3], "Invert Mouse: %s", config.invert_mouse ? " ON":"OFF");
    sprintf (padmenu[4], "Set Player: %d%s", player + 1, (j_cart && (player > 1)) ? "-JCART" : "");

    if (config.input[player].device == 0)
      sprintf (padmenu[5], "Device: GAMECUBE %d", config.input[player].port + 1);
#ifdef HW_RVL
    else if (config.input[player].device == 1)
      sprintf (padmenu[5], "Device: WIIMOTE %d", config.input[player].port + 1);
    else if (config.input[player].device == 2)
      sprintf (padmenu[5], "Device: NUNCHUK %d", config.input[player].port + 1);
    else if (config.input[player].device == 3)
      sprintf (padmenu[5], "Device: CLASSIC %d", config.input[player].port + 1);
#endif
    else
      sprintf (padmenu[5], "Device: NONE");

    /* when using wiimote, force to 3Buttons pad */
    if (config.input[player].device == 1) input.padtype[player] = DEVICE_3BUTTON;
    sprintf (padmenu[6], "%s", (input.padtype[player] == DEVICE_3BUTTON) ? "Type: 3BUTTONS":"Type: 6BUTTONS");

    sprintf (padmenu[7], "Configure Input");

    ret = domenu (&padmenu[0], 8,0);

    switch (ret)
    {
      case 0:
        if (j_cart)
        {
          WaitPrompt("JCART detected !");
          break;
        }
        input.system[0] ++;
        if (input.system[0] == SYSTEM_MENACER) input.system[0] ++;
        if (input.system[0] == SYSTEM_JUSTIFIER) input.system[0] ++;
        if ((input.system[0] == SYSTEM_MOUSE) && (input.system[1] == SYSTEM_MOUSE)) input.system[0] ++;
        if (input.system[0] == SYSTEM_WAYPLAY) input.system[1] = SYSTEM_WAYPLAY;
        if (input.system[0] > SYSTEM_WAYPLAY)
        {
          input.system[0] = NO_SYSTEM;
          input.system[1] = SYSTEM_GAMEPAD;
        }
        io_reset();
        old_system[0] = input.system[0];
        old_system[1] = input.system[1];
        break;
    
      case 1:
        if (j_cart)
        {
          WaitPrompt("JCART detected !");
          break;
        }
        input.system[1] ++;
        if ((input.system[0] == SYSTEM_MOUSE) && (input.system[1] == SYSTEM_MOUSE)) input.system[1] ++;
        if (input.system[1] == SYSTEM_WAYPLAY) input.system[0] = SYSTEM_WAYPLAY;
        if (input.system[1] > SYSTEM_WAYPLAY)
        {
          input.system[1] = NO_SYSTEM;
          input.system[0] = SYSTEM_GAMEPAD;
        }
        io_reset();
        old_system[0] = input.system[0];
        old_system[1] = input.system[1];
        break;

      case 2:
        config.gun_cursor ^= 1;
        break;

      case 3:
        config.invert_mouse ^= 1;
        break;

      case 4:
        /* remove duplicate assigned inputs */
        for (i=0; i<8; i++)
        {
          if ((i!=player) && (config.input[i].device == config.input[player].device) && (config.input[i].port == config.input[player].port))
          {
            config.input[i].device = -1;
            config.input[i].port = i%4;
          }
        }
        player = (player + 1) % max_players;
        break;

      case 5:
#ifdef HW_RVL
        if (config.input[player].device > 0)
        {
          config.input[player].port ++;
        }
        else
        {
          config.input[player].device ++;
          if (config.input[player].device == 1) config.input[player].port = 0;
        }

        if (config.input[player].device == 1)
        {
          exp = 4;
          if (config.input[player].port<4)
          {
            WPAD_Probe(config.input[player].port,&exp);
            if (exp == WPAD_EXP_NUNCHUK) exp = 4;
          }

          while ((config.input[player].port<4) && (exp == 4))
          {
            config.input[player].port ++;
            if (config.input[player].port<4)
            {
              exp = 4;
              WPAD_Probe(config.input[player].port,&exp);
              if (exp == WPAD_EXP_NUNCHUK) exp = 4;
            }
          }

          if (config.input[player].port >= 4)
          {
            config.input[player].port = 0;
            config.input[player].device = 2;
          }
        }

        if (config.input[player].device == 2)
        {
          exp = 4;
          if (config.input[player].port<4)
          {
            WPAD_Probe(config.input[player].port,&exp);
          }

          while ((config.input[player].port<4) && (exp != WPAD_EXP_NUNCHUK))
          {
            config.input[player].port ++;
            if (config.input[player].port<4)
            {
              exp = 4;
              WPAD_Probe(config.input[player].port,&exp);
            }
          }

          if (config.input[player].port >= 4)
          {
            config.input[player].port = 0;
            config.input[player].device = 3;
          }
        }

        if (config.input[player].device == 3)
        {
          exp = 4;
          if (config.input[player].port<4)
          {
            WPAD_Probe(config.input[player].port,&exp);
          }

          while ((config.input[player].port<4) && (exp != WPAD_EXP_CLASSIC))
          {
            config.input[player].port ++;
            if (config.input[player].port<4)
            {
              exp = 4;
              WPAD_Probe(config.input[player].port,&exp);
            }
          }

          if (config.input[player].port >= 4)
          {
            config.input[player].port = player % 4;
            config.input[player].device = 0;
          }
        }
#else
        config.input[player].device = 0;
#endif
        break;
    
      case 6:
        if (config.input[player].device == 1) break;
        input.padtype[player] ^= 1;
        io_reset();
        break;

      case 7:
        if (config.input[player].device < 0) break;
        ogc_input__config(config.input[player].port, config.input[player].device, input.padtype[player]);
        break;

      case -1:
        /* remove duplicate assigned inputs */
        for (i=0; i<8; i++)
        {
          if ((i!=player) && (config.input[i].device == config.input[player].device) && (config.input[i].port == config.input[player].port))
          {
            config.input[i].device = -1;
            config.input[i].port = i%4;
          }
        }
        quit = 1;
        break;
    }
  }

  menu = prevmenu;
}

/****************************************************************************
 * Main Option menu
 *
 ****************************************************************************/
void optionmenu ()
{
  int ret;
  int quit = 0;

  while (quit == 0)
  {
    ret = MenuCall(&menu_options);
    switch (ret)
    {
      case 0:
        miscmenu();
        break;
      case 1:
        dispmenu();
        break;
      case 2:
        soundmenu();
        break;
      case 3:
        ConfigureJoypads();
        break;
      case 4:
        GetGGEntries();
        break;
      case -1:
        quit = 1;
        break;
    }
  }

  config_save();
}

/****************************************************************************
* Generic Load/Save menu
*
****************************************************************************/
static u8 device = 0;

int loadsavemenu (int which)
{
  int prevmenu = menu;
  int quit = 0;
  int ret;
  int count = 3;
  char items[3][25];

  strcpy (menutitle, "Press B to return");

  menu = 2;

  if (which == 1)
  {
    sprintf(items[1], "Save State");
    sprintf(items[2], "Load State");
  }
  else
  {
    sprintf(items[1], "Save SRAM");
    sprintf(items[2], "Load SRAM");
  }

  while (quit == 0)
  {
    if (device == 0) sprintf(items[0], "Device: FAT");
    else if (device == 1) sprintf(items[0], "Device: MCARD A");
    else if (device == 2) sprintf(items[0], "Device: MCARD B");

    ret = domenu (&items[0], count, 0);
    switch (ret)
    {
      case -1:
        quit = 1;
        break;

      case 0:
        device = (device + 1)%3;
        break;

      case 1:
      case 2:
        if (which == 1) quit = ManageState(ret-1,device);
        else if (which == 0) quit = ManageSRAM(ret-1,device);
        if (quit) return 1;
        break;
    }
  }

  menu = prevmenu;
  return 0;
}


/****************************************************************************
 * File Manager menu
 *
 ****************************************************************************/
int filemenu ()
{
  int prevmenu = menu;
  int ret;
  int quit = 0;
  int count = 2;
  char items[2][25] = {
    {"SRAM Manager"},
    {"STATE Manager"}
  };

  menu = 0;

  while (quit == 0)
  {
    strcpy (menutitle, "Press B to return");
    ret = domenu (&items[0], count, 0);
    switch (ret)
    {
      case -1: /*** Button B ***/
        ret = 0;
        quit = 1;
        break;

      case 0:   /*** SRAM Manager ***/
      case 1:  /*** SaveState Manager ***/
        if (loadsavemenu(ret)) return 1;
        break;
    }
  }

  menu = prevmenu;
  return 0;
}


/****************************************************************************
 * Load Rom menu
 *
 ****************************************************************************/
extern char rom_filename[MAXJOLIET];
static u8 load_menu = 0;
static u8 dvd_on = 0;

int loadmenu ()
{
  int ret,size;
  int quit = 0;
  while (quit == 0)
  {
    ret = MenuCall(&menu_load);
    switch (ret)
    {
      /*** Button B ***/
      case -1: 
        quit = 1;
        break;

      /*** Load from DVD ***/
#ifdef HW_RVL
      case 3:
#else
      case 2:
#endif
        load_menu = menu;
        size = DVD_Open(cart_rom);
        if (size)
        {
          dvd_on = 1;
          genromsize = size;
          memfile_autosave();
          reloadrom();
          sprintf(rom_filename,"%s",filelist[selection].filename);
          rom_filename[strlen(rom_filename) - 4] = 0;
          memfile_autoload();
          return 1;
        }
        break;

      /*** Stop DVD Disc ***/
#ifdef HW_RVL
      case 4:  
#else
      case 3:
#endif
        dvd_motor_off();
        dvd_on = 0;
        menu = load_menu;
        break;

      /*** Load from FAT device ***/
      default:
        load_menu = menu;
        size = FAT_Open(ret,cart_rom);
        if (size)
        {
          memfile_autosave();
          genromsize = size;
          reloadrom();
          sprintf(rom_filename,"%s",filelist[selection].filename);
          rom_filename[strlen(rom_filename) - 4] = 0;
          memfile_autoload();
          return 1;
        }
        break;

    }
  }

  return 0;
}

/***************************************************************************
  * Show rom info screen
 ***************************************************************************/
void showrominfo ()
{
  int ypos;
  u8 i,j,quit,redraw,max;
  char msg[128];
  short p;
  char pName[14][21];

  quit = 0;
  j = 0;
  redraw = 1;

  /*** Remove any still held buttons ***/
  while (PAD_ButtonsHeld(0))  PAD_ScanPads();
#ifdef HW_RVL
  while (WPAD_ButtonsHeld(0)) WPAD_ScanPads();
#endif

  max = 14;
  for (i = 0; i < 14; i++)
  {
    if (peripherals & (1 << i))
    {
      sprintf(pName[max-14],"%s", peripheralinfo[i].pName);
      max ++;
    }
  }

  while (quit == 0)
  {
    if (redraw)
    {
      ClearScreen ((GXColor)BLACK);

      ypos = 134;
      WriteCentre(ypos, "ROM Header Information");
      ypos += 2*fheight;

      for (i=0; i<8; i++)
      {
        switch (i+j)
        {
        case 0:
          sprintf (msg, "Console type: %s", rominfo.consoletype);
          break;
        case 1:
          sprintf (msg, "Copyright: %s", rominfo.copyright);
          break;
        case 2:
          sprintf (msg, "Company: %s", companyinfo[getcompany ()].company);
          break;
        case 3:
          sprintf (msg, "Game Domestic Name:");
          break;
        case 4:
          sprintf(msg, " %s",rominfo.domestic);
          break;
        case 5:
          sprintf (msg, "Game International Name:");
          break;
        case 6:
          sprintf(msg, " %s",rominfo.international);
          break;
        case 7:
          sprintf (msg, "Type - %s : %s", rominfo.ROMType, strcmp (rominfo.ROMType, "AI") ? "Game" : "Educational");
          break;
        case 8:
          sprintf (msg, "Product - %s", rominfo.product);
          break;
        case 9:
          sprintf (msg, "Checksum - %04x (%04x) (%s)", rominfo.checksum, realchecksum, (rominfo.checksum == realchecksum) ? "Good" : "Bad");
          break;
        case 10:
          sprintf (msg, "ROM end: $%06X", rominfo.romend);
          break;
        case 11:
          if (svp) sprintf (msg, "SVP Chip detected");
          else if (sram.custom) sprintf (msg, "EEPROM(%dK) - $%06X", ((eeprom.type.size_mask+1)* 8) /1024, (unsigned int)sram.start);
          else if (sram.detected) sprintf (msg, "SRAM Start  - $%06X", sram.start);
          else sprintf (msg, "External RAM undetected");
             
          break;
        case 12:
          if (sram.custom) sprintf (msg, "EEPROM(%dK) - $%06X", ((eeprom.type.size_mask+1)* 8) /1024, (unsigned int)sram.end);
          else if (sram.detected) sprintf (msg, "SRAM End   - $%06X", sram.end);
          else if (sram.on) sprintf (msg, "Default SRAM activated ");
          else sprintf (msg, "SRAM is disactivated  ");
          break;
        case 13:
          if (region_code == REGION_USA) sprintf (msg, "Region - %s (USA)", rominfo.country);
          else if (region_code == REGION_EUROPE) sprintf (msg, "Region - %s (EUR)", rominfo.country);
          else if (region_code == REGION_JAPAN_NTSC) sprintf (msg, "Region - %s (JAP)", rominfo.country);
          else if (region_code == REGION_JAPAN_PAL) sprintf (msg, "Region - %s (JPAL)", rominfo.country);
          break;
        default:
          sprintf (msg, "Supports - %s", pName[i+j-14]);
          break;
      }

      write_font (100, ypos, msg);
      ypos += fheight;
    }

    ypos += fheight;
    WriteCentre (ypos, "Press A to Continue");
    SetScreen ();
  }

  p = ogc_input__getMenuButtons(0);
  redraw = 0;

  if ((j<(max-8)) && (p & PAD_BUTTON_DOWN)) {redraw = 1; j++;}
  if ((j>0) && (p & PAD_BUTTON_UP)) {redraw = 1; j--;}
  if (p & PAD_BUTTON_A) quit = 1;
  if (p & PAD_BUTTON_B) quit = 1;
  }
}

/****************************************************************************
 * Main Menu
 *
 ****************************************************************************/
void MainMenu (u32 fps)
{
  int ret;
  int quit = 0;
  uint32 crccheck;

  /* autosave (SRAM only) */
  int temp = config.freeze_auto;
  config.freeze_auto = -1;
  memfile_autosave();
  config.freeze_auto = temp;

  while (quit == 0)
  {
    crccheck = crc32 (0, &sram.sram[0], 0x10000);
    strcpy (menutitle,"");
    if (genromsize && (crccheck != sram.crc)) strcpy (menutitle, "*** SRAM has been modified ***");
    else if (genromsize) sprintf (menutitle, "%d FPS",fps);

    menu_initialize(&menu_main);
    ret = menu_callback(&menu_main);
    switch (ret)
    {
      case -1: /*** Button B ***/
      case 0:  /*** Play Game ***/
        if (genromsize) quit = 1;
        break;

      case 1:  /*** Load ROM Menu ***/
        quit = loadmenu();
        break;

      case 2:  /*** Emulator Options */
        optionmenu ();
        break;

      case 3:  /*** Memory Manager ***/
        quit = filemenu ();
        break;

      case 4:  /*** Emulator Reset ***/
        if (genromsize || (config.bios_enabled == 3))
        {
          system_reset (); 
          quit = 1;
        }
        break;

      case 5:   /*** ROM Information ***/
        showrominfo ();
        break;


      case 6:  /*** SD/PSO/TP Reload ***/
        menu_delete(&menu_main);
        memfile_autosave();
        system_shutdown();
        audio_shutdown();
        VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], COLOR_BLACK);
        VIDEO_Flush();
        VIDEO_WaitVSync();
#ifdef HW_RVL
        DI_Close();
#endif
        exit(0);
        break;

      case 7:  /*** Return to Wii System Menu ***/
        menu_delete(&menu_main);
        memfile_autosave();
        system_shutdown();
        audio_shutdown();
        VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], COLOR_BLACK);
        VIDEO_Flush();
        VIDEO_WaitVSync();
#ifdef HW_RVL
        DI_Close();
        SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
#else
        SYS_ResetSystem(SYS_HOTRESET,0,0);
#endif
        break;
    }
  }

  menu_delete(&menu_main);

  /*** Remove any still held buttons ***/
  while (PAD_ButtonsHeld(0))  PAD_ScanPads();
#ifdef HW_RVL
  while (WPAD_ButtonsHeld(0)) WPAD_ScanPads();
#endif

#ifndef HW_RVL
  /*** Stop the DVD from causing clicks while playing ***/
  uselessinquiry ();
#endif
}