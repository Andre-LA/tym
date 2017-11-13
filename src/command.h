/**
 * command.h
 *
 * Copyright (c) 2017 endaaman
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef COMMAND_H
#define COMMAND_H

#include "common.h"
#include "context.h"

void command_notify(Context* context, const char* body, const char* title);
void command_reload(Context* context);
void command_copy_clipboard(Context* context);
void command_paste_clipboard(Context* context);
void command_increase_font_scale(Context* context);
void command_decrease_font_scale(Context* context);
void command_reset_font_scale(Context* context);

#endif
