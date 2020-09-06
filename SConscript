Import('rtconfig')
from building import *

cwd = GetCurrentDir()
src	= Glob('*.c')

CPPPATH = [cwd]

group = DefineGroup('threes', src, depend = ['PKG_USING_THREES'], CPPPATH = CPPPATH)

Return('group')
