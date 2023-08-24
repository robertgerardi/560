#include "UI.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <cmsis_os2.h>

#include "LCD.h"
#include "colors.h"
#include "ST7789.h"
#include "T6963.h"
#include "font.h"
#include "FX.h"
#include "MMA8451.h"
#include "debug.h"
#include "timers.h"
#include "logger.h"
#include "profile.h"

PT_T scope_tl = {0,0};
PT_T scope_br = {UI_SCOPE_WIDTH-1, UI_SCOPE_HEIGHT-1};
PT_T scope_c = {UI_SCOPE_WIDTH/2, UI_SCOPE_HEIGHT/2};

COLOR_T * data_color[3] = {&light_red, &light_green, &light_blue};

// Label, Units, Buffer, *Val, *VatT, {row,column},
// fg, bg, Updated, Selected, ReadOnly, Volatile, *Handler 

UI_FIELD_T Fields[] = {
	// Start Recording
	{"Recording      ", "", "", (volatile int *)&g_mode, NULL, {0,10},
	&green, &black, 1, 0, 0, 1, Logger_Mode_Handler},
	{"Backlight     ", "%", "", (volatile int *)&g_backlight_pct, NULL, {0, 11},
	&green, &black, 1, 0, 0, 0, UI_Control_IntNonNegative_Handler},
	{"Sample Prd. ", " ms", "",  (volatile int *)&g_sampling_period, NULL, {0, 12},
	&green, &black, 1, 0, 0, 0, UI_Control_IntNonNegative_Handler},
	{"Num. Samples   ", "", "",  (volatile int *)&g_samples_wanted, NULL, {0, 13},
	&green, &black, 1, 0, 0, 0, UI_Control_IntNonNegative_Handler},
	{"Samples Used   ", "", "", (volatile int *)&g_samples_used, NULL, {0,14},
	&green, &black, 1, 0, 1, 1, NULL},
};

UI_SLIDER_T Slider = {
	0, {0,LCD_HEIGHT-UI_SLIDER_HEIGHT}, {UI_SLIDER_WIDTH-1,LCD_HEIGHT-1}, 
	{119,LCD_HEIGHT-UI_SLIDER_HEIGHT}, {119,LCD_HEIGHT-1}, &white, &dark_gray, &light_gray
};

int UI_sel_field = -1;

#ifdef OPT_PARTIAL_FIELD_UPDATE
int PrevVal[10];

void UI_Update_Field_Values (UI_FIELD_T * f, int num) {
	int i;
	
	for (i=0; i < num; i++) {
		if (f[i].Val) {			// REMOVE
			if (*f[i].Val != PrevVal[i]) {
				snprintf(f[i].Buffer, sizeof(f[i].Buffer), "%s%4d%s", f[i].Label, 
					f[i].Val? *(f[i].Val) : 0, f[i].Units);
				f[i].Updated = 1;
			} else {
				f[i].Updated = 0;
			}
			PrevVal[i] = *f[i].Val;
		}	
	}
}
#else
void UI_Update_Field_Values (UI_FIELD_T * f, int num) {
	int i;
	for (i=0; i < num; i++) {
		snprintf(f[i].Buffer, sizeof(f[i].Buffer), "%s%4d%s", f[i].Label, 
			f[i].Val? *(f[i].Val) : 0, f[i].Units);
		f[i].Updated = 1;
	}	
}
#endif

#ifdef OPT_PARTIAL_FIELD_UPDATE
void UI_Update_Volatile_Field_Values(UI_FIELD_T * f) {
	int i;

	for (i=0; i < UI_NUM_FIELDS; i++) {
		if (f[i].Volatile) {
			if (f[i].Val) {			// REMOVE
				if (*f[i].Val != PrevVal[i]) {
					snprintf(f[i].Buffer, sizeof(f[i].Buffer), "%s%4d%s", f[i].Label, 
						f[i].Val? *(f[i].Val) : 0, f[i].Units);
					f[i].Updated = 1;
				} else {
					f[i].Updated = 0;
				}
				PrevVal[i] = *f[i].Val;
			}
		}
	}	
}
#else
void UI_Update_Volatile_Field_Values(UI_FIELD_T * f) {
	int i;
	for (i=0; i < UI_NUM_FIELDS; i++) {
		if (f[i].Volatile) {
			snprintf(f[i].Buffer, sizeof(f[i].Buffer), "%s%4d%s", f[i].Label, 
				f[i].Val? *(f[i].Val) : 0, f[i].Units);
			f[i].Updated = 1;
		}
	}
}
#endif

