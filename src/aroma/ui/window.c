/********************************************************************[libaroma]*
 * Copyright (C) 2011-2015 Ahmad Amarullah (http://amarullz.com/)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *______________________________________________________________________________
 *
 * Filename    : window.c
 * Description : window api
 *
 * + This is part of libaroma, an embedded ui toolkit.
 * + 27/01/15 - Author(s): Ahmad Amarullah
 *
 */
#ifndef __libaroma_aroma_c__
  #error "Should be inside aroma.c."
#endif
#ifndef __libaroma_window_c__
#define __libaroma_window_c__
/* check wm macro */
#define __CHECK_WM(RETVAL) \
  if (libaroma_wm()==NULL){ \
    ALOGW("window manager uninitialized"); \
    return RETVAL; \
  }

/*
 * Function    : libaroma_window
 * Return Value: LIBAROMA_WINDOWP
 * Descriptions: new window
 */
LIBAROMA_WINDOWP libaroma_window(
  char * bg_theme_name,
  int x, int y, int w, int h
){
  __CHECK_WM(NULL);
  LIBAROMA_WINDOWP win = (LIBAROMA_WINDOWP) malloc(sizeof(LIBAROMA_WINDOW));
  /* zero values */
  win->threadn = win->childn = 
  win->client_w = win->client_h =
  win->scroll_x = win->scroll_y = 0;
  if (bg_theme_name){
    snprintf(win->theme_bg,256,"%s",bg_theme_name);
  }
  else{
    snprintf(win->theme_bg,256,"%s","window");
  }
  win->bg = NULL;
  win->dc = NULL;
  win->childs = NULL;
  win->focused = NULL;
  win->touched = NULL;
  win->x = x;
  win->y = y;
  win->w = w;
  win->h = h;
  return win;
} /* End of libaroma_window */

/*
 * Function    : libaroma_window_free
 * Return Value: byte
 * Descriptions: free window
 */
byte libaroma_window_free(
  LIBAROMA_WINDOWP win
){
  __CHECK_WM(0);
  if (win==NULL){
    return 0;
  }
  
  /* inactivate it */
  LIBAROMA_MSG _msg;
  _msg.msg    = LIBAROMA_MSG_WIN_INACTIVE;
  _msg.state = _msg.key = _msg.x = _msg.y= 0;
  _msg.d      = NULL;
  _msg.sent   = libaroma_nano_tick();
  libaroma_window_event(win,&_msg);
  
  /* delete childs */
  int i;
  if (win->childn>0){
    for (i=0;i<win->childn;i++){
      libaroma_control_free(win->childs[i]);
    }
    free(win->childs);
  }
  if (win->bg){
    libaroma_canvas_free(win->bg);
  }
  if (win->dc){
    libaroma_canvas_free(win->dc);
  }
  free(win);
  return 1;
} /* End of libaroma_window_free */

/*
 * Function    : _libaroma_window_updatebg
 * Return Value: byte
 * Descriptions: update window background
 */
byte _libaroma_window_updatebg(LIBAROMA_WINDOWP win){
  if (win==NULL){
    ALOGW("window_recalculate win is NULL");
    return 0;
  }
  /* draw background */
  if (win->bg!=NULL){
    if ((win->bg->w==win->client_w)&&(win->bg->h==win->client_h)){
      /* not need update */
      return 1;
    }
    libaroma_canvas_free(win->bg);
  }
  int w = win->client_w;
  int h = win->client_h;
  win->bg = libaroma_canvas(w,h);
  libaroma_grad(win->bg,0,0,w,h,
    libaroma_wm_get_color("window"),
    libaroma_wm_get_color("window_gradient"));
  libaroma_wm_draw_theme(
      win->bg, 
      win->theme_bg,
      0,0,w,h,NULL);
  return 1;
} /* End of _libaroma_window_updatebg */

/*
 * Function    : _libaroma_window_recalculate
 * Return Value: byte
 * Descriptions: recalculate client size
 */
byte _libaroma_window_recalculate(LIBAROMA_WINDOWP win){
  if (win==NULL){
    ALOGW("window_recalculate win is NULL");
    return 0;
  }
  int i;
  win->client_w = win->client_h = 0;
  for (i=0;i<win->childn;i++){
    int right = win->childs[i]->x + win->childs[i]->w;
    int bottom = win->childs[i]->y + win->childs[i]->h;
    if (right>win->client_w){
      win->client_w = right;
    }
    if (bottom>win->client_h){
      win->client_h = bottom;
    }
  }
  if (win->client_w<win->w){
    win->client_w=win->w;
  }
  if (win->client_h<win->h){
    win->client_h=win->h;
  }
  if (win->scroll_x>(win->client_w-win->w)){
    win->scroll_x=(win->client_w-win->w);
  }
  if (win->scroll_y>(win->client_h-win->h)){
    win->scroll_y=(win->client_h-win->h);
  }
  if (win->scroll_x<0){
    win->scroll_x=0;
  }
  if (win->scroll_y<0){
    win->scroll_y=0;
  }
  if (libaroma_window_isactive(win)){
    _libaroma_window_updatebg(win);
    libaroma_window_invalidate(win);
  }
  return 1;
} /* End of _libaroma_window_recalculate */

