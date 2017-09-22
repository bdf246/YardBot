; GamePadEx UDF has built in debugging support.
; This example explains that.

;
; The debugging is Adlib based and doesn't effect (much) your main code
;

#include "GamePadEx.au3"

HotKeySet("{ESC}", "_Quit") ; Just for the demo
HotKeySet("{SPACE}", "_ToggleDebugging") ; Just for the demo / Toggle Debugging ON and OFF
Global $isEnabled ; Just for the demo

Global $myGamePadHandle = GPExInitialize() ; Get a handle. This doesn't have to be global but needs to be passed to all the other functions.
Global $myID = 0 ; We want to monitor the default GamePad.

GPExDebugEnable($myGamePadHandle, $myID) ; Enable the debugging.
$isEnabled = True ; Just for the demo

While 1
	; Your code here.
	Sleep(10)
WEnd

Func _ToggleDebugging()
	If $isEnabled = True Then
		GPExDebugDisable($myGamePadHandle, $myID) ; Disable the debugging.
		$isEnabled = False
	Else
		$isEnabled = True
		GPExDebugEnable($myGamePadHandle, $myID) ; Enable the debugging.
	EndIf
EndFunc   ;==>_ToggleDebugging

Func _Quit()
	GPExTerminate($myGamePadHandle) ; Just for good manners.
	Exit
EndFunc   ;==>_Quit