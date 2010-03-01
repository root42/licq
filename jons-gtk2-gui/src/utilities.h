#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include <gtk/gtk.h>
#include <string>

char *
convert_to_utf8(const char *input_text, const char *input_enc = NULL);
char *
convert_from_utf8(const char *input_text, const char *output_enc);

std::string
s_convert_to_utf8(const char *input_text, const char *input_enc = NULL);

std::string
entry_get_chars(GtkWidget *w);

std::string
textview_get_chars(GtkWidget *w);

void
status_change(GtkWidget *statusbar, const char *st_name, const char *newstatus);

GtkWidget *
hbutton_box_new(int spacing = 5, 
    GtkButtonBoxStyle layout_style = GTK_BUTTONBOX_END);

struct encoding {
	char *name;
	char *enc;
};

extern encoding encodings[];
#endif