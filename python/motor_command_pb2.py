# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: motor_command.proto

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import nanopb_pb2 as nanopb__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='motor_command.proto',
  package='',
  syntax='proto2',
  serialized_options=None,
  serialized_pb=b'\n\x13motor_command.proto\x1a\x0cnanopb.proto\"X\n\x0eMotorInitProto\x12\x0f\n\x07\x61\x64\x64ress\x18\x01 \x02(\x05\x12\x12\n\nenable_pin\x18\x02 \x02(\x05\x12\x0f\n\x07\x64ir_pin\x18\x03 \x02(\x05\x12\x10\n\x08step_pin\x18\x04 \x02(\x05\"\x82\x01\n\x0eMotorMoveProto\x12\x11\n\tmax_speed\x18\x01 \x02(\x02\x12\x11\n\tmin_speed\x18\x02 \x02(\x02\x12\x1c\n\x14\x64isable_after_moving\x18\x03 \x02(\x08\x12\x16\n\x0e\x61\x62solute_steps\x18\x04 \x02(\x05\x12\x14\n\x0c\x61\x63\x63\x65leration\x18\x05 \x02(\x02\"W\n\x10MotorConfigProto\x12\x0f\n\x07\x61\x64\x64ress\x18\x01 \x02(\x05\x12\x0c\n\x04zero\x18\x05 \x02(\x08\x12\x11\n\tmin_steps\x18\x06 \x02(\x05\x12\x11\n\tmax_steps\x18\x07 \x02(\x05\"A\n\x11MotorMoveAllProto\x12,\n\x06motors\x18\x01 \x03(\x0b\x32\x0f.MotorMoveProtoB\x0b\x92?\x02\x10\x06\x92?\x03\x80\x01\x01\"8\n\x0eMotorTareProto\x12\x0f\n\x07\x61\x64\x64ress\x18\x01 \x02(\x05\x12\x15\n\rtare_to_steps\x18\x02 \x02(\x05\"2\n\x10MotorReportProto\x12\x1e\n\x16\x63urrent_absolute_steps\x18\x01 \x02(\x05\"E\n\x13\x41llMotorReportProto\x12.\n\x06motors\x18\x01 \x03(\x0b\x32\x11.MotorReportProtoB\x0b\x92?\x02\x10\x06\x92?\x03\x80\x01\x01'
  ,
  dependencies=[nanopb__pb2.DESCRIPTOR,])




_MOTORINITPROTO = _descriptor.Descriptor(
  name='MotorInitProto',
  full_name='MotorInitProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='address', full_name='MotorInitProto.address', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='enable_pin', full_name='MotorInitProto.enable_pin', index=1,
      number=2, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='dir_pin', full_name='MotorInitProto.dir_pin', index=2,
      number=3, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='step_pin', full_name='MotorInitProto.step_pin', index=3,
      number=4, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=37,
  serialized_end=125,
)


_MOTORMOVEPROTO = _descriptor.Descriptor(
  name='MotorMoveProto',
  full_name='MotorMoveProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='max_speed', full_name='MotorMoveProto.max_speed', index=0,
      number=1, type=2, cpp_type=6, label=2,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='min_speed', full_name='MotorMoveProto.min_speed', index=1,
      number=2, type=2, cpp_type=6, label=2,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='disable_after_moving', full_name='MotorMoveProto.disable_after_moving', index=2,
      number=3, type=8, cpp_type=7, label=2,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='absolute_steps', full_name='MotorMoveProto.absolute_steps', index=3,
      number=4, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='acceleration', full_name='MotorMoveProto.acceleration', index=4,
      number=5, type=2, cpp_type=6, label=2,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=128,
  serialized_end=258,
)


_MOTORCONFIGPROTO = _descriptor.Descriptor(
  name='MotorConfigProto',
  full_name='MotorConfigProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='address', full_name='MotorConfigProto.address', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='zero', full_name='MotorConfigProto.zero', index=1,
      number=5, type=8, cpp_type=7, label=2,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='min_steps', full_name='MotorConfigProto.min_steps', index=2,
      number=6, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='max_steps', full_name='MotorConfigProto.max_steps', index=3,
      number=7, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=260,
  serialized_end=347,
)


_MOTORMOVEALLPROTO = _descriptor.Descriptor(
  name='MotorMoveAllProto',
  full_name='MotorMoveAllProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='motors', full_name='MotorMoveAllProto.motors', index=0,
      number=1, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\222?\002\020\006\222?\003\200\001\001', file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=349,
  serialized_end=414,
)