#ifdef OPT_PARTIAL_FIELD_UPDATE
void UI_Draw_Fields(UI_FIELD_T * f, int num){
	int i;
	COLOR_T * bg_color, *fg_color;
	for (i=0; i < num; i++) {
		if ((f[i].Updated) ){ // || (f[i].Volatile)) { // redraw updated or volatile fields
			f[i].Updated = 0;
			if ((f[i].Selected) && (!f[i].ReadOnly)) {
				bg_color = &dark_red;
			} else {
				bg_color = f[i].ColorBG;
			}
			if (f[i].ReadOnly) {
				fg_color = &light_gray;
			} else {
				fg_color = f[i].ColorFG;
			}
			LCD_Text_Set_Colors(fg_color, bg_color);
			DEBUG_START(DBG_4); // REMOVE
			LCD_Text_PrintStr_RC(f[i].RC.Y, f[i].RC.X, f[i].Buffer);
			DEBUG_STOP(DBG_4); // REMOVE
		}
	}
}
#else
void UI_Draw_Fields(UI_FIELD_T * f, int num){
	int i;
	COLOR_T * bg_color, *fg_color;
	for (i=0; i < num; i++) {
		if ((f[i].Updated) || (f[i].Volatile)) { // redraw updated or volatile fields
			f[i].Updated = 0;
			if ((f[i].Selected) && (!f[i].ReadOnly)) {
				bg_color = &dark_red;
			} else {
				bg_color = f[i].ColorBG;
			}
			if (f[i].ReadOnly) {
				fg_color = &light_gray;
			} else {
				fg_color = f[i].ColorFG;
			}
			LCD_Text_Set_Colors(fg_color, bg_color);
			LCD_Text_PrintStr_RC(f[i].RC.Y, f[i].RC.X, f[i].Buffer);
		}
	}
}
#endif

void UI_Draw_Slider(UI_SLIDER_T * s) {
	static int initialized=0;
	
	if (!initialized) {
		LCD_Fill_Rectangle(&s->UL, &s->LR, s->ColorBG);
		initialized = 1;
	}
	LCD_Fill_Rectangle(&s->BarUL, &s->BarLR, s->ColorBG); // Erase old bar
	
	// Y coordinates are same, don't change
	s->BarUL.Y = s->UL.Y;
	s->BarLR.Y = s->LR.Y;
	
	// Find X-coord. middle of bar, add Val offset
	s->BarUL.X = (s->LR.X - s->UL.X)/2 + s->Val;
	// Adjust for width of bar
	s->BarLR.X = s->BarUL.X + UI_SLIDER_BAR_WIDTH/2;
	s->BarUL.X -= UI_SLIDER_BAR_WIDTH/2;
	// Draw the new bar
	LCD_Fill_Rectangle(&s->BarUL, &s->BarLR, s->ColorFG);
}

int UI_Identify_Field(PT_T * p) {
	int i, t, b, l, r;

	if ((p->X >= LCD_WIDTH) || (p->Y >= LCD_HEIGHT)) {
		return -1;
	}

	if ((p->X >= Slider.UL.X) && (p->X <= Slider.LR.X) 
		&& (p->Y >= Slider.UL.Y) && (p->Y <= Slider.LR.Y)) {
		return UI_SLIDER;
	}
  for (i=0; i<UI_NUM_FIELDS; i++) {
		l = COL_TO_X(Fields[i].RC.X);
		r = l + strlen(Fields[i].Buffer)*CHAR_WIDTH;
		t = ROW_TO_Y(Fields[i].RC.Y);
		b = t + CHAR_HEIGHT-1;
		
		if ((p->X >= l) && (p->X <= r) 
			&& (p->Y >= t) && (p->Y <= b) ) {
			return i;
		}
	}
	return -1;
}

void UI_Update_Field_Selects(int sel) {
	int i;
	for (i=0; i < UI_NUM_FIELDS; i++) {
		Fields[i].Selected = (i == sel)? 1 : 0;
	}
}

void UI_Process_Touch(PT_T * p) {  // Called by Thread_Read_TS
	int i;
	
	i = UI_Identify_Field(p);
	if (i == UI_SLIDER) {
		// Determine slider position (value)
		Slider.Val = p->X - (Slider.LR.X - Slider.UL.X)/2; 
		if (UI_sel_field >= 0) {  // If a field is selected...
			if (Fields[UI_sel_field].Val != NULL) {
				if (Fields[UI_sel_field].Handler != NULL) {
					// Have the field handle the new slider value
					(*Fields[UI_sel_field].Handler)(&Fields[UI_sel_field], Slider.Val); 
				}
				UI_Update_Field_Values(&Fields[UI_sel_field], 1);
			}
		}
	} else if (i>=0) {
		if (!Fields[i].ReadOnly) { // Can't select (and modify) a ReadOnly field
			UI_sel_field = i;
			UI_Update_Field_Selects(UI_sel_field);
			UI_Update_Field_Values(Fields, UI_NUM_FIELDS);
			Slider.Val = 0; // return to slider to zero if a different field is selected
		}
	} 
}

