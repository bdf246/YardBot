; #INDEX# =======================================================================================================================
; Title .........: GamePadEx UDF
; UDF Version ...: 1.00
; AutoIt Version : 3.3.6.1 (Not tested on previous versions
; Language ......: English
; Description ...: Functions for receiving GamePad (almost any) inputs
; Author(s) .....: Shafayat <bitventure.x10.mx>
; Gratitude .....: Various people (or their code) influenced this UDF. They are - (from last to first)
;                  1. Nologic
;                  2. Adam1213
;                  3. Ejoc, Monamo
; License .......: Freeware/Opensource
; ===============================================================================================================================

; #CURRENT# =====================================================================================================================
;GPExInitialize
;GPExTerminate
;GPExButton
;GPExSwapButtons
;GPExGetPressed
;GPExGetAxis
;GPExGetAxisEx
;GPExGetPOV
;GPExGetPOVDirection
;GPExGetRawData
;GPExDebugEnable
;GPExDebugDisable
;GPExDebug
; ===============================================================================================================================

; #INTERNAL_USE_ONLY# ===========================================================================================================
; ___GPExDebugAuto() ; Basically an Adlib
; ===============================================================================================================================

; #INCLUDES# ====================================================================================================================
; #include "Array.au3" - I used this only for testing. Not required.
; ===============================================================================================================================

; #GLOBAL VARS# =================================================================================================================
; Do not bother. You'll never need to use these variables in your script.
Global $___GPExButtonsArray[13] = [0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048]
Global $___GPExAdlibParams[2] = [0, 0]
; ===============================================================================================================================

; #FUNCTION# ====================================================================================================================
; Name...........: GPExInitialize
; Description ...: Initialize and Return Handle to the GamePadEx Inputs
; Return values .: Success - Handle to the GamePad Inputs
;                  Failure - 0
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExInitialize()
	Local $joy
	Local $JOYINFOEX_struct = "dword[13]"
	$joy = DllStructCreate($JOYINFOEX_struct)
	If @error Then Return 0
	DllStructSetData($joy, 1, DllStructGetSize($joy), 1) ; dwSize  = sizeof(struct)
	DllStructSetData($joy, 1, 255, 2) ; dwFlags = GetAll
	Return $joy
EndFunc   ;==>GPExInitialize

; #FUNCTION# ====================================================================================================================
; Name...........: GPExTerminate
; Description ...: Removes Handle to the GamePadEx Inputs
; Return values .: True
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExTerminate(ByRef $givenhandle)
	$givenhandle = 0
	Return True
EndFunc   ;==>GPExTerminate

; #FUNCTION# ====================================================================================================================
; Name...........: GPExButton
; Description ...: Get the nth Button's Code Number.
; Parameters ....: $ButtonNumber - A number between 1 to 12.
; Return values .: Success - nth Button's Code Number
;                  Failure - -1
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExButton($ButtonNumber)
	If $ButtonNumber > 0 & $ButtonNumber < 13 Then
		Return $___GPExButtonsArray[$ButtonNumber]
	Else
		Return -1
	EndIf
EndFunc   ;==>GPExButton

; #FUNCTION# ====================================================================================================================
; Name...........: GPExSwapButtons
; Description ...: Swap 2 buttons. Good for a GamePad that uses UnOrthobox button placement.
; Parameters ....: $Button1Number - A number between 1 to 12.
;                  $Button2Number - A number between 1 to 12.
; Return values .: Success - 1
;                  Failure - -1
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExSwapButtons($Button1Number, $Button2Number)
	If $Button1Number > 0 And $Button1Number < 13 And $Button2Number > 0 And $Button2Number < 13 Then
		Local $b1
		$b1 = $___GPExButtonsArray[$Button1Number]
		$___GPExButtonsArray[$Button1Number] = $___GPExButtonsArray[$Button2Number]
		$___GPExButtonsArray[$Button2Number] = $b1
		Return 1
	Else
		Return -1
	EndIf
EndFunc   ;==>GPExSwapButtons

; #FUNCTION# ====================================================================================================================
; Name...........: GPExGetPressed
; Description ...: Get the pressed button code.
; Parameters ....: $tempHandle - Handle from GPExInitialize
;                  $tempID - ID of gamepad. (use 0 for the default one)
; Return values .: Button Code.
; Remarks .......: Use BitAnd to find out if multiple buttons are pressed. (try the example)
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExGetPressed($tempHandle, $tempID)
	Local $tempArray = GPExGetRawData($tempHandle, $tempID)
	Return $tempArray[7]
EndFunc   ;==>GPExGetPressed

; #FUNCTION# ====================================================================================================================
; Name...........: GPExGetAxis
; Description ...: Get the Orientation of Axis
; Parameters ....: $tempHandle - Handle from GPExInitialize
;                  $tempID - ID of gamepad. (use 0 for the default one)
;                  $tempAxis - The Axis, of which the value has to be returned.
; Return values .: Success - Orientation of Axis. from 0 to 65535
;                  Failure - -1
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExGetAxis($tempHandle, $tempID, $tempAxis)
	Local $tempArray = GPExGetRawData($tempHandle, $tempID)
	Switch $tempAxis
		Case "X"
			Return $tempArray[0]
		Case "Y"
			Return $tempArray[1]
		Case "Z"
			Return $tempArray[2]
		Case "R"
			Return $tempArray[3]
		Case "U"
			Return $tempArray[4]
		Case "V"
			Return $tempArray[5]
		Case Else
			Return -1
	EndSwitch
EndFunc   ;==>GPExGetAxis

; #FUNCTION# ====================================================================================================================
; Name...........: GPExGetAxisEx
; Description ...: Get the Orientation of Axis and more details.
; Parameters ....: $tempHandle - Handle from GPExInitialize
;                  $tempID - ID of gamepad. (use 0 for the default one)
;                  $tempAxis - The Axis, of which the value has to be returned.
;                  $tolerance - Tolerance means the Threshold of orientation to be ignored.
; Return values .: Success - An Array.
;                            [0] - Raw data (same as GPExGetAxis)
;                            [1] - Direction with Zero as Middle Point. (Left < 0, Middle = 0, Right > 0 etc..)
;                            [2] - Percentage of Orientation. Very useful when trying to find out how much the Radars Tilted.
;                  Failure - -1
; Remarks .......: Higher values of Tolerance will make GamePad a less responsive/sensitive.
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExGetAxisEx($tempHandle, $tempID, $tempAxis, $tolerance = 6000)
	If $tolerance > 65535 / 2 Or $tolerance < -65535 / 2 Then Return -2
	Local $tempArray = GPExGetRawData($tempHandle, $tempID)
	Local $tempArray2[3] = [0, 0, 0]
	Switch $tempAxis
		Case "X"
			$tempArray2[0] = $tempArray[0]
			$tempArray2[1] = $tempArray2[0] - 65535 / 2
			If $tolerance > $tempArray2[1] And $tolerance * (-1) < $tempArray2[1] Then $tempArray2[1] = 0
			$tempArray2[2] = Abs($tempArray2[0] - 65535 / 2) / (65535 / 2) * 100
		Case "Y"
			$tempArray2[0] = $tempArray[1]
			$tempArray2[1] = $tempArray2[0] - 65535 / 2
			If $tolerance > $tempArray2[1] And $tolerance * (-1) < $tempArray2[1] Then $tempArray2[1] = 0
			$tempArray2[2] = Abs($tempArray2[0] - 65535 / 2) / (65535 / 2) * 100
		Case "Z"
			$tempArray2[0] = $tempArray[2]
			$tempArray2[1] = $tempArray2[0] - 65535 / 2
			If $tolerance > $tempArray2[1] And $tolerance * (-1) < $tempArray2[1] Then $tempArray2[1] = 0
			$tempArray2[2] = Abs($tempArray2[0] - 65535 / 2) / (65535 / 2) * 100
		Case "R"
			$tempArray2[0] = $tempArray[3]
			$tempArray2[1] = $tempArray2[0] - 65535 / 2
			If $tolerance > $tempArray2[1] And $tolerance * (-1) < $tempArray2[1] Then $tempArray2[1] = 0
			$tempArray2[2] = Abs($tempArray2[0] - 65535 / 2) / (65535 / 2) * 100
		Case "U"
			$tempArray2[0] = $tempArray[4]
			$tempArray2[1] = $tempArray2[0] - 65535 / 2
			If $tolerance > $tempArray2[1] And $tolerance * (-1) < $tempArray2[1] Then $tempArray2[1] = 0
			$tempArray2[2] = Abs($tempArray2[0] - 65535 / 2) / (65535 / 2) * 100
		Case "V"
			$tempArray2[0] = $tempArray[5]
			$tempArray2[1] = $tempArray2[0] - 65535 / 2
			If $tolerance > $tempArray2[1] And $tolerance * (-1) < $tempArray2[1] Then $tempArray2[1] = 0
			$tempArray2[2] = Abs($tempArray2[0] - 65535 / 2) / (65535 / 2) * 100
		Case Else
			Return -1
	EndSwitch
	Return $tempArray2
EndFunc   ;==>GPExGetAxisEx

; #FUNCTION# ====================================================================================================================
; Name...........: GPExGetPOV
; Description ...: Get the Point of View as Raw Data.
; Parameters ....: $tempHandle - Handle from GPExInitialize
;                  $tempID - ID of gamepad. (use 0 for the default one)
; Return values .: POV Data
; Remarks .......: POV is the Eight directional buttons on the Gamepad
;                  65535  = Not pressed
;                  0      = Up
;                  4500   = Up-Right
;                  9000   = Right
;                  Goes around clockwise increasing 4500 for each position.
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExGetPOV($tempHandle, $tempID)
	Local $tempArray = GPExGetRawData($tempHandle, $tempID)
	Return $tempArray[6]
EndFunc   ;==>GPExGetPOV

; #FUNCTION# ====================================================================================================================
; Name...........: GPExGetPOVDirection
; Description ...: Get the Point of View as Directions.
; Parameters ....: $tempHandle - Handle from GPExInitialize
;                  $tempID - ID of gamepad. (use 0 for the default one)
; Return values .: POV Directions.
;                  Blank string  - Not pressed
;                  U = Up
;                  UR = Up-Right
;                  R..
;                  DR..
;                  D..
;                  DL..
;                  L..
;                  UL..
; Remarks .......: POV is the Eight directional buttons on the Gamepad
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExGetPOVDirection($tempHandle, $tempID)
	Local $tempArray = GPExGetRawData($tempHandle, $tempID)
	Switch $tempArray[6]
		Case 4500 * 0
			Return "U"
		Case 4500 * 1
			Return "UR"
		Case 4500 * 2
			Return "R"
		Case 4500 * 3
			Return "DR"
		Case 4500 * 4
			Return "D"
		Case 4500 * 5
			Return "DL"
		Case 4500 * 6
			Return "L"
		Case 4500 * 7
			Return "UL"
		Case 65535
			Return ""
	EndSwitch
EndFunc   ;==>GPExGetPOVDirection

; #FUNCTION# ====================================================================================================================
; Name...........: GPExGetRawData
; Description ...: Returns Array containing X-Pos, Y-Pos, Z-Pos, R-Pos, U-Pos, V-Pos, POV and Pressed Button(s)
; Parameters ....: $tempHandle - Handle from GPExInitialize
;                  $tempID - ID of gamepad. (use 0 for the default one)
; Return values .: Array containing X-Pos, Y-Pos, Z-Pos, R-Pos, U-Pos, V-Pos, POV and Pressed Button(s)
; Remarks .......: Try the Samples provided.
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExGetRawData($tempHandle, $tempID)
	Local $coor
	Dim $coor[8]
	DllCall("Winmm.dll", "int", "joyGetPosEx", _
			"int", $tempID, "ptr", DllStructGetPtr($tempHandle))
	If Not @error Then
		$coor[0] = DllStructGetData($tempHandle, 1, 3) ; X Axis
		$coor[1] = DllStructGetData($tempHandle, 1, 4) ; Y Axis
		$coor[2] = DllStructGetData($tempHandle, 1, 5) ; Z Axis
		$coor[3] = DllStructGetData($tempHandle, 1, 6) ; R Axis
		$coor[4] = DllStructGetData($tempHandle, 1, 7) ; U Axis
		$coor[5] = DllStructGetData($tempHandle, 1, 8) ; V Axis
		$coor[6] = DllStructGetData($tempHandle, 1, 11) ; POV Value
		$coor[7] = DllStructGetData($tempHandle, 1, 9) ; Buttons Mask
	EndIf
	Return $coor
EndFunc   ;==>GPExGetRawData

; #FUNCTION# ====================================================================================================================
; Name...........: GPExDebugEnable
; Description ...: Start Debugging
; Parameters ....: $tempHandle - Handle from GPExInitialize
;                  $tempID - ID of gamepad. (use 0 for the default one)
; Return values .: True
; Remarks .......: Try the Samples provided.
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExDebugEnable($myGamePadHandle, $tempID)
	$___GPExAdlibParams[0] = $myGamePadHandle
	$___GPExAdlibParams[1] = $tempID
	AdlibRegister("___GPExDebugAuto", 100)
	Return True
EndFunc   ;==>GPExDebugEnable

; #FUNCTION# ====================================================================================================================
; Name...........: GPExDebugDisable
; Description ...: Stop Debugging
; Parameters ....: $tempHandle - Handle from GPExInitialize
;                  $tempID - ID of gamepad. (use 0 for the default one)
; Return values .: True
; Remarks .......: Try the Samples provided.
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExDebugDisable($myGamePadHandle, $tempID)
	If $tempID = $___GPExAdlibParams[1] Then
		$___GPExAdlibParams[0] = 0
		$___GPExAdlibParams[1] = 0
		AdlibUnRegister("___GPExDebugAuto")
		Return True
	EndIf
EndFunc   ;==>GPExDebugDisable

Func ___GPExDebugAuto()
	$myGamePadHandle = $___GPExAdlibParams[0]
	$tempID = $___GPExAdlibParams[1]
	Local $GamePad_1 = GPExGetRawData($myGamePadHandle, $tempID)
	ConsoleWrite(@CRLF & "=======================================")
	ConsoleWrite(@CRLF & "BUTTON PRESS {" & $GamePad_1[7] & "}")
	ConsoleWrite(@CRLF & "X,Y AXIS {" & $GamePad_1[0] & " , " & $GamePad_1[1] & "}")
	ConsoleWrite(@CRLF & "Z,R AXIS {" & $GamePad_1[2] & " , " & $GamePad_1[3] & "}")
	ConsoleWrite(@CRLF & "U,X AXIS {" & $GamePad_1[4] & " , " & $GamePad_1[5] & "}")
	ConsoleWrite(@CRLF & "POV {" & $GamePad_1[6] & "}")
	ConsoleWrite(@CRLF & "=======================================")
EndFunc   ;==>___GPExDebugAuto

; #FUNCTION# ====================================================================================================================
; Name...........: GPExDebug
; Description ...: Manual Debugging to Console
; Parameters ....: $tempHandle - Handle from GPExInitialize
;                  $tempID - ID of gamepad. (use 0 for the default one)
; Return values .: True
; Author ........: Same as Header
; ==============================================================================================================================
Func GPExDebug($myGamePadHandle, $tempID)
	Local $GamePad_1 = GPExGetRawData($myGamePadHandle, $tempID)
	ConsoleWrite(@CRLF & "=======================================")
	ConsoleWrite(@CRLF & "BUTTON PRESS {" & $GamePad_1[7] & "}")
	ConsoleWrite(@CRLF & "X,Y AXIS {" & $GamePad_1[0] & " , " & $GamePad_1[1] & "}")
	ConsoleWrite(@CRLF & "Z,R AXIS {" & $GamePad_1[2] & " , " & $GamePad_1[3] & "}")
	ConsoleWrite(@CRLF & "U,X AXIS {" & $GamePad_1[4] & " , " & $GamePad_1[5] & "}")
	ConsoleWrite(@CRLF & "POV {" & $GamePad_1[6] & "}")
	ConsoleWrite(@CRLF & "=======================================")
	Return True
EndFunc   ;==>GPExDebug