; GPExGetAxisEx command makes it really easy to get the gamepad's direction and orientation.
; If you like manual work, use GPExGetAxis instead. Look into GPExGetAxisEx in the include file to learn how to parse values returned by GPExGetAxis

#include "GamePadEx.au3"


#include <ButtonConstants.au3> ; For Demo
#include <GUIConstantsEx.au3> ; For Demo
#include <StaticConstants.au3> ; For Demo
#include <WindowsConstants.au3> ; For Demo

HotKeySet("{ESC}", "_Quit") ; Just for the demo
HotKeySet("{SPACE}", "_Space") ; Just for the demo

Opt("GUIOnEventMode", 1)
Global $myGamePadHandle = GPExInitialize() ; Get a handle. This doesn't have to be global but needs to be passed to all the other functions.
Global $myID = 0 ; We want to monitor the default GamePad

$Form1 = GUICreate("ESCAPE TO QUIT", 705, 584, 192, 124) ; For Demo
$ButtonXY = GUICtrlCreateButton("X,Y", 320, 192, 49, 49, $WS_GROUP) ; For Demo
$ButtonZR = GUICtrlCreateButton("Z,R", 256, 256, 49, 49, $WS_GROUP) ; For Demo
;~ $ButtonUV = GUICtrlCreateButton("U,V", 368, 256, 49, 49, $WS_GROUP) ; For Demo
$Label1 = GUICtrlCreateLabel("Tilt the various directional Radars (NOT the POINT OF VIEW) in random ways and watch the corresponding buttons move around.", 8, 8, 618, 17) ; For Demo
$Label2 = GUICtrlCreateLabel("Press Space to reset position.   NOTE: Tilt further to accelerate the speed.", 8, 32, 1440, 17) ; For Demo
GUISetState(@SW_SHOW) ; For Demo

$feed = 5 ; for the demo
;ConsoleWrite(@CRLF & $X[1])

While 1
	Sleep(10)
	$posXY = ControlGetPos($Form1, "", $ButtonXY)
	$delta = GPExGetAxisEx($myGamePadHandle, $myID, "X", 600)
	If $delta[1] > 0 Then GUICtrlSetPos($ButtonXY, $posXY[0] + $feed * $delta[2] / 100, $posXY[1])
	If $delta[1] < 0 Then GUICtrlSetPos($ButtonXY, $posXY[0] - $feed * $delta[2] / 100, $posXY[1])
	$posXY = ControlGetPos($Form1, "", $ButtonXY)
	$delta = GPExGetAxisEx($myGamePadHandle, $myID, "Y", 600)
	If $delta[1] > 0 Then GUICtrlSetPos($ButtonXY, $posXY[0], $posXY[1] + $feed * $delta[2] / 100)
	If $delta[1] < 0 Then GUICtrlSetPos($ButtonXY, $posXY[0], $posXY[1] - $feed * $delta[2] / 100)
	$posXY = ControlGetPos($Form1, "", $ButtonZR)
	$delta = GPExGetAxisEx($myGamePadHandle, $myID, "Z", 600)
	If $delta[1] > 0 Then GUICtrlSetPos($ButtonZR, $posXY[0] + $feed * $delta[2] / 100, $posXY[1])
	If $delta[1] < 0 Then GUICtrlSetPos($ButtonZR, $posXY[0] - $feed * $delta[2] / 100, $posXY[1])
	$posXY = ControlGetPos($Form1, "", $ButtonZR)
	$delta = GPExGetAxisEx($myGamePadHandle, $myID, "R", 600)
	If $delta[1] > 0 Then GUICtrlSetPos($ButtonZR, $posXY[0], $posXY[1] + $feed * $delta[2] / 100)
	If $delta[1] < 0 Then GUICtrlSetPos($ButtonZR, $posXY[0], $posXY[1] - $feed * $delta[2] / 100)
;~ 	$posXY = ControlGetPos($Form1, "", $ButtonUV)
;~ 	$delta = GPExGetAxisEx($myGamePadHandle, $myID, "U", 600)
;~ 	If $delta[1] > 0 Then GUICtrlSetPos($ButtonUV, $posXY[0] + 2 * $delta[2] / 100, $posXY[1])
;~ 	If $delta[1] < 0 Then GUICtrlSetPos($ButtonUV, $posXY[0] - 2 * $delta[2] / 100, $posXY[1])
;~ 	$posXY = ControlGetPos($Form1, "", $ButtonUV)
;~ 	$delta = GPExGetAxisEx($myGamePadHandle, $myID, "V", 600)
;~ 	If $delta[1] > 0 Then GUICtrlSetPos($ButtonUV, $posXY[0], $posXY[1] + 2 * $delta[2] / 100)
;~ 	If $delta[1] < 0 Then GUICtrlSetPos($ButtonUV, $posXY[0], $posXY[1] - 2 * $delta[2] / 100)
WEnd

Func _Space()
	GUICtrlSetPos($ButtonXY, 320, 192)
	GUICtrlSetPos($ButtonZR, 256, 256)
EndFunc   ;==>_Space

Func _Quit()
	GPExTerminate($myGamePadHandle) ; Just for good manners.
	Exit
EndFunc   ;==>_Quit