%include "Std.inc"

global UI_Start
global UI_End
datasect
UI_Start:
	incbin "Debugger.glade"
	db 0
UI_End:
