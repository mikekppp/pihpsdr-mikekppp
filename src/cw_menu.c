/* Copyright (C)
* 2015 - John Melton, G0ORX/N6LYT
* 2025 - Christoph van Wüllen, DL1YCF
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*
*/

#include <gtk/gtk.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "band.h"
#include "bandstack.h"
#include "ext.h"
#include "filter.h"
#include "iambic.h"
#include "new_menu.h"
#include "new_protocol.h"
#include "pa_menu.h"
#include "old_protocol.h"
#include "radio.h"
#include "receiver.h"

static GtkWidget *dialog = NULL;

void cw_set_sidetone_freq(int val) {
  cw_keyer_sidetone_frequency = val;

  if (radio_is_remote) {
    send_sidetone_freq(client_socket, cw_keyer_sidetone_frequency);
  } else {
    rx_filter_changed(active_receiver);
    // changing the side tone frequency affects BFO frequency offsets
    schedule_high_priority();
    schedule_transmit_specific();
  }

  g_idle_add(ext_vfo_update, NULL);
}

static void cw_changed() {
  // inform the local keyer about CW parameter changes
  // NewProtocol: rely on periodically sent HighPrio packets
  keyer_update();

  if (!radio_is_remote) { schedule_transmit_specific(); }

  //
  // speed and side tone frequency are displayed in the VFO bar
  //
  g_idle_add(ext_vfo_update, NULL);
}

static void cleanup() {
  if (dialog != NULL) {
    GtkWidget *tmp = dialog;
    dialog = NULL;
    gtk_widget_destroy(tmp);
    sub_menu = NULL;
    active_menu  = NO_MENU;
    radio_save_state();
  }
}

static gboolean close_cb () {
  cleanup();
  return TRUE;
}

static void cw_keyer_internal_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_internal = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  cw_changed();
}

static void cw_keyer_spacing_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_spacing = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  cw_changed();
}

static void cw_keyer_speed_value_changed_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_speed = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  cw_changed();
}

static void cw_breakin_cb(GtkWidget *widget, gpointer data) {
  cw_breakin = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  cw_changed();
}

static void cw_keyer_hang_time_value_changed_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_hang_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  cw_changed();
}

static void cw_keyer_weight_value_changed_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_weight = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  cw_changed();
}

static void cw_keys_reversed_cb(GtkWidget *widget, gpointer data) {
  cw_keys_reversed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  cw_changed();
}

static void cw_keyer_mode_cb(GtkToggleButton *widget, gpointer data) {
  int val = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
  cw_keyer_mode = val;
  cw_changed();
}

static void cw_keyer_sidetone_level_value_changed_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_sidetone_volume = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  cw_changed();
}

static void cw_keyer_sidetone_frequency_value_changed_cb(GtkWidget *widget, gpointer data) {
  cw_keyer_sidetone_frequency = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  cw_changed();

  if (radio_is_remote) {
    send_sidetone_freq(client_socket, cw_keyer_sidetone_frequency);
  } else {
    rx_filter_changed(active_receiver);
    // changing the side tone frequency affects BFO frequency offsets
    schedule_high_priority();
  }
}

#if 0
static void cw_ramp_width_changed_cb(GtkWidget *widget, gpointer data) {
  cw_ramp_width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  tx_set_ramps(transmitter);
  schedule_transmit_specific();
}

#endif

void cw_menu(GtkWidget *parent) {
  dialog = gtk_dialog_new();
  int col;
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
  GtkWidget *headerbar = gtk_header_bar_new();
  gtk_window_set_titlebar(GTK_WINDOW(dialog), headerbar);
  gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(headerbar), TRUE);
  gtk_header_bar_set_title(GTK_HEADER_BAR(headerbar), "piHPSDR - CW");
  g_signal_connect (dialog, "delete_event", G_CALLBACK (close_cb), NULL);
  g_signal_connect (dialog, "destroy", G_CALLBACK (close_cb), NULL);
  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_column_spacing (GTK_GRID(grid), 10);
  gtk_grid_set_column_homogeneous (GTK_GRID(grid), FALSE);
  gtk_grid_set_row_homogeneous (GTK_GRID(grid), TRUE);
  col = 0;
  GtkWidget *close_b = gtk_button_new_with_label("Close");
  gtk_widget_set_name(close_b, "close_button");
  g_signal_connect (close_b, "button-press-event", G_CALLBACK(close_cb), NULL);
  gtk_grid_attach(GTK_GRID(grid), close_b, 0, col, 1, 1);

  if (!radio_is_remote) {
    GtkWidget *cw_keyer_internal_b = gtk_check_button_new_with_label("CW handled in Radio");
    gtk_widget_set_name(cw_keyer_internal_b, "boldlabel");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_keyer_internal_b), cw_keyer_internal);
    gtk_widget_show(cw_keyer_internal_b);
    gtk_grid_attach(GTK_GRID(grid), cw_keyer_internal_b, 1, col, 1, 1);
    g_signal_connect(cw_keyer_internal_b, "toggled", G_CALLBACK(cw_keyer_internal_cb), NULL);
  }

  col++;
  GtkWidget *cw_speed_label = gtk_label_new("CW Speed (WPM)");
  gtk_widget_set_name(cw_speed_label, "boldlabel");
  gtk_widget_set_halign(cw_speed_label, GTK_ALIGN_END);
  gtk_widget_show(cw_speed_label);
  gtk_grid_attach(GTK_GRID(grid), cw_speed_label, 0, col, 1, 1);
  GtkWidget *cw_keyer_speed_b = gtk_spin_button_new_with_range(1.0, 60.0, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_speed_b), (double)cw_keyer_speed);
  gtk_widget_show(cw_keyer_speed_b);
  gtk_grid_attach(GTK_GRID(grid), cw_keyer_speed_b, 1, col, 1, 1);
  g_signal_connect(cw_keyer_speed_b, "value_changed", G_CALLBACK(cw_keyer_speed_value_changed_cb), NULL);
  col++;
  GtkWidget *cw_breakin_b = gtk_check_button_new_with_label("CW Break-In, Delay (ms):");
  gtk_widget_set_name(cw_breakin_b, "boldlabel");
  gtk_widget_set_halign(cw_breakin_b, GTK_ALIGN_END);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_breakin_b), cw_breakin);
  gtk_widget_show(cw_breakin_b);
  gtk_grid_attach(GTK_GRID(grid), cw_breakin_b, 0, col, 1, 1);
  g_signal_connect(cw_breakin_b, "toggled", G_CALLBACK(cw_breakin_cb), NULL);
  GtkWidget *cw_keyer_hang_time_b = gtk_spin_button_new_with_range(0.0, 1000.0, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_hang_time_b), (double)cw_keyer_hang_time);
  gtk_widget_show(cw_keyer_hang_time_b);
  gtk_grid_attach(GTK_GRID(grid), cw_keyer_hang_time_b, 1, col, 1, 1);
  g_signal_connect(cw_keyer_hang_time_b, "value_changed", G_CALLBACK(cw_keyer_hang_time_value_changed_cb), NULL);
  col++;
  GtkWidget *cw_keyer_sidetone_level_label = gtk_label_new("Sidetone Level:");
  gtk_widget_set_name(cw_keyer_sidetone_level_label, "boldlabel");
  gtk_widget_set_halign(cw_keyer_sidetone_level_label, GTK_ALIGN_END);
  gtk_widget_show(cw_keyer_sidetone_level_label);
  gtk_grid_attach(GTK_GRID(grid), cw_keyer_sidetone_level_label, 0, col, 1, 1);
  GtkWidget *cw_keyer_sidetone_level_b = gtk_spin_button_new_with_range(0.0, 127.0, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_sidetone_level_b), (double)cw_keyer_sidetone_volume);
  gtk_widget_show(cw_keyer_sidetone_level_b);
  gtk_grid_attach(GTK_GRID(grid), cw_keyer_sidetone_level_b, 1, col, 1, 1);
  g_signal_connect(cw_keyer_sidetone_level_b, "value_changed", G_CALLBACK(cw_keyer_sidetone_level_value_changed_cb),
                   NULL);
  col++;
  GtkWidget *cw_keyer_sidetone_frequency_label = gtk_label_new("Sidetone Freq:");
  gtk_widget_set_name(cw_keyer_sidetone_frequency_label, "boldlabel");
  gtk_widget_set_halign(cw_keyer_sidetone_frequency_label, GTK_ALIGN_END);
  gtk_widget_show(cw_keyer_sidetone_frequency_label);
  gtk_grid_attach(GTK_GRID(grid), cw_keyer_sidetone_frequency_label, 0, col, 1, 1);
  GtkWidget *cw_keyer_sidetone_frequency_b = gtk_spin_button_new_with_range(100.0, 1000.0, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_sidetone_frequency_b), (double)cw_keyer_sidetone_frequency);
  gtk_widget_show(cw_keyer_sidetone_frequency_b);
  gtk_grid_attach(GTK_GRID(grid), cw_keyer_sidetone_frequency_b, 1, col, 1, 1);
  g_signal_connect(cw_keyer_sidetone_frequency_b, "value_changed",
                   G_CALLBACK(cw_keyer_sidetone_frequency_value_changed_cb), NULL);
  col++;
  GtkWidget *cw_keyer_weight_label = gtk_label_new("Weight:");
  gtk_widget_set_name(cw_keyer_weight_label, "boldlabel");
  gtk_widget_set_halign(cw_keyer_weight_label, GTK_ALIGN_END);
  gtk_widget_show(cw_keyer_weight_label);
  gtk_grid_attach(GTK_GRID(grid), cw_keyer_weight_label, 0, col, 1, 1);
  GtkWidget *cw_keyer_weight_b = gtk_spin_button_new_with_range(0.0, 100.0, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_weight_b), (double)cw_keyer_weight);
  gtk_widget_show(cw_keyer_weight_b);
  gtk_grid_attach(GTK_GRID(grid), cw_keyer_weight_b, 1, col, 1, 1);
  g_signal_connect(cw_keyer_weight_b, "value_changed", G_CALLBACK(cw_keyer_weight_value_changed_cb), NULL);
#if 0
  col++;
  GtkWidget *cw_ramp_width_label = gtk_label_new("CW ramp width (ms):");
  gtk_widget_set_name(cw_ramp_width_label, "boldlabel");
  gtk_widget_set_halign(cw_ramp_width_label, GTK_ALIGN_END);
  gtk_widget_show(cw_ramp_width_label);
  gtk_grid_attach(GTK_GRID(grid), cw_ramp_width_label, 0, col, 1, 1);
  GtkWidget *cw_ramp_width_b = gtk_spin_button_new_with_range(5.0, 16.0, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_ramp_width_b), cw_ramp_width);
  gtk_widget_show(cw_ramp_width_b);
  gtk_grid_attach(GTK_GRID(grid), cw_ramp_width_b, 1, col, 1, 1);
  g_signal_connect(cw_ramp_width_b, "value_changed", G_CALLBACK(cw_ramp_width_changed_cb), NULL);
#endif
  col++;
  GtkWidget *paddle_label = gtk_label_new("Paddle Mode:");
  gtk_widget_set_name(paddle_label, "boldlabel");
  gtk_widget_set_halign(paddle_label, GTK_ALIGN_END);
  gtk_widget_show(paddle_label);
  gtk_grid_attach(GTK_GRID(grid), paddle_label, 0, col, 1, 1);
  GtkWidget *mode_combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(mode_combo), NULL, "Straight Key");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(mode_combo), NULL, "Iambic Mode A");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(mode_combo), NULL, "Iambic Mode B");
  gtk_combo_box_set_active(GTK_COMBO_BOX(mode_combo), cw_keyer_mode);
  gtk_widget_show(mode_combo);
  my_combo_attach(GTK_GRID(grid), mode_combo, 1, col, 1, 1);
  g_signal_connect(mode_combo, "changed", G_CALLBACK(cw_keyer_mode_cb), NULL);
  col++;
  GtkWidget *cw_keys_reversed_b = gtk_check_button_new_with_label("Keys reversed");
  gtk_widget_set_name(cw_keys_reversed_b, "boldlabel");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_keys_reversed_b), cw_keys_reversed);
  gtk_widget_show(cw_keys_reversed_b);
  gtk_grid_attach(GTK_GRID(grid), cw_keys_reversed_b, 1, col, 1, 1);
  g_signal_connect(cw_keys_reversed_b, "toggled", G_CALLBACK(cw_keys_reversed_cb), NULL);
  GtkWidget *cw_keyer_spacing_b = gtk_check_button_new_with_label("Enforce letter spacing");
  gtk_widget_set_name(cw_keyer_spacing_b, "boldlabel");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_keyer_spacing_b), cw_keyer_spacing);
  gtk_widget_show(cw_keyer_spacing_b);
  gtk_grid_attach(GTK_GRID(grid), cw_keyer_spacing_b, 0, col, 1, 1);
  g_signal_connect(cw_keyer_spacing_b, "toggled", G_CALLBACK(cw_keyer_spacing_cb), NULL);
  gtk_container_add(GTK_CONTAINER(content), grid);
  sub_menu = dialog;
  gtk_widget_show_all(dialog);
}
