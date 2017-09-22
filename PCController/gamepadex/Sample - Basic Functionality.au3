#include "GamePadEx.au3"

HotKeySet("{ESC}", "_Quit") ; Just for the demo

Global $myGamePadHandle = GPExInitialize() ; Get a handle. This doesn't have to be global but needs to be passed to all the other functions.
Global $myID = 0 ; We want to monitor the default GamePad

While 1
	Sleep(10)
	$testrawdata = GPExGetRawData($myGamePadHandle, $myID) ; Get almost every data possible from the gamepad. Returns in an array with 8 elements.
	;
	; Display the RAW output.
	; --->
	; ---> Resize the console area so that you can have all the outputs and then move the gamepad. <---
	; --->
	ConsoleWrite(@CRLF & "==========================================================")
	ConsoleWrite(@CRLF & "[0]" & $testrawdata[0]) ; X Axis
	ConsoleWrite(@CRLF & "[1]" & $testrawdata[1]) ; Y Axis
	ConsoleWrite(@CRLF & "[2]" & $testrawdata[2]) ; Z Axis
	ConsoleWrite(@CRLF & "[3]" & $testrawdata[3]) ; R Axis
	ConsoleWrite(@CRLF & "[4]" & $testrawdata[4]) ; U Axis
	ConsoleWrite(@CRLF & "[5]" & $testrawdata[5]) ; V Axis
	ConsoleWrite(@CRLF & "[6]" & $testrawdata[6]) ; Point of View
	ConsoleWrite(@CRLF & "[7]" & $testrawdata[7]) ; Buttons
	ConsoleWrite(@CRLF & "==========================================================")

WEnd

Func _Quit()
	GPExTerminate($myGamePadHandle) ; Just for good manners.
	Exit
EndFunc   ;==>_Quit