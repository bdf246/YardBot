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
$ButtonXY = GUICtrlCreateButton("POV", 320, 192, 49, 49, $WS_GROUP) ; For Demo
$Label1 = GUICtrlCreateLabel("Press the Eight directional Keys (POV or Point Of View) in random ways and watch the corresponding button move around.", 8, 8, 618, 17) ; For Demo
$Label2 = GUICtrlCreateLabel("Press Space to reset position.", 8, 32, 144, 17) ; For Demo
GUISetState(@SW_SHOW) ; For Demo


While 1
	Sleep(10)
	$posXY = ControlGetPos($Form1, "", $ButtonXY)
	;$delta = GPExGetAxisEx($myGamePadHandle, $myID, "X", 600)
	;If $delta[1] > 0 Then GUICtrlSetPos($ButtonXY, $posXY[0] + $feed * $delta[2] / 100, $posXY[1])
	$POVdirection = GPExGetPOVDirection($myGamePadHandle,  $myID)
	Switch $POVdirection
		Case "U"
			GUICtrlSetPos($ButtonXY, $posXY[0], $posXY[1] - 1)
		Case "UR"
			GUICtrlSetPos($ButtonXY, $posXY[0] + 1, $posXY[1] - 1)
		Case "R"
			GUICtrlSetPos($ButtonXY, $posXY[0] + 1, $posXY[1])
		Case "DR"
			GUICtrlSetPos($ButtonXY, $posXY[0] + 1, $posXY[1] + 1)
		Case "D"
			GUICtrlSetPos($ButtonXY, $posXY[0], $posXY[1] + 1)
		Case "DL"
			GUICtrlSetPos($ButtonXY, $posXY[0] - 1, $posXY[1] + 1)
		Case "L"
			GUICtrlSetPos($ButtonXY, $posXY[0] - 1, $posXY[1])
		Case "UL"
			GUICtrlSetPos($ButtonXY, $posXY[0] -1 , $posXY[1] - 1)
		Case Else
			; Not moving
	EndSwitch

WEnd

Func _Space()
	GUICtrlSetPos($ButtonXY, 320, 192)
EndFunc   ;==>_Space

Func _Quit()
	GPExTerminate($myGamePadHandle) ; Just for good manners.
	Exit
EndFunc   ;==>_Quit