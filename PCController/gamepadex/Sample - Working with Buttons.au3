#include "GamePadEx.au3"

#include <ButtonConstants.au3>
#include <GUIConstantsEx.au3>
#include <StaticConstants.au3>
#include <WindowsConstants.au3>
Opt("GUIOnEventMode", 1)

HotKeySet("{ESC}", "_Quit") ; Just for the demo

Global $myGamePadHandle = GPExInitialize() ; Get a handle. This doesn't have to be global but needs to be passed to all the other functions.
Global $myID = 0 ; We want to monitor the default GamePad

Global $Button[13]
$Form1 = GUICreate("ESCAPE TO QUIT", 450, 443, 211, 142)
$Button[1] = GUICtrlCreateButton("1", 144, 128, 33, 33, $WS_GROUP)
$Button[2] = GUICtrlCreateButton("2", 184, 128, 33, 33, $WS_GROUP)
$Button[3] = GUICtrlCreateButton("3", 224, 128, 33, 33, $WS_GROUP)
$Button[4] = GUICtrlCreateButton("4", 264, 128, 33, 33, $WS_GROUP)
$Button[5] = GUICtrlCreateButton("5", 144, 168, 33, 33, $WS_GROUP)
$Button[6] = GUICtrlCreateButton("6", 184, 168, 33, 33, $WS_GROUP)
$Button[7] = GUICtrlCreateButton("7", 224, 168, 33, 33, $WS_GROUP)
$Button[8] = GUICtrlCreateButton("8", 264, 168, 33, 33, $WS_GROUP)
$Button[9] = GUICtrlCreateButton("9", 144, 208, 33, 33, $WS_GROUP)
$Button[10] = GUICtrlCreateButton("10", 184, 208, 33, 33, $WS_GROUP)
$Button[11] = GUICtrlCreateButton("11", 224, 208, 33, 33, $WS_GROUP)
$Button[12] = GUICtrlCreateButton("12", 264, 208, 33, 33, $WS_GROUP)
$Label1 = GUICtrlCreateLabel("Press buttons on gamepad and the corresponding buttons become enabled.", 8, 8, 363, 17)
GUISetState(@SW_SHOW)

DisableAll()

While 1
	Sleep(10)
	$pressedbutton = GPExGetPressed($myGamePadHandle, $myID)
	Select
		Case BitAND($pressedbutton, GPExButton(1))
			GUICtrlSetState($Button[1], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(2))
			GUICtrlSetState($Button[2], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(3))
			GUICtrlSetState($Button[3], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(4))
			GUICtrlSetState($Button[4], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(5))
			GUICtrlSetState($Button[5], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(6))
			GUICtrlSetState($Button[6], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(7))
			GUICtrlSetState($Button[7], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(8))
			GUICtrlSetState($Button[8], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(9))
			GUICtrlSetState($Button[9], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(10))
			GUICtrlSetState($Button[10], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(11))
			GUICtrlSetState($Button[11], $GUI_ENABLE)
		Case BitAND($pressedbutton, GPExButton(12))
			GUICtrlSetState($Button[12], $GUI_ENABLE)
		Case Else
			DisableAll()
	EndSelect
WEnd

Func DisableAll()
	For $i = 1 To 12
		GUICtrlSetState($Button[$i], $GUI_DISABLE)
	Next
EndFunc   ;==>DisableAll

Func _Quit()
	GPExTerminate($myGamePadHandle) ; Just for good manners.
	Exit
EndFunc   ;==>_Quit