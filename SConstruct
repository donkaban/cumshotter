# stupid but fast

env = Environment()
env.Append(CPPFLAGS  = ['-Wall', '-Wextra','-Werror','-std=c++0x','-fPIC','-m32'])
env.Append(LINKFLAGS = ['-m32'])
env.Append(LIBS      = ['jpeg'])

env.Program(['test.cpp','webcam.cpp','jpeg.cpp'])