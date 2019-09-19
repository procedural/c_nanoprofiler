#!/usr/bin/python3

#
#   A script that instruments a C header file with procedure declarations.
#
#   Assumptions:
# * Remove all comments
# * Leave only procedure return types with no API macros
# * Don't typedef void procedure return types
# * Replace [] procedure parameter types with *
# * Procedure and parameter names should be in ASCII
# * No spaces between procedure name and (
# * No 2 or more spaces, space everything with 1 space character
# * Generally, procedures should look like this:
#   proc_type proc_name(proc_param_type proc_param_name, ...);

import sys

with open(str(sys.argv[1]), encoding='utf-8') as fd:
  chars = fd.read()

  proc_types = []
  proc_names = []
  proc_param_types = []
  proc_param_names = []

  start_parsing = False
  proc_type = ''
  proc_name = ''
  param_type = ''
  param_name = ''
  param_recording = False
  param_recording_name = False
  proc_recording_name = False
  for c in reversed(chars):
    if c == '\n':
      continue
    if c == ';':
      proc_param_types.insert(0, [])
      proc_param_names.insert(0, [])
      if start_parsing == False:
        start_parsing = True
        continue
      proc_types.insert(0, proc_type[::-1])
      proc_names.insert(0, proc_name[::-1])
      proc_type = ''
      proc_name = ''
      continue
    if c == ')':
      param_recording = True
      param_recording_name = True
      continue
    if c == '(':
      proc_param_types[0].insert(0, param_type[::-1])
      param_type = ''
      param_recording = False
      param_recording_name = False
      proc_recording_name = True
      continue
    if c == ',':
      proc_param_types[0].insert(0, param_type[::-1])
      param_type = ''
      param_recording_name = True
      continue
    if param_recording == True:
      if param_recording_name == True:
        if c not in '_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789':
          proc_param_names[0].insert(0, param_name[::-1])
          param_name = ''
          param_recording_name = False
          param_type += c
          continue
        param_name += c
      else:
        param_type += c
      continue
    if start_parsing == True:
      if proc_recording_name == True:
        if c not in '_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789':
          proc_recording_name = False
          proc_type += c
          continue
        proc_name += c
        continue
      proc_type += c
  proc_types.insert(0, proc_type[::-1])
  proc_names.insert(0, proc_name[::-1])
  proc_type = ''
  proc_name = ''

  #print(proc_types)
  #print()
  #print(proc_names)
  #print()
  #print(proc_param_types)
  #print()
  #print(proc_param_names)
  #print()

  for proc_name in proc_names:
    if proc_name == '':
      print('Error: one of the procedure names has invalid characters before (')
      print()

  print('#pragma once')
  print()
  print('#include "nanoprofiler.h"')
  print()

  for i, proc_name in enumerate(proc_names):
    proc_type = proc_types[i]
    param_names = proc_param_names[i]
    param_types = proc_param_types[i]
    should_return_value = proc_type != 'void '
    if len(param_names) == 0:
      print('static inline ', proc_type, 'np_', proc_name, '(int ___nanoprofiler_thread_id', sep='', end='')
    else:
      print('static inline ', proc_type, 'np_', proc_name, '(int ___nanoprofiler_thread_id, ', sep='', end='')
    for j, param_name in enumerate(param_names):
      param_type = param_types[j]
      if j == len(param_names) - 1:
        print(param_type, param_name, sep='', end='')
      else:
        print(param_type, param_name, ',', sep='', end='')
    print(') {')
    print('  NanoprofilerBegin(___nanoprofiler_thread_id, "', proc_name, '");', sep='')
    if should_return_value == True:
      print('  ', proc_type, '___nanoprofiler_procedure_result = ', proc_name, '(', sep='', end='')
    else:
      print('  ', proc_name, '(', sep='', end='')
    for j, param_name in enumerate(param_names):
      if j == len(param_names) - 1:
        print(param_name, sep='', end='')
      else:
        print(param_name, ', ', sep='', end='')
    print(');')
    print('  NanoprofilerEnd(___nanoprofiler_thread_id, "', proc_name, '");', sep='')
    if should_return_value == True:
      print('  return ___nanoprofiler_procedure_result;')
    print('}')
    print()

  for i, proc_name in enumerate(proc_names):
    print('#define ', proc_name, ' np_', proc_name, sep='')
