from building import *

group = []
if not GetDepend(['PKG_USING_THREES']):
    Return('group')

src	= Glob('*.c')

group = DefineGroup('threes', src, depend = ['PKG_USING_THREES'])

Return('group')