void UI_Draw_Blank_Scope(void) {
	PT_T p1 = {0,0},	p2 = {0,0};
	
	LCD_Fill_Rectangle(&scope_tl, &scope_br, &black);
	p2.X = UI_SCOPE_WIDTH-1;
	LCD_Draw_Line(&p1, &p2, &light_gray);
	p1 = p2;
	p2.Y = UI_SCOPE_HEIGHT-1;
	LCD_Draw_Line(&p1, &p2, &light_gray);
	p1 = p2;
	p2.X = 0;
	LCD_Draw_Line(&p1, &p2, &light_gray);
	p1 = p2;
	p2.Y = 0;
	LCD_Draw_Line(&p1, &p2, &light_gray);

	p1.Y = UI_SCOPE_HEIGHT/2;
	p2 = p1;
	p2.X = UI_SCOPE_WIDTH-1;
	LCD_Draw_Line(&p1, &p2, &dark_gray);
}

void UI_Draw_Scope(void) {
	int num_samples;
	float x_step;
	PT_T p1, p2;
	float y_scale = -(0.5*UI_SCOPE_HEIGHT/(0.5*COUNTS_PER_G)); 
	
	if (g_redraw_scope) {
		g_redraw_scope = 0;
		UI_Draw_Blank_Scope();
		
		num_samples = g_samples_used;
		if (num_samples < UI_SCOPE_WIDTH) {
			float x = 0;
			x_step = ((float) UI_SCOPE_WIDTH)/num_samples;
			for (int i=0; i< num_samples-1; i++) {
				for (int axis = 0; axis < 3; axis++) {
					p1.X = (int) x;
					p1.Y = scope_c.Y + y_scale*Acc_log[axis].Data[i];
					p1.Y = MIN(p1.Y, scope_br.Y-1);
					p1.Y = MAX(p1.Y, scope_tl.Y+1);
					
					p2.X = (int) (x + x_step);
					p2.Y = scope_c.Y + y_scale*Acc_log[axis].Data[i+1];
					p2.Y = MIN(p2.Y, scope_br.Y-1);
					p2.Y = MAX(p2.Y, scope_tl.Y+1);
					LCD_Draw_Line(&p1, &p2, data_color[axis]);
				}
				x += x_step;
			}
		} else {
			for (int axis = 0; axis < 3; axis++) {
				for (int i=0; i< num_samples-1; i++) {
					p1.X = i*((float)UI_SCOPE_WIDTH/num_samples);
					p1.Y = scope_c.Y + y_scale*Acc_log[axis].Data[i];
					p1.Y = MIN(p1.Y, scope_br.Y-1);
					p1.Y = MAX(p1.Y, scope_tl.Y+1);
					LCD_Plot_Pixel(&p1, data_color[axis]);
				}
			}
		}
		
#if USE_PROFILER & SHOW_PROFILE_AFTER_SCOPE
		Disable_Profiling(); 
		Sort_Profile_Regions();
		osDelay(3000); // Display scope for limited time, then show profile
		Display_Profile(); 
#if OPT_PROFILE_ONCE
		PIT_Stop();
#else
		Enable_Profiling();
#endif
		
		// Redraw text on screen
		UI_Update_Field_Values(Fields, UI_NUM_FIELDS);
		UI_Draw_Fields(Fields, UI_NUM_FIELDS);
		UI_Draw_Slider(&Slider);
#endif
	}
}

void UI_Draw_Screen(int first_time) { // Called by Thread_Update_Screen
	if (first_time) {
		UI_Update_Field_Values(Fields, UI_NUM_FIELDS);
	}
	
	UI_Update_Volatile_Field_Values(Fields);
	UI_Draw_Fields(Fields, UI_NUM_FIELDS);
	UI_Draw_Slider(&Slider);
	UI_Draw_Scope();
}

// Handler functions (callbacks)
// Default handlers
void UI_Control_OnOff_Handler (UI_FIELD_T * fld, int v) {
	if (fld->Val != NULL) {
		if (v > 0) {
			*fld->Val = 1;
		} else {
			*fld->Val = 0;
		}
	}
}

void UI_Control_IntNonNegative_Handler (UI_FIELD_T * fld, int v) {
	int n;
	if (fld->Val != NULL) {
		n = *fld->Val + v/16;
		if (n < 0) {
			n = 0;
		}
		*fld->Val = n;
	}
}