/*
 * Function    : libaroma_window_show
 * Return Value: byte
 * Descriptions: show window
 */
byte libaroma_window_show(LIBAROMA_WINDOWP win){
  __CHECK_WM(0);
  if (win!=NULL){
    /* set initial focus */
    libaroma_window_setfocus(win, NULL);
  }
  return libaroma_wm_set_active_window(win);
} /* End of libaroma_window_show */

/*
 * Function    : libaroma_window_resize
 * Return Value: byte
 * Descriptions: resize window
 */
byte libaroma_window_resize(
  LIBAROMA_WINDOWP win,
  int x, int y, int w, int h
){
  __CHECK_WM(0);
  if (win==NULL){
    ALOGW("window_resize win is NULL");
    return 0;
  }
  if (w==0){
    w = libaroma_wm()->w;
    x = 0;
  }
  if (h==0){
    h = libaroma_wm()->h;
    y = 0;
  }
  /* set position */
  if (win->dc!=NULL){
    libaroma_canvas_free(win->dc);
  }
  win->dc= libaroma_wm_canvas(x, y, w, h);
  if (win->dc==NULL){
    ALOGW("window_resize cannot allocate drawing canvas");
    return 0;
  }
  if (libaroma_window_isactive(win)){
    libaroma_wm_clean_workspace();
  }
  win->x = x;
  win->y = y;
  win->w = win->dc->w;
  win->h = win->dc->h;
  
  _libaroma_window_recalculate(win);
  return 1;
} /* End of libaroma_window_resize */

/*
 * Function    : libaroma_window_isactive
 * Return Value: byte
 * Descriptions: check if window is active
 */
byte libaroma_window_isactive(LIBAROMA_WINDOWP win){
  if (win!=NULL){
    return ((win==libaroma_wm_get_active_window())?1:0);
  }
  return 0;
} /* End of libaroma_window_isactive */

/*
 * Function    : libaroma_window_add_control
 * Return Value: byte
 * Descriptions: add control into window
 */
byte libaroma_window_add_control(
  LIBAROMA_WINDOWP win,
  LIBAROMA_CONTROLP ctl
){
  __CHECK_WM(0);
  if (win==NULL){
    ALOGW("window_add_control win is NULL");
    return 0;
  }
  if (ctl==NULL){
    ALOGW("window_add_control ctl is NULL");
    return 0;
  }
  if (ctl->window != NULL){
    ALOGW("window_add_control ctl already have window");
    return 0;
  }
  if (win->childn==0){
    win->childs = (LIBAROMA_CONTROLP *) malloc(sizeof(LIBAROMA_CONTROLP));
    if (!win->childs){
      ALOGW("window_add_control malloc failed");
      win->childs=NULL;
      return 0;
    }
    win->childs[0]=ctl;
  }
  else{
    LIBAROMA_CONTROLP * newchilds = (LIBAROMA_CONTROLP *)
      realloc(win->childs, sizeof(LIBAROMA_CONTROLP)*(win->childn+1));
    if (!newchilds){
      ALOGW("window_add_control realloc failed");
      return 0;
    }
    win->childs = newchilds;
    win->childs[win->childn] = ctl;
  }
  ctl->window = win;
  win->childn++;
  _libaroma_window_recalculate(win);
  return 1;
} /* End of libaroma_window_add_control */

/*
 * Function    : libaroma_window_del_control
 * Return Value: byte
 * Descriptions: delete control from window
 */
byte libaroma_window_del_control(
  LIBAROMA_CONTROLP ctl
){
  __CHECK_WM(0);
  if (ctl==NULL){
    ALOGW("window_del_control ctl is null");
    return 0;
  }
  if (ctl->window==NULL){
    ALOGW("window_del_control ctl doesn't have window");
    return 0;
  }
  LIBAROMA_WINDOWP win = ctl->window;
  if (win->childn<=0){
    ALOGW("window_del_control window data corrupt doesn't have childs??");
    return 0;
  }
  else if (win->childn==1){
    if (win->childs[0]==ctl){
      ctl->window = NULL;
      win->childn=0;
      free(win->childs);
      win->childs=NULL;
      _libaroma_window_recalculate(win);
      return 1;
    }
    else{
      ALOGW("window_del_control ctl not found in window");
      return 0;
    }
  }
  LIBAROMA_CONTROLP * newchilds = (LIBAROMA_CONTROLP *)
      malloc(sizeof(LIBAROMA_CONTROLP)*(win->childn-1));
  if (!newchilds){
    ALOGW("window_del_control malloc temp childs failed");
    return 0;
  }
  int j;
  int i;
  for (i=0;i<win->childn;i++){
    if (win->childs[i]!=ctl){
      newchilds[j++]=win->childs[i];
      if (j==win->childn-2){
        /* current ctl not found */
        free(newchilds);
        ALOGW("window_del_control ctl not found in window");
        return 0;
      }
    }
  }
  free(win->childs);
  win->childs=newchilds;
  win->childn--;
  _libaroma_window_recalculate(win);
  return 1;
} /* End of libaroma_window_del_control */

