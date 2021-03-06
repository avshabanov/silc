/*
 * Copyright 2015 Alexander Shabanov - http://alexshabanov.com.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "silc.h"

silc_obj silc_internal_fn_define(struct silc_funcall_t* f);

silc_obj silc_internal_fn_lambda(struct silc_funcall_t* f);

silc_obj silc_internal_fn_print(struct silc_funcall_t* f);

silc_obj silc_internal_fn_cons(struct silc_funcall_t* f);

silc_obj silc_internal_fn_inc(struct silc_funcall_t* f);
silc_obj silc_internal_fn_plus(struct silc_funcall_t* f);
silc_obj silc_internal_fn_minus(struct silc_funcall_t* f);
silc_obj silc_internal_fn_div(struct silc_funcall_t* f);
silc_obj silc_internal_fn_mul(struct silc_funcall_t* f);

silc_obj silc_internal_fn_load(struct silc_funcall_t* f);

silc_obj silc_internal_fn_gc(struct silc_funcall_t* f);

silc_obj silc_internal_fn_quit(struct silc_funcall_t* f);

silc_obj silc_internal_fn_begin(struct silc_funcall_t* f);
