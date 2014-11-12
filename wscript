APPNAME = ''
VERSION = '1.0.0'

srcdir = '.'
blddir = 'build'

def options(opt):
    opt.load("compiler_cxx")

def configure(conf):
    conf.load("compiler_cxx")

def build(bld):
    bld.program(features = "cxx cprogram",
        source = "main.cpp",
        cxxflags = ["-std=c++14", "-g", "-Wall"],
        target = "a.out",
        includes = "")