_MOTORTAREPROTO = _descriptor.Descriptor(
  name='MotorTareProto',
  full_name='MotorTareProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='address', full_name='MotorTareProto.address', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='tare_to_steps', full_name='MotorTareProto.tare_to_steps', index=1,
      number=2, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=416,
  serialized_end=472,
)


_MOTORREPORTPROTO = _descriptor.Descriptor(
  name='MotorReportProto',
  full_name='MotorReportProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='current_absolute_steps', full_name='MotorReportProto.current_absolute_steps', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=474,
  serialized_end=524,
)


_ALLMOTORREPORTPROTO = _descriptor.Descriptor(
  name='AllMotorReportProto',
  full_name='AllMotorReportProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='motors', full_name='AllMotorReportProto.motors', index=0,
      number=1, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\222?\002\020\006\222?\003\200\001\001', file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=526,
  serialized_end=595,
)

_MOTORMOVEALLPROTO.fields_by_name['motors'].message_type = _MOTORMOVEPROTO
_ALLMOTORREPORTPROTO.fields_by_name['motors'].message_type = _MOTORREPORTPROTO
DESCRIPTOR.message_types_by_name['MotorInitProto'] = _MOTORINITPROTO
DESCRIPTOR.message_types_by_name['MotorMoveProto'] = _MOTORMOVEPROTO
DESCRIPTOR.message_types_by_name['MotorConfigProto'] = _MOTORCONFIGPROTO
DESCRIPTOR.message_types_by_name['MotorMoveAllProto'] = _MOTORMOVEALLPROTO
DESCRIPTOR.message_types_by_name['MotorTareProto'] = _MOTORTAREPROTO
DESCRIPTOR.message_types_by_name['MotorReportProto'] = _MOTORREPORTPROTO
DESCRIPTOR.message_types_by_name['AllMotorReportProto'] = _ALLMOTORREPORTPROTO
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

MotorInitProto = _reflection.GeneratedProtocolMessageType('MotorInitProto', (_message.Message,), {
  'DESCRIPTOR' : _MOTORINITPROTO,
  '__module__' : 'motor_command_pb2'
  # @@protoc_insertion_point(class_scope:MotorInitProto)
  })
_sym_db.RegisterMessage(MotorInitProto)

MotorMoveProto = _reflection.GeneratedProtocolMessageType('MotorMoveProto', (_message.Message,), {
  'DESCRIPTOR' : _MOTORMOVEPROTO,
  '__module__' : 'motor_command_pb2'
  # @@protoc_insertion_point(class_scope:MotorMoveProto)
  })
_sym_db.RegisterMessage(MotorMoveProto)

MotorConfigProto = _reflection.GeneratedProtocolMessageType('MotorConfigProto', (_message.Message,), {
  'DESCRIPTOR' : _MOTORCONFIGPROTO,
  '__module__' : 'motor_command_pb2'
  # @@protoc_insertion_point(class_scope:MotorConfigProto)
  })
_sym_db.RegisterMessage(MotorConfigProto)

MotorMoveAllProto = _reflection.GeneratedProtocolMessageType('MotorMoveAllProto', (_message.Message,), {
  'DESCRIPTOR' : _MOTORMOVEALLPROTO,
  '__module__' : 'motor_command_pb2'
  # @@protoc_insertion_point(class_scope:MotorMoveAllProto)
  })
_sym_db.RegisterMessage(MotorMoveAllProto)

MotorTareProto = _reflection.GeneratedProtocolMessageType('MotorTareProto', (_message.Message,), {
  'DESCRIPTOR' : _MOTORTAREPROTO,
  '__module__' : 'motor_command_pb2'
  # @@protoc_insertion_point(class_scope:MotorTareProto)
  })
_sym_db.RegisterMessage(MotorTareProto)

MotorReportProto = _reflection.GeneratedProtocolMessageType('MotorReportProto', (_message.Message,), {
  'DESCRIPTOR' : _MOTORREPORTPROTO,
  '__module__' : 'motor_command_pb2'
  # @@protoc_insertion_point(class_scope:MotorReportProto)
  })
_sym_db.RegisterMessage(MotorReportProto)

AllMotorReportProto = _reflection.GeneratedProtocolMessageType('AllMotorReportProto', (_message.Message,), {
  'DESCRIPTOR' : _ALLMOTORREPORTPROTO,
  '__module__' : 'motor_command_pb2'
  # @@protoc_insertion_point(class_scope:AllMotorReportProto)
  })
_sym_db.RegisterMessage(AllMotorReportProto)


_MOTORMOVEALLPROTO.fields_by_name['motors']._options = None
_ALLMOTORREPORTPROTO.fields_by_name['motors']._options = None
# @@protoc_insertion_point(module_scope)