/*
 * Function    : libaroma_window_attach
 * Return Value: LIBAROMA_CONTROLP
 * Descriptions: attach control into window
 */
LIBAROMA_CONTROLP libaroma_window_attach(
  LIBAROMA_WINDOWP win,
  LIBAROMA_CONTROLP ctl){
  /* attach into window */
  if (win){
    if (libaroma_window_add_control(win,ctl)){
      return ctl;
    }
    ALOGW("window_attach cannot attach into window");
    libaroma_control_free(ctl);
    return NULL;
  }
  return ctl;
} /* End of libaroma_window_attach */

/*
 * Function    : libaroma_window_control_id
 * Return Value: LIBAROMA_CONTROLP
 * Descriptions: get control by id
 */
LIBAROMA_CONTROLP libaroma_window_control_id(
    LIBAROMA_WINDOWP win, word id){
  __CHECK_WM(NULL);
  if (win==NULL){
    ALOGW("window_control_id win is null");
    return NULL;
  }
  int i;
  for (i=0;i<win->childn;i++){
    if (win->childs[i]->id==id){
      return win->childs[i];
    }
  }
  return NULL; /* not found */
} /* End of libaroma_window_control_id */

/*
 * Function    : libaroma_window_setfocus
 * Return Value: LIBAROMA_CONTROLP
 * Descriptions: set control focus
 */
LIBAROMA_CONTROLP libaroma_window_setfocus(
    LIBAROMA_WINDOWP win, LIBAROMA_CONTROLP ctl){
  if (win==NULL){
    ALOGW("window_setfocus window is null");
    return NULL;
  }
  if (ctl!=NULL){
    /* set */
    if (win!=ctl->window){
      ALOGW("window_setfocus control is not window child");
      return NULL;
    }
    if (ctl->focus!=NULL){
      if (ctl->focus(ctl,1)){
        if (win->focused){
          win->focused->focus(win->focused,0);
        }
        win->focused=ctl;
        return ctl;
      }
    }
    return NULL;
  }
  else{
    /* find focus */
    if (win->focused){
      return win->focused;
    }
    int i;
    for (i=0;i<win->childn;i++){
      if (win->childs[i]->focus!=NULL){
        if (win->childs[i]->focus(win->childs[i],1)){
          win->focused = win->childs[i];
          return win->childs[i];
        }
      }
    }
  }
  return NULL;
} /* End of libaroma_window_setfocus */

/*
 * Function    : libaroma_window_event
 * Return Value: byte
 * Descriptions: process message
 */
byte libaroma_window_event(LIBAROMA_WINDOWP win, LIBAROMA_MSGP msg){
  __CHECK_WM(0);
  if (win==NULL){
    ALOGW("window_event win is null");
    return 0;
  }
  switch (msg->msg){
    case LIBAROMA_MSG_WIN_ACTIVE:
      {
        libaroma_window_resize(win, win->x, win->y, win->w, win->h);
      }
      break;
    case LIBAROMA_MSG_WIN_INACTIVE:
      {
        /* wait for thread if any */
        while (win->threadn>0) {
          usleep(500);
        }
      }
      break;
    case LIBAROMA_MSG_WIN_INVALIDATE:
      {
        libaroma_window_invalidate(win);
      }
      break;
  }
  
  return msg->msg;
} /* End of libaroma_window_event */

/*
 * Function    : libaroma_window_invalidate
 * Return Value: byte
 * Descriptions: invalidate window drawing
 */
byte libaroma_window_invalidate(LIBAROMA_WINDOWP win){
  __CHECK_WM(0);
  if (win==NULL){
    ALOGW("window_invalidate win is null");
    return 0;
  }
  if (!libaroma_window_isactive(win)){
    ALOGW("window_invalidate win is not active window");
    return 0;
  }
  if (win->dc==NULL){
    ALOGW("window_invalidate dc is null");
    return 0;
  }
  
  /* draw bg */
  libaroma_draw(
    win->dc,
    win->bg,
    0, 0, 1);
  
  /* draw childs */
  int i;
  for (i=0;i<win->childn;i++){
    /* draw no sync */
    libaroma_control_draw(win->childs[i], 0);
  }
  
  /* sync */
  libaroma_wm_sync(win->x, win->y, win->w, win->h);
  return 1;
} /* End of libaroma_window_invalidate */

#undef __CHECK_WM
#endif /* __libaroma_window_c__ */
