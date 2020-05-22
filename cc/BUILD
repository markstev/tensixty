package(default_visibility = ["//visibility:public"])

cc_library(name = "packet",
           srcs = ["packet.cc"],
           hdrs = ["packet.h"],
)

cc_library(name = "commlink",
           srcs = ["commlink.cc"],
           hdrs = ["commlink.h"],
           deps = [
               ":packet",
               ":interfaces",
           ],
)

cc_library(name = "interfaces",
           hdrs = [
               "arduino.h",
               "clock_interface.h",
               "serial_interface.h",
           ],
           )

cc_library(name = "module_dispatcher",
           srcs = ["module_dispatcher.cc"],
           hdrs = ["module_dispatcher.h"],
		deps = ["@com_github_nanopb_nanopb//:nanopb"],
)

cc_library(name = "serial_module",
           srcs = ["serial_module.cc"],
           hdrs = ["serial_module.h"],
           deps = [
               ":module_dispatcher",
               ":commlink",
               ":interfaces"
           ]
)

cc_library(name = "motor",
           srcs = ["motor.cc"],
           hdrs = ["motor.h"],
           deps = [
               ":motor_command_proto",
               ":interfaces",
         ]
)

cc_library(name = "motor_bank_module",
           srcs = ["motor_bank_module.cc"],
           hdrs = ["motor_bank_module.h"],
           deps = [
               ":module_dispatcher",
               ":motor",
               ":motor_command_proto",
           ],
)

cc_library(name = "read_module",
           srcs = ["read_module.cc"],
           hdrs = ["read_module.h"],
           deps = [
               ":interfaces",
               ":module_dispatcher",
               ":motor_command_proto",
         ],
)

genrule(
		name = "motor_command_generator",
		srcs = ["motor_command.proto"],
		outs = ["_motor_command.pb.h",
						"_motor_command.pb.c"],
		cmd = "python3 /Users/mark/nanopb/generator/nanopb_generator.py $(location motor_command.proto) \
				&& sed 's/#include <pb.h>/#include \"pb.h\"/' cc/motor_command.pb.h \
        > $(location _motor_command.pb.h) \
				&& cp cc/motor_command.pb.c $(location _motor_command.pb.c)",
)

genrule(
    name = "motor_command_generator_h",
    srcs = [":motor_command_generator"],
    outs = ["motor_command.pb.h"],
    cmd = "cp $$(echo $(locations :motor_command_generator) | sed 's/[^ \t]*\.c[^ \t]*//') $@"
)

genrule(
    name = "motor_command_generator_c",
    srcs = [":motor_command_generator"],
    outs = ["motor_command.pb.c"],
    cmd = "cp $$(echo $(locations :motor_command_generator) | sed 's/[^ \t]*\.h$$//') $@"
)

cc_library(
		name = "motor_command_proto",
    #hdrs = ["motor_command.pb.h"],
    #srcs = ["motor_command.pb.c"],
		srcs = [":motor_command_generator_c"],
		hdrs = [":motor_command_generator_h"],
		deps = ["@com_github_nanopb_nanopb//:nanopb"],
		)